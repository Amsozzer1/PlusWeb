// In-process microbenchmarks for the PlusWeb request pipeline, plus exact
// allocation accounting (global operator new/delete are replaced with counting
// versions, so the numbers are deterministic rather than sampled).
//
// The benchmarks deliberately mirror what HttpServer::processClientConnection
// does per request: split the buffer, parse headers, look up the route, run the
// middleware chain, serialize the response.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>

#include <PlusWeb/HttpRequest.h>
#include <PlusWeb/HttpResponse.h>
#include <PlusWeb/RouteRegistry.h>
#include <PlusWeb/RoutingBase.h>
#include <PlusWeb/utils.h>

#include "HttpParser.h"

// ---------------------------------------------------------------- allocations
namespace alloc {
size_t count = 0;
size_t bytes = 0;
size_t frees = 0;
bool on = false;
inline void reset() { count = 0; bytes = 0; frees = 0; }
}  // namespace alloc

void* operator new(size_t sz) {
    if (alloc::on) { alloc::count++; alloc::bytes += sz; }
    void* p = malloc(sz ? sz : 1);
    if (!p) throw std::bad_alloc();
    return p;
}
void* operator new[](size_t sz) { return operator new(sz); }
void operator delete(void* p) noexcept { if (alloc::on && p) alloc::frees++; free(p); }
void operator delete[](void* p) noexcept { operator delete(p); }
void operator delete(void* p, size_t) noexcept { operator delete(p); }
void operator delete[](void* p, size_t) noexcept { operator delete(p); }

// ---------------------------------------------------------------------- timing
using Clock = std::chrono::steady_clock;
static double ns_since(Clock::time_point t0) {
    return std::chrono::duration<double, std::nano>(Clock::now() - t0).count();
}

struct Stat {
    double ns_per_op;
    double allocs_per_op;
    double bytes_per_op;
};

template <typename F>
Stat measure(const char* /*label*/, long iters, F&& fn) {
    for (long i = 0; i < iters / 10 + 1; i++) fn();  // warm up

    alloc::reset();
    alloc::on = true;
    for (long i = 0; i < iters; i++) fn();
    alloc::on = false;
    double allocs = (double)alloc::count / iters;
    double abytes = (double)alloc::bytes / iters;

    // Time separately so the counters do not pollute the timing.
    double best = 1e18;
    for (int rep = 0; rep < 5; rep++) {
        auto t0 = Clock::now();
        for (long i = 0; i < iters; i++) fn();
        double d = ns_since(t0) / iters;
        best = std::min(best, d);
    }
    return {best, allocs, abytes};
}

static void row(const char* name, const char* param, Stat s) {
    printf("%-34s %-14s %10.1f %12.1f %12.0f\n", name, param, s.ns_per_op,
           s.allocs_per_op, s.bytes_per_op);
}

static void header(const char* title) {
    printf("\n== %s ==\n", title);
    printf("%-34s %-14s %10s %12s %12s\n", "benchmark", "param", "ns/op",
           "allocs/op", "bytes/op");
    printf("%s\n", std::string(86, '-').c_str());
}

// ------------------------------------------------------------------- fixtures
static std::string makePath(int depth) {
    std::string p;
    for (int i = 0; i < depth; i++) p += "/seg" + std::to_string(i);
    return p.empty() ? "/" : p;
}

static void fillRoutes(RouteRegistry& reg, int n, int depth) {
    for (int i = 0; i < n; i++) {
        std::string p = "/r" + std::to_string(i);
        for (int d = 1; d < depth; d++) p += "/seg" + std::to_string(d);
        reg.Register("GET", p, [](HttpRequest&, HttpResponse& res) { res.send("x"); });
    }
}

