// Exercises the WASM contract natively. The functions under test are the exact
// ones exported to WebAssembly, so this verifies the logic without needing an
// Emscripten toolchain; only the wasm codegen step is left unproven.
#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstring>
#include <string>

using json = nlohmann::json;

extern "C" {
const char* pw_version();
void pw_reset();
const char* pw_register_route(const char* method, const char* pattern);
const char* pw_dispatch(const char* method, const char* path);
const char* pw_parse_request(const char* rawText);
double pw_bench_dispatch(const char* method, const char* path, int iters);
const char* pw_dump_trie();
}

static int failures = 0;

static void check(bool cond, const std::string& what, const std::string& detail = "") {
    printf("  [%s] %s%s\n", cond ? "PASS" : "FAIL", what.c_str(),
           detail.empty() ? "" : ("  -> " + detail).c_str());
    if (!cond) failures++;
}

int main() {
    printf("== version() ==\n");
    json v = json::parse(pw_version());
    check(v.contains("version") && v.contains("commit") && v.contains("builtAt"),
          "returns { version, commit, builtAt }", v.dump());

    printf("\n== reset() + registerRoute() ==\n");
    pw_reset();
    json r0 = json::parse(pw_register_route("GET", "/users/:id"));
    json r1 = json::parse(pw_register_route("GET", "/users/new"));
    json r2 = json::parse(pw_register_route("POST", "/users"));
    check(r0.value("routeId", -1) == 0, "first route gets id 0", r0.dump());
    check(r1.value("routeId", -1) == 1, "ids increment", r1.dump());
    check(r2.value("routeId", -1) == 2, "ids span methods", r2.dump());

    json dup = json::parse(pw_register_route("GET", "/users/:id"));
    check(dup.contains("error"), "duplicate route reports an error", dup.dump());
    json bad = json::parse(pw_register_route("GET", "users/no-slash"));
    check(bad.contains("error"), "pattern without leading '/' errors", bad.dump());
    json bad2 = json::parse(pw_register_route("", "/x"));
    check(bad2.contains("error"), "empty method errors", bad2.dump());

    printf("\n== dispatch() ==\n");
    json d = json::parse(pw_dispatch("GET", "/users/42"));
    check(d.value("matched", false) && d.value("routeId", -1) == 0,
          "parameter route matches", d.dump());
    check(d["params"].value("id", "") == "42", "binds :id", d["params"].dump());
    check(d.contains("ms") && d["ms"].get<double>() >= 0.0, "reports ms");

    json lit = json::parse(pw_dispatch("GET", "/users/new"));
    check(lit.value("routeId", -1) == 1, "literal beats parameter", lit.dump());

    json miss = json::parse(pw_dispatch("GET", "/nope"));
    check(!miss.value("matched", true) && miss.value("routeId", 0) == -1,
          "miss reports matched=false, routeId=-1", miss.dump());

    json wrongMethod = json::parse(pw_dispatch("DELETE", "/users/42"));
    check(!wrongMethod.value("matched", true), "method is part of the key",
          wrongMethod.dump());

    json post = json::parse(pw_dispatch("POST", "/users"));
    check(post.value("routeId", -1) == 2, "POST route matches", post.dump());

    printf("\n== parseRequest() ==\n");
    json p = json::parse(pw_parse_request(
        "GET /search?q=cats&n=2 HTTP/1.1\r\nHost: localhost:8084\r\n"
        "User-Agent: probe\r\n\r\n"));
    check(p.value("ok", false), "valid request parses", p.dump().substr(0, 120));
    check(p.value("method", "") == "GET", "method");
    check(p.value("path", "") == "/search", "path excludes the query");
    check(p.value("version", "") == "1.1", "version");
    check(p["headers"].value("Host", "") == "localhost:8084",
          "header value keeps its colon");
    check(p.value("bodyLength", -1) == 0, "bodyLength 0 for a bodyless GET");

    json pb = json::parse(pw_parse_request(
        "POST /echo HTTP/1.1\r\nHost: h\r\nContent-Type: text/plain\r\n"
        "Content-Length: 11\r\n\r\nhello world"));
    check(pb.value("ok", false) && pb.value("bodyLength", -1) == 11,
          "body length is reported", pb.dump().substr(0, 140));

    json p10 = json::parse(pw_parse_request("GET /x HTTP/1.0\r\nHost: h\r\n\r\n"));
    check(p10.value("version", "") == "1.0", "HTTP/1.0 version", p10.dump().substr(0, 80));

    json pbad = json::parse(pw_parse_request("GARBAGE\r\n\r\n"));
    check(!pbad.value("ok", true) && pbad.contains("error"),
          "garbage request reports an error, does not crash", pbad.dump());

    json ppart = json::parse(pw_parse_request("GET /x HTTP/1.1\r\nHost: h\r\n"));
    check(!ppart.value("ok", true), "incomplete request is not ok", ppart.dump());

    json pempty = json::parse(pw_parse_request(""));
    check(!pempty.value("ok", true), "empty input is not ok", pempty.dump());

    printf("\n== benchDispatch() ==\n");
    double hit = pw_bench_dispatch("GET", "/users/42", 20000);
    double missNs = pw_bench_dispatch("GET", "/nope", 20000);
    check(hit > 0, "returns nanos/op for a hit", std::to_string(hit) + " ns");
    check(missNs > 0, "returns nanos/op for a miss", std::to_string(missNs) + " ns");
    check(pw_bench_dispatch("GET", "/x", 0) < 0, "iters<=0 returns -1");

    printf("\n== dumpTrie() ==\n");
    json t = json::parse(pw_dump_trie());
    check(t.value("routeCount", -1) == 3, "routeCount matches registrations");
    check(t["root"].contains("children"), "root has children");
    bool sawGet = false, sawParam = false;
    std::function<void(const json&)> walk = [&](const json& n) {
        if (n.value("value", "") == "GET:") sawGet = true;
        if (n.value("isParameter", false) && n.value("parameterName", "") == "id")
            sawParam = true;
        for (const auto& c : n["children"]) walk(c);
    };
    walk(t["root"]);
    check(sawGet, "depth 1 holds the method key 'GET:'");
    check(sawParam, "parameter node ':id' is present with its name");
    check(t["routes"].size() == 3, "routes list is returned");

    printf("\n== reset() clears everything ==\n");
    pw_reset();
    json after = json::parse(pw_dump_trie());
    check(after.value("routeCount", -1) == 0, "routeCount back to 0", after.dump().substr(0, 90));
    json afterD = json::parse(pw_dispatch("GET", "/users/42"));
    check(!afterD.value("matched", true), "previously registered route no longer matches");
    json reAdd = json::parse(pw_register_route("GET", "/users/:id"));
    check(reAdd.value("routeId", -1) == 0, "ids restart after reset", reAdd.dump());

    printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "ALL CHECKS PASSED",
           failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
