// WebAssembly contract for PlusWeb's routing trie and HTTP parser.
//
// Only the parts of PlusWeb that make sense without sockets are exposed: the
// router and the parser. There is no event loop here, so libuv is not linked.
//
// The ABI is deliberately plain C with JSON strings, so the module works under
// any WASM host, not just Emscripten's JS glue, and so the exact same code can
// be compiled and tested natively (see test_contract.cpp).
//
// Returned pointers stay valid until the next call to the same function. The
// module is single-threaded, which is what WASM gives us anyway.

#include <PlusWeb/HttpRequest.h>
#include <PlusWeb/HttpResponse.h>
#include <PlusWeb/RouteRegistry.h>
#include <PlusWeb/trie.h>

#include "HttpParser.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define PW_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define PW_EXPORT
#endif

#ifndef PLUSWEB_VERSION
#define PLUSWEB_VERSION "unknown"
#endif
#ifndef PLUSWEB_COMMIT
#define PLUSWEB_COMMIT "unknown"
#endif
#ifndef PLUSWEB_BUILT_AT
#define PLUSWEB_BUILT_AT "unknown"
#endif

using json = nlohmann::json;
using Clock = std::chrono::steady_clock;

namespace {

struct State {
    std::unique_ptr<RouteRegistry> registry{new RouteRegistry()};
    std::vector<std::string> patterns;   // routeId -> "METHOD pattern"
    int lastRouteId = -1;                // set by the stub handler on a match
};

State& state() {
    static State s;
    return s;
}

// Each returned string is owned by its own slot so that one call's result is not
// clobbered by an unrelated call.
const char* hold(int slot, std::string value) {
    static std::string slots[8];
    slots[slot] = std::move(value);
    return slots[slot].c_str();
}

void dumpNode(const Node* node, json& out) {
    out["value"] = node->value;
    out["isParameter"] = node->isParameter;
    if (node->isParameter) {
        out["parameterName"] = node->parameterName;
    }
    out["isLeaf"] = node->isLeaf;
    out["hasHandler"] = static_cast<bool>(node->handler);

    json kids = json::array();
    for (const auto& pair : node->children) {
        json child = json::object();
        dumpNode(pair.second, child);
        kids.push_back(std::move(child));
    }
    if (node->paramChild) {
        json child = json::object();
        dumpNode(node->paramChild, child);
        kids.push_back(std::move(child));
    }
    // unordered_map iteration order is arbitrary; sort so the dump is stable.
    std::sort(kids.begin(), kids.end(), [](const json& a, const json& b) {
        return a["value"].get<std::string>() < b["value"].get<std::string>();
    });
    out["children"] = std::move(kids);
}

}  // namespace

