// Server under test. Routes are chosen to exercise the paths the README makes
// claims about: a literal route, a parameterized route, an echo route that
// reports back what the parser produced, and a route that sleeps (to show what
// one slow handler does to the pool).
//
// usage: benchserver <port> [nroutes] [nmiddleware]
#include <PlusWeb/HttpServer.h>

#include <chrono>
#include <csignal>
#include <dlfcn.h>
#include <cstdlib>
#include <string>
#include <thread>

static HttpServer* g_app = nullptr;

int main(int argc, char** argv) {
    int port = argc > 1 ? atoi(argv[1]) : 18080;
    int nroutes = argc > 2 ? atoi(argv[2]) : 100;
    int nmw = argc > 3 ? atoi(argv[3]) : 0;

    static HttpServer app(port);
    g_app = &app;

    for (int i = 0; i < nmw; i++) {
        app.use([](HttpRequest&, HttpResponse&, NextFunction next) { next(); });
    }

    // Filler routes, so lookup cost is measured against a realistic table.
    for (int i = 0; i < nroutes; i++) {
        app.GET("/r" + std::to_string(i) + "/seg1/seg2",
                [](HttpRequest&, HttpResponse& res) { res.send("filler"); });
    }

    app.GET("/plain", [](HttpRequest&, HttpResponse& res) {
        res.status(200).send(std::string("hello"));
    });

    static const std::string kP1k(1024, 'x');
    static const std::string kP10k(10 * 1024, 'x');
    static const std::string kP100k(100 * 1024, 'x');
    app.GET("/p1k", [](HttpRequest&, HttpResponse& res) { res.status(200).send(kP1k); });
    app.GET("/p10k", [](HttpRequest&, HttpResponse& res) { res.status(200).send(kP10k); });
    app.GET("/p100k", [](HttpRequest&, HttpResponse& res) { res.status(200).send(kP100k); });

    app.GET("/json", [](HttpRequest&, HttpResponse& res) {
        res.status(200).send(nlohmann::json{{"ok", true}, {"n", 42}});
    });

    app.GET("/users/:id", [](HttpRequest& req, HttpResponse& res) {
        res.status(200).send(nlohmann::json{{"id", req.params["id"]}});
    });

    // Routes that expose the parser's view of the request, so the probes can
    // assert on what actually got parsed rather than guessing.
    app.GET("/echo", [](HttpRequest& req, HttpResponse& res) {
        nlohmann::json h = nlohmann::json::object();
        for (auto& kv : req.headers) h[kv.first] = kv.second;
        nlohmann::json q = nlohmann::json::object();
        for (auto& kv : req.query) q[kv.first] = kv.second;
        res.status(200).send(nlohmann::json{
            {"method", req.method}, {"path", req.path},
            {"protocol", req.protocol}, {"headers", h}, {"query", q}});
    });

    app.POST("/echobody", [](HttpRequest& req, HttpResponse& res) {
        res.status(200).send(nlohmann::json{
            {"raw_len", req.body.getRaw().size()},
            {"raw_prefix", req.body.getRaw().substr(0, 40)},
            {"is_json", req.body.isJson()},
            {"content_length_header", req.headers.count("Content-Length")
                                          ? req.headers["Content-Length"] : ""}});
    });

    // Backtracking probe: a literal sibling next to a parameter at the same level.
    app.GET("/files/:name", [](HttpRequest& req, HttpResponse& res) {
        res.status(200).send(nlohmann::json{{"matched", "param"},
                                            {"name", req.params["name"]}});
    });
    app.GET("/files/archive/list", [](HttpRequest&, HttpResponse& res) {
        res.status(200).send(nlohmann::json{{"matched", "literal"}});
    });

    app.GET("/slow", [](HttpRequest&, HttpResponse& res) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        res.status(200).send(std::string("slow"));
    });

    // Just break the accept loop; the profiler flush happens below. Calling
    // _exit() here would skip it.
    signal(SIGTERM, [](int) { if (g_app) g_app->stop(); });
    signal(SIGINT, [](int) { if (g_app) g_app->stop(); });

    app.serve([&] { printf("READY %d\n", port); fflush(stdout); });

    // Flush the gperftools profile explicitly. Returning from main would join
    // the pool, and a worker parked in recv() on a keep-alive connection never
    // returns - so stop the profiler and leave immediately.
    if (auto stop_fn = (void (*)())dlsym(RTLD_DEFAULT, "ProfilerStop")) {
        stop_fn();
    }
    fflush(nullptr);
    _exit(0);
}
