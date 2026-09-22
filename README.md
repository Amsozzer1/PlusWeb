# PlusWeb

[![CI](https://github.com/Amsozzer1/PlusWeb/actions/workflows/ci.yml/badge.svg)](https://github.com/Amsozzer1/PlusWeb/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)

An Express-style HTTP framework for C++, built on a libuv event loop, the llhttp
parser, and a routing trie.

**[Try the router in your browser](https://amsozzer.com/projects/plusweb)** — the real trie
compiled to WebAssembly. Or read
**[why I wrote this](https://amsozzer.com/writing/i-could-not-understand-express-so-i-wrote-my-own)**,
including the `std::map` that was making every response 339x slower than it needed to be.

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

> ### Work in progress
>
> Routing, middleware, JSON and HTTP parsing work and are covered by tests. The
> core is fast and the parser handles malformed input safely. But there is no
> TLS, no body size limit, no idle timeout, no cookie parsing, and no static file
> serving. Handlers run on the event loop thread, so a handler that blocks stops
> the server.
>
> Use it to learn how a web framework works, or to serve something you control.
> Do not put it on the open internet yet. The [roadmap](#roadmap) lists what is
> missing.

## Speed

PlusWeb is a lot faster than Express. How much depends on how many routes you
have, because Express matches routes by walking its layer list in registration
order while PlusWeb walks a trie.

Both servers running single threaded on one pinned core, identical routes and
handlers, 32 keep-alive connections:

| Routes registered | PlusWeb | Express | |
| --- | --- | --- | --- |
| 5 | 91,950 rps | 19,236 rps | **4.8x** |
| 1,000 | 90,085 rps | 5,639 rps | **16x** |
| 10,000 | 89,636 rps | 355 rps | **253x** |

PlusWeb stays flat at roughly 90,000 requests per second no matter how big the
route table gets. Express drops by a factor of 50 between its first registered
route and its last once you have 10,000 of them.

The gap is widest on things a real app does. Requests that match nothing (404s,
scanners, bad links) run at 76,568 rps against Express's 393 once 10,000 routes
are registered.

Under load the difference shows up as reliability rather than throughput. Driving
both servers with 512 concurrent connections against a 1,000 route table, Express
left 264 of those connections with no response at all across a 5 second run, and
its slowest request took 4,988 ms. PlusWeb answered every connection in every run
at every concurrency level tested, worst case 20 ms.

It is also smaller: 4.6 MB resident against Express's 92 MB, and it starts fast
enough that boot time does not register.

Express wins on breadth, not speed. It has sessions, cookies, static files, body
parsers, template engines and TLS. PlusWeb has a fast core and a list of things
it cannot do yet.

Where the gap narrows: 100 KB response bodies bring it down to 4.5x, and
connections that close after one request bring it down to 4.1x, because PlusWeb
allocates a parser per connection. Full numbers, including the cases where it
does worst, are in [PROFILING_REPORT.md](PROFILING_REPORT.md).

## Why

I wanted to understand what a web framework actually does between the socket and
the handler, so I wrote one. The interesting parts turned out to be the router
and the parser, and most of the bugs were in the parser.

## Requirements

- A C++17 compiler
- CMake 3.14+
- libuv 1.0+ (`libuv-dev` on Debian, `libuv` on Arch and Homebrew)
- libcurl, for the tests only

llhttp, nlohmann/json and GoogleTest are fetched by CMake if they are not already
installed.

## Build

```bash
cmake -B build
cmake --build build
ctest --test-dir build
```

Builds are `Release` unless you ask for something else, for example
`cmake -B build -DCMAKE_BUILD_TYPE=Debug`.

Run an example from the repo root, so the static file path resolves:

```bash
./build/examples/hello_world     # :3000
./build/examples/rest_api        # :8084, shows routing, middleware, routers, JSON
```

## Use it in your project

However you get it, the consumer side is the same:

```cmake
find_package(PlusWeb CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE PlusWeb::PlusWeb)
```

Install it from source with `cmake --install build`, or from a package manager. PlusWeb is
not in the vcpkg or Conan registries yet, so both come from this repo for now:

```bash
# vcpkg, as an overlay port
vcpkg install plusweb --overlay-ports=PlusWeb/packaging/vcpkg

# Conan, from the recipe in the repo
conan create PlusWeb/packaging/conan --build=missing
```

[`packaging/`](packaging/) has the details, including what changes when these go upstream.
Windows is not supported yet: nothing in the library is POSIX-bound, but it has never been
built there.

## Routing

`GET`, `POST`, `PUT`, `DELETE`, `PATCH`, `OPTIONS`, `HEAD` and `ALL` are
available, each with a lowercase alias like `app.get(...)`. Path parameters use
`:name` and land in `req.params`:

```cpp
app.GET("/users/:id/posts/:postId", [](HttpRequest& req, HttpResponse& res) {
    res.send("user " + req.params["id"] + " post " + req.params["postId"]);
});
```

Literal segments beat parameters, so `/users/new` wins over `/users/:id` when
both are registered. If the literal branch has no handler for the path being
matched, the router backtracks to the parameter branch. Registering
`/users/new/edit` therefore still leaves `/users/new` matching `/users/:id`.

A `GET` route also answers `HEAD` with the same headers and no body.

## Request

| Member | Type | Notes |
| --- | --- | --- |
| `req.method`, `req.path`, `req.protocol` | `std::string` | |
| `req.params` | `map<string, string>` | Bound from `:name` segments |
| `req.query` | `map<string, string>` | Parsed from `?a=b&c=d` |
| `req.headers` | `map<string, string>` | |
| `req.body` | `HttpBody` | `getRaw()`, `getJson()`, `isJson()` |

## Response

`status()` and `setHeader()` chain. `send()` picks the Content-Type from the type
you hand it: `nlohmann::json` becomes `application/json`, a `std::string` becomes
`text/html`, a `std::vector<uint8_t>` becomes `application/octet-stream`.

```cpp
res.status(201).setHeader("X-Powered-By", "PlusWeb").send(json{{"ok", true}});
```

Unmatched routes get a JSON 404: `{"error": "Not Found", "path": ..., "method": ...}`.

## Middleware

Middleware runs before routing, so it can short-circuit a request. Call `next()`
to continue, or respond without calling it to stop the chain:

```cpp
app.use([](HttpRequest& req, HttpResponse& res, NextFunction next) {
    if (req.headers["Authorization"].empty()) {
        res.status(401).send(json{{"error", "Unauthorized"}});
        return;   // chain stops here, the route never runs
    }
    next();
});

app.use("/api", authMiddleware);   // scoped to a path prefix
```

## Routers

Group routes and mount them under a prefix:

```cpp
Router api;
api.GET("/health", [](HttpRequest& req, HttpResponse& res) {
    res.send(json{{"status", "ok"}});
});

app.use("/v1", api);   // serves GET /v1/health
```

Middleware has to be registered on a router before you mount it.

## Serving files

There is no `express.static()` equivalent, so you wire the route yourself:

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

## Shutdown

`serve()` blocks until `stop()` is called. `stop()` is safe to call from another
thread, which is how you unblock it:

```cpp
std::thread t([&] { app.serve(); });
app.stop();
t.join();
```

## How it works

Routes live in a trie keyed by `METHOD:/path/segments`. A lookup costs one step
per path segment instead of a scan over every registered route, and that holds
whether the path matches or not. Parameter nodes get their own pointer rather
than being found by scanning a node's children, which is what keeps 404s cheap.

Requests are parsed by [llhttp](https://github.com/nodejs/llhttp), the parser
Node uses. It does framing as well as parsing, so pipelined requests, chunked
bodies and requests split across packets all work, and malformed input gets a 400
instead of being trusted. Request lines and headers are capped at 16 KiB, past
which the request gets a 431.

One libuv event loop handles every connection. Idle keep-alive clients cost
nothing and the number of concurrent clients is not capped by the CPU count.
Handlers run on the loop thread, so a slow handler blocks everything, the same
rule that applies in Node.

Source layout: `include/PlusWeb/` has the public headers, `src/` the
implementation, `tests/` the unit and integration tests, `examples/` runnable
programs, `bench/` the benchmark harness, and `wasm/` a WebAssembly build of the
router and parser.

## Tests

17 tests. Unit tests for path splitting, plus an integration suite that boots a
real server and drives it over loopback with libcurl.

```bash
ctest --test-dir build --output-on-failure
```

CI runs them on Linux and macOS, then again under AddressSanitizer,
UndefinedBehaviorSanitizer and LeakSanitizer.

The benchmark and profiling harness lives in [`bench/`](bench/) and builds with
`-DPLUSWEB_BUILD_BENCH=ON`. It includes a load generator that reports how many
connections got no response at all, which is the number that matters when a
server is saturated.

## Roadmap

- TLS
- Request body size limits and idle connection timeouts
- Moving slow handlers off the loop thread with `uv_queue_work`
- `express.static()` style directory serving
- Cookie parsing, since `req.cookies` exists but is never populated
- Route-specific middleware, as in `app.get(path, mw, handler)`
- Pooling connection and parser objects, which is what costs PlusWeb most on
  connections that do not keep alive
- Structured logging instead of `std::cerr`

## License

[MIT](LICENSE)