extern "C" {

// version() -> { version, commit, builtAt }
PW_EXPORT const char* pw_version() {
    return hold(0, json{{"version", PLUSWEB_VERSION},
                        {"commit", PLUSWEB_COMMIT},
                        {"builtAt", PLUSWEB_BUILT_AT}}
                       .dump());
}

// reset() -> void. Drops every registered route and the whole node graph.
PW_EXPORT void pw_reset() {
    state().registry.reset(new RouteRegistry());
    state().patterns.clear();
    state().lastRouteId = -1;
}

// registerRoute(method, pattern) -> { routeId } | { error }
PW_EXPORT const char* pw_register_route(const char* method, const char* pattern) {
    State& s = state();
    if (!method || !*method) {
        return hold(1, json{{"error", "method is empty"}}.dump());
    }
    if (!pattern || *pattern != '/') {
        return hold(1, json{{"error", "pattern must start with '/'"}}.dump());
    }

    const std::string key = std::string(method) + " " + pattern;
    for (size_t i = 0; i < s.patterns.size(); ++i) {
        if (s.patterns[i] == key) {
            return hold(1, json{{"error", "route already registered"},
                                {"routeId", static_cast<int>(i)}}
                               .dump());
        }
    }

    const int id = static_cast<int>(s.patterns.size());
    s.patterns.push_back(key);
    // The trie stores handlers, not ids, so the handler reports its own id.
    s.registry->Register(method, pattern,
                         [id](HttpRequest&, HttpResponse&) { state().lastRouteId = id; });
    return hold(1, json{{"routeId", id}}.dump());
}

// dispatch(method, path) -> { matched, routeId, params, ms }
PW_EXPORT const char* pw_dispatch(const char* method, const char* path) {
    State& s = state();
    HttpRequest req;
    req.method = method ? method : "";
    req.path = path ? path : "";

    s.lastRouteId = -1;
    const auto t0 = Clock::now();
    auto handler = s.registry->getHandler(req);
    const double ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();

    json params = json::object();
    json out{{"matched", false}, {"routeId", -1}, {"params", params}, {"ms", ms}};
    if (!handler) {
        return hold(2, out.dump());
    }

    HttpResponse res;
    handler(req, res);   // records the route id
    for (const auto& kv : req.params) {
        params[kv.first] = kv.second;
    }
    out["matched"] = true;
    out["routeId"] = s.lastRouteId;
    out["params"] = std::move(params);
    if (s.lastRouteId >= 0 && s.lastRouteId < static_cast<int>(s.patterns.size())) {
        out["pattern"] = s.patterns[s.lastRouteId];
    }
    return hold(2, out.dump());
}

// parseRequest(rawText) -> { ok, method, path, version, headers, bodyLength, error }
PW_EXPORT const char* pw_parse_request(const char* rawText) {
    const std::string raw = rawText ? rawText : "";

    HttpParser parser;
    bool got = false;
    json out;

    const bool ok = parser.execute(raw.data(), raw.size(),
                                   [&](HttpRequest& req, bool /*keepAlive*/) {
        got = true;
        std::string version = req.protocol;
        if (version.rfind("HTTP/", 0) == 0) {
            version = version.substr(5);   // "HTTP/1.1" -> "1.1"
        }
        json headers = json::object();
        for (const auto& kv : req.headers) {
            headers[kv.first] = kv.second;
        }
        out = json{{"ok", true},
                   {"method", req.method},
                   {"path", req.path},
                   {"version", version},
                   {"headers", headers},
                   {"bodyLength", static_cast<uint64_t>(req.body.getRaw().size())},
                   {"error", nullptr}};
        if (!req.query.empty()) {
            json q = json::object();
            for (const auto& kv : req.query) q[kv.first] = kv.second;
            out["query"] = std::move(q);
        }
    });

    if (!ok) {
        return hold(3, json{{"ok", false}, {"error", parser.error()}}.dump());
    }
    if (!got) {
        return hold(3, json{{"ok", false},
                            {"error", "incomplete request: headers did not terminate"}}
                           .dump());
    }
    return hold(3, out.dump());
}

// benchDispatch(method, path, iters) -> nanosPerOp
PW_EXPORT double pw_bench_dispatch(const char* method, const char* path, int iters) {
    if (iters <= 0) {
        return -1.0;
    }
    State& s = state();
    HttpRequest req;
    req.method = method ? method : "";
    req.path = path ? path : "";

    for (int i = 0; i < iters / 10 + 1; ++i) {   // warm up
        s.registry->getHandler(req);
    }

    // Best of 3, so a scheduler hiccup does not become the reported number.
    double best = -1.0;
    for (int rep = 0; rep < 3; ++rep) {
        const auto t0 = Clock::now();
        for (int i = 0; i < iters; ++i) {
            auto h = s.registry->getHandler(req);
            (void)h;
        }
        const double ns =
            std::chrono::duration<double, std::nano>(Clock::now() - t0).count() / iters;
        if (best < 0 || ns < best) {
            best = ns;
        }
    }
    return best;
}

// dumpTrie() -> JSON of the node graph
PW_EXPORT const char* pw_dump_trie() {
    State& s = state();
    json root = json::object();
    dumpNode(s.registry->trie.rootNode(), root);

    json routes = json::array();
    for (size_t i = 0; i < s.patterns.size(); ++i) {
        routes.push_back(json{{"routeId", static_cast<int>(i)}, {"route", s.patterns[i]}});
    }
    // Keys are "METHOD:/a/b", split on '/', so the first level is "GET:", "POST:"...
    return hold(4, json{{"root", std::move(root)},
                        {"routeCount", static_cast<int>(s.patterns.size())},
                        {"routes", std::move(routes)},
                        {"note", "trie keys are METHOD:/path, so depth 1 holds the methods"}}
                       .dump());
}

}  // extern "C"
