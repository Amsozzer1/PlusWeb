# PlusWeb

[![CI](https://github.com/Amsozzer1/PlusWeb/actions/workflows/ci.yml/badge.svg)](https://github.com/Amsozzer1/PlusWeb/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)

An Express-style HTTP framework for modern C++, built on POSIX sockets, a
segment-based routing trie, and a fixed-size thread pool.

```cpp
#include <PlusWeb/HttpServer.h>

int main() {
    HttpServer app(3000);

    app.GET("/users/:id", [](HttpRequest& req, HttpResponse& res) {
        res.status(200).send(nlohmann::json{
            {"id", req.params["id"]},
            {"name", "ada"},
        });
    });

    app.serve([] { std::cout << "listening on :3000\n"; });
}
```

> **Status: early, and a learning project.** The routing, middleware, and JSON
> paths work and are covered by tests, but PlusWeb has not been hardened for
> production: there is no TLS, no request size limit, and no timeout handling.
> See [Roadmap](#roadmap) for what is missing.

## Why

I wanted to understand what a web framework actually does between the socket
and the handler, so I wrote one: HTTP parsing, a trie that matches `/users/:id`
without scanning every route, a middleware chain, and a thread pool to serve
connections concurrently.

## Requirements

- A C++17 compiler
- CMake 3.14+
- libcurl (tests only — the integration suite drives a live server)

nlohmann/json and GoogleTest are fetched automatically by CMake if they are not
already installed.

## Build

```bash
cmake -B build
cmake --build build
ctest --test-dir build
```

Run an example (from the repo root, so the static-file path resolves):

```bash
./build/examples/hello_world     # :3000
./build/examples/rest_api        # :8084 — routing, middleware, routers, JSON
```

## Use it in your project

```cmake
find_package(PlusWeb REQUIRED)
target_link_libraries(my_app PRIVATE PlusWeb::PlusWeb)
```

## API

### Routing

`GET`, `POST`, `PUT`, `DELETE`, `PATCH`, `OPTIONS`, `HEAD`, and `ALL` are
available, each with a lowercase alias (`app.get(...)`). Path parameters are
declared with `:name` and land in `req.params`:

```cpp
app.GET("/users/:id/posts/:postId", [](HttpRequest& req, HttpResponse& res) {
    res.send("user " + req.params["id"] + " post " + req.params["postId"]);
});
```

Literal segments beat parameters, so `/users/new` wins over `/users/:id` when
both are registered.

### Request

| Member | Type | Notes |
| --- | --- | --- |
| `req.method`, `req.path`, `req.protocol` | `std::string` | |
| `req.params` | `map<string, string>` | Bound from `:name` segments |
| `req.query` | `map<string, string>` | Parsed from `?a=b&c=d` |
| `req.headers` | `map<string, string>` | |
| `req.body` | `HttpBody` | `getRaw()`, `getJson()`, `isJson()` |

### Response

`status()` and `setHeader()` chain, and `send()` picks the Content-Type from
the type you hand it — `nlohmann::json` becomes `application/json`, a
`std::string` becomes `text/html`, a `std::vector<uint8_t>` becomes
`application/octet-stream`.

```cpp
res.status(201).setHeader("X-Powered-By", "PlusWeb").send(json{{"ok", true}});
```

Unmatched routes get a JSON 404: `{"error": "Not Found", "path": ..., "method": ...}`.

### Middleware

Middleware runs before routing, so it can short-circuit a request. Call `next()`
to continue, or respond without calling it to stop the chain:

```cpp
app.use([](HttpRequest& req, HttpResponse& res, NextFunction next) {
    if (req.headers["Authorization"].empty()) {
        res.status(401).send(json{{"error", "Unauthorized"}});
        return;   // chain stops here; the route never runs
    }
    next();
});

app.use("/api", authMiddleware);   // scoped to a path prefix
```

### Routers

Group routes and mount them under a prefix:

```cpp
Router api;
api.GET("/health", [](HttpRequest& req, HttpResponse& res) {
    res.send(json{{"status", "ok"}});
});

app.use("/v1", api);   // -> GET /v1/health
```

### Serving files

```cpp
app.GET("/logo.png", [](HttpRequest& req, HttpResponse& res) {
    std::vector<uint8_t> bytes;
    if (!Utils::readFile("public/logo.png", bytes)) {
        res.status(404).send(json{{"error", "Not Found"}});
        return;
    }
    res.status(200).send(bytes).setHeader("Content-Type", Utils::mimeTypeFor("logo.png"));
});
```

There is no `express.static()` equivalent yet — you wire the route yourself.

### Shutdown

`serve()` blocks until `stop()` is called. `stop()` is safe to call from another
thread, which is how you unblock it:

```cpp
std::thread t([&] { app.serve(); });
app.stop();
t.join();
```

## How it works

- **Routing** — routes are stored in a trie keyed by `METHOD:/path/segments`, so
  a lookup costs one step per path segment rather than a scan over every
  registered route. Parameter nodes (`:id`) match any single segment and bind it.
- **Concurrency** — one thread accepts connections and hands each socket to a
  thread pool (capped at 16 workers). Connections are keep-alive by default.
- **Layout** — `include/PlusWeb/` public headers, `src/` implementation,
  `tests/` unit + integration tests, `examples/` runnable programs.

## Tests

17 tests: unit tests for path splitting, plus an integration suite that boots a
real server and drives it over the loopback interface with libcurl.

```bash
ctest --test-dir build --output-on-failure
```

CI runs these on Linux and macOS, and again under AddressSanitizer,
UndefinedBehaviorSanitizer, and LeakSanitizer.

## Roadmap

Not yet implemented:

- TLS/HTTPS
- Request size limits and connection timeouts — a slow or oversized request can
  currently tie up a worker
- Request bodies larger than the 1 KB read buffer
- `express.static()`-style directory serving
- Cookie parsing (`req.cookies` exists but is never populated)
- Route-specific middleware (`app.get(path, mw, handler)`)
- Structured logging instead of `std::cerr`

## License

[MIT](LICENSE)