int main(int argc, char** argv) {
    long iters = argc > 1 ? atol(argv[1]) : 200000;

    // ---- 1. route lookup vs. NUMBER OF REGISTERED ROUTES -------------------
    // The README claims a lookup costs "one step per path segment rather than a
    // scan over every registered route". If true, these rows are flat.
    header("Trie lookup vs route count (fixed path depth 3, hit)");
    for (int n : {10, 100, 1000, 10000}) {
        auto* reg = new RouteRegistry();
        fillRoutes(*reg, n, 3);
        HttpRequest req;
        req.method = "GET";
        req.path = "/r" + std::to_string(n / 2) + "/seg1/seg2";
        Stat s = measure("lookup", iters / 4, [&] {
            auto h = reg->getHandler(req);
            if (!h) abort();
        });
        row("getHandler (hit)", (std::to_string(n) + " routes").c_str(), s);
    }

    // A miss has to fall through the parameter scan at each level, which walks
    // every child of the node - that is where a wide trie could degrade.
    header("Trie lookup vs route count (miss / 404 path)");
    for (int n : {10, 100, 1000, 10000}) {
        auto* reg = new RouteRegistry();
        fillRoutes(*reg, n, 3);
        HttpRequest req;
        req.method = "GET";
        req.path = "/nope/seg1/seg2";
        Stat s = measure("lookup", iters / 4, [&] { reg->getHandler(req); });
        row("getHandler (miss)", (std::to_string(n) + " routes").c_str(), s);
    }

    // ---- 2. route lookup vs PATH DEPTH ------------------------------------
    // "one step per path segment" implies linear in depth. Node::find rebuilds
    // the remaining path string at every level, so watch for quadratic growth.
    header("Trie lookup vs path depth (1000 routes)");
    for (int depth : {1, 2, 4, 8, 16}) {
        auto* reg = new RouteRegistry();
        for (int i = 0; i < 1000; i++) {
            std::string p = "/r" + std::to_string(i) + makePath(depth - 1);
            reg->Register("GET", p, [](HttpRequest&, HttpResponse& r) { r.send("x"); });
        }
        HttpRequest req;
        req.method = "GET";
        req.path = "/r500" + makePath(depth - 1);
        Stat s = measure("lookup", iters / 8, [&] {
            auto h = reg->getHandler(req);
            if (!h) abort();
        });
        row("getHandler (hit)", ("depth " + std::to_string(depth)).c_str(), s);
    }

    // ---- 3. parameterized routes ------------------------------------------
    header("Parameterized vs literal lookup (1000 routes, depth 3)");
    {
        auto* reg = new RouteRegistry();
        fillRoutes(*reg, 1000, 3);
        reg->Register("GET", "/users/:id/posts",
                      [](HttpRequest&, HttpResponse& r) { r.send("x"); });
        HttpRequest lit;
        lit.method = "GET";
        lit.path = "/r500/seg1/seg2";
        row("literal hit", "1000 routes",
            measure("", iters / 4, [&] { reg->getHandler(lit); }));
        HttpRequest par;
        par.method = "GET";
        par.path = "/users/12345/posts";
        row("parameter hit", "1000 routes",
            measure("", iters / 4, [&] { reg->getHandler(par); }));
    }

    // ---- 4. HTTP parsing ---------------------------------------------------
    header("Request parsing (what every request pays)");
    {
        const char* raw =
            "GET /r500/seg1/seg2?a=1&b=2 HTTP/1.1\r\n"
            "Host: localhost:8084\r\n"
            "User-Agent: bench/1.0\r\n"
            "Accept: */*\r\n"
            "Connection: keep-alive\r\n"
            "\r\n";
        row("HttpParser (llhttp)", "5 headers",
            measure("", iters, [&] {
                HttpParser parser;
                bool got = false;
                parser.execute(raw, strlen(raw),
                               [&](HttpRequest& r, bool) { got = !r.method.empty(); });
                if (!got) abort();
            }));

        const char* raw15 =
            "GET /r500/seg1/seg2 HTTP/1.1\r\nHost: h\r\nA: 1\r\nB: 2\r\nC: 3\r\n"
            "D: 4\r\nE: 5\r\nF: 6\r\nG: 7\r\nH: 8\r\nI: 9\r\nJ: 10\r\nK: 11\r\n"
            "L: 12\r\nM: 13\r\nN: 14\r\n\r\n";
        row("HttpParser (llhttp)", "15 headers",
            measure("", iters, [&] {
                HttpParser parser;
                parser.execute(raw15, strlen(raw15), [](HttpRequest&, bool) {});
            }));
    }

    // ---- 5. response serialization ----------------------------------------
    header("Response serialization");
    {
        row("prepareResponse (json, 3 hdrs)", "-", measure("", iters, [&] {
            HttpResponse res;
            res.protocol = "HTTP/1.1";
            res.status(200).send(nlohmann::json{{"id", "42"}, {"name", "ada"}});
            res.headers["Connection"] = "keep-alive";
            res.headers["Content-Length"] = std::to_string(res.Body.length());
            std::string s = res.prepareResponse();
            if (s.empty()) abort();
        }));
        row("prepareResponse (text 1KB)", "-", measure("", iters, [&] {
            HttpResponse res;
            res.protocol = "HTTP/1.1";
            res.status(200).send(std::string(1024, 'x'));
            res.headers["Connection"] = "keep-alive";
            std::string s = res.prepareResponse();
            if (s.empty()) abort();
        }));
        row("HttpResponse construction only", "-", measure("", iters, [&] {
            HttpResponse res;
            res.protocol = "HTTP/1.1";
            (void)res.getResponseMessage(200);
        }));
    }

    // ---- 6. middleware chain ----------------------------------------------
    // getMiddleWares() returns the vector by value, so every request copies
    // every registered std::function.
    header("Middleware chain cost vs middleware count");
    for (int m : {0, 1, 4, 16}) {
        auto* reg = new RouteRegistry();
        fillRoutes(*reg, 100, 3);
        for (int i = 0; i < m; i++) {
            MiddlewareFunction mw = [](HttpRequest&, HttpResponse&, NextFunction next) {
                next();
            };
            reg->RegisterMiddleWare(mw);
        }
        row("getMiddleWares() copy", (std::to_string(m) + " mw").c_str(),
            measure("", iters / 2, [&] {
                auto mws = reg->getMiddleWares();
                if (mws.size() != (size_t)m) abort();
            }));
    }

    // ---- 7. FULL synthetic request cycle -----------------------------------
    header("Full request cycle (parse + route + handler + serialize), no socket");
    for (int nroutes : {10, 1000}) {
        for (int m : {0, 2}) {
            auto* reg = new RouteRegistry();
            fillRoutes(*reg, nroutes, 3);
            for (int i = 0; i < m; i++) {
                MiddlewareFunction mw = [](HttpRequest&, HttpResponse&,
                                           NextFunction next) { next(); };
                reg->RegisterMiddleWare(mw);
            }
            std::string raw = "GET /r" + std::to_string(nroutes / 2) +
                              "/seg1/seg2 HTTP/1.1\r\nHost: localhost:8084\r\n"
                              "User-Agent: bench/1.0\r\nAccept: */*\r\n"
                              "Connection: keep-alive\r\n\r\n";
            char label[64];
            snprintf(label, sizeof(label), "%d routes, %d mw", nroutes, m);
            row("full cycle", label, measure("", iters / 4, [&] {
                HttpParser parser;
                parser.execute(raw.data(), raw.size(), [&](HttpRequest& request, bool) {
                    HttpResponse response;
                    response.protocol = "HTTP/1.1";
                    auto handler = reg->getHandler(request);
                    if (handler) {
                        handler(request, response);
                    } else {
                        response.status(404).send(nlohmann::json{{"error", "Not Found"}});
                    }
                    auto mws = reg->getMiddleWares();
                    response.headers["Connection"] = "keep-alive";
                    response.headers["Content-Length"] =
                        std::to_string(response.Body.length());
                    if (response.prepareResponse().empty()) abort();
                });
            }));
        }
    }

    printf("\n");
    return 0;
}
