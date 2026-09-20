#include <PlusWeb/HttpServer.h>

#include "HttpParser.h"

#include <uv.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <system_error>

namespace {

[[noreturn]] void throwUvError(int err, const std::string& what) {
    // libuv errors are negative errno values, so system_error appends the
    // matching text itself.
    throw std::system_error(-err, std::generic_category(), what);
}

struct WriteReq {
    uv_write_t req;
    std::string payload;
};

}  // namespace

// Nested in HttpServer so the callbacks can reach its private members.
struct HttpServer::Loop {
    uv_loop_t loop;
    uv_tcp_t server;
    uv_async_t stopper;   // the only thread-safe way to poke a running loop
    HttpServer* self = nullptr;

    struct Conn {
        uv_tcp_t handle;
        HttpServer* self;
        HttpParser parser;   // one parser per connection; it owns the framing
        bool closing = false;
    };

    static void onAlloc(uv_handle_t*, size_t suggested, uv_buf_t* buf) {
        buf->base = static_cast<char*>(malloc(suggested));
        buf->len = buf->base ? suggested : 0;
    }

    static void onConnClosed(uv_handle_t* handle) {
        delete static_cast<Conn*>(handle->data);
    }

    static void closeConn(Conn* c) {
        if (c->closing) {
            return;
        }
        c->closing = true;
        uv_close(reinterpret_cast<uv_handle_t*>(&c->handle), onConnClosed);
    }

    static void onWrite(uv_write_t* req, int status) {
        auto* wr = reinterpret_cast<WriteReq*>(req->data);
        if (status < 0) {
            closeConn(static_cast<Conn*>(req->handle->data));
        }
        delete wr;
    }

    static void send(uv_stream_t* stream, std::string payload) {
        auto* wr = new WriteReq{{}, std::move(payload)};
        wr->req.data = wr;
        uv_buf_t out = uv_buf_init(const_cast<char*>(wr->payload.data()),
                                   static_cast<unsigned int>(wr->payload.size()));
        if (uv_write(&wr->req, stream, &out, 1, onWrite) != 0) {
            delete wr;
            closeConn(static_cast<Conn*>(stream->data));
        }
    }

    static void onRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
        auto* c = static_cast<Conn*>(stream->data);
        if (nread < 0) {   // peer hung up, or read error
            free(buf->base);
            closeConn(c);
            return;
        }

        // llhttp calls back once per complete request, so pipelined requests and
        // requests split across packets both work without any buffering here.
        bool shouldClose = false;
        const bool ok = c->parser.execute(
            buf->base, static_cast<size_t>(nread),
            [&](HttpRequest& request, bool keepAlive) {
                send(stream, c->self->handleRequest(request, keepAlive, shouldClose));
            });
        free(buf->base);

        if (!ok) {
            std::cerr << "bad request: " << c->parser.error() << std::endl;
            send(stream, HttpServer::badRequestResponse());
            shouldClose = true;
        }
        if (shouldClose) {
            closeConn(c);
        }
    }

    static void onConnection(uv_stream_t* server, int status) {
        auto* L = static_cast<Loop*>(server->data);
        if (status < 0) {
            std::cerr << "accept failed: " << uv_strerror(status) << std::endl;
            return;
        }

        auto* c = new Conn();
        c->self = L->self;
        uv_tcp_init(&L->loop, &c->handle);
        c->handle.data = c;

        if (uv_accept(server, reinterpret_cast<uv_stream_t*>(&c->handle)) != 0) {
            closeConn(c);
            return;
        }
        uv_tcp_nodelay(&c->handle, 1);
        uv_read_start(reinterpret_cast<uv_stream_t*>(&c->handle), onAlloc, onRead);
    }

    // Closes the listener, the async handle and every live connection; once
    // they are all closed uv_run() has nothing left to do and returns.
    static void onStop(uv_async_t* async) {
        auto* L = static_cast<Loop*>(async->data);
        uv_walk(&L->loop,
                [](uv_handle_t* h, void* arg) {
                    auto* L = static_cast<Loop*>(arg);
                    if (uv_is_closing(h)) {
                        return;
                    }
                    if (h == reinterpret_cast<uv_handle_t*>(&L->server) ||
                        h == reinterpret_cast<uv_handle_t*>(&L->stopper)) {
                        uv_close(h, nullptr);
                    } else {
                        closeConn(static_cast<Conn*>(h->data));
                    }
                },
                L);
    }
};

HttpServer::HttpServer(int port) : loop(new Loop()), port(port) {
    loop->self = this;

    int rc = uv_loop_init(&loop->loop);
    if (rc != 0) {
        throwUvError(rc, "failed to create event loop");
    }

    uv_tcp_init(&loop->loop, &loop->server);
    loop->server.data = loop.get();

    uv_async_init(&loop->loop, &loop->stopper, Loop::onStop);
    loop->stopper.data = loop.get();

    struct sockaddr_in address;
    uv_ip4_addr("0.0.0.0", port, &address);

    rc = uv_tcp_bind(&loop->server, reinterpret_cast<const struct sockaddr*>(&address), 0);
    if (rc != 0) {
        throwUvError(rc, "failed to bind port " + std::to_string(port));
    }

    rc = uv_listen(reinterpret_cast<uv_stream_t*>(&loop->server), 128, Loop::onConnection);
    if (rc != 0) {
        throwUvError(rc, "failed to listen");
    }
}

HttpServer::~HttpServer() {
    stop();

    // If serve() was never called, the close callbacks queued by stop() have not
    // run yet. Spin the loop until they have, so uv_loop_close() can succeed.
    for (int i = 0; i < 64 && uv_loop_alive(&loop->loop); ++i) {
        uv_run(&loop->loop, UV_RUN_NOWAIT);
    }
    uv_loop_close(&loop->loop);
}

void HttpServer::serve(std::function<void()> onListening) {
    if (stopRequested) {
        return;
    }
    running = true;

    if (onListening) {
        onListening();
    }

    uv_run(&loop->loop, UV_RUN_DEFAULT);
    running = false;
}

void HttpServer::stop() {
    stopRequested = true;
    // uv_async_send is the one libuv call that is safe from another thread; it
    // wakes the loop, which then closes everything in Loop::onStop.
    uv_async_send(&loop->stopper);
}

std::string HttpServer::badRequestResponse() {
    HttpResponse response;
    response.protocol = "HTTP/1.1";
    response.status(400).send(nlohmann::json{{"error", "Bad Request"}});
    response.headers["Connection"] = "close";
    response.headers["Content-Length"] = std::to_string(response.Body.length());
    return response.prepareResponse();
}

std::string HttpServer::handleRequest(HttpRequest& request, bool keepAlive,
                                      bool& shouldClose) {
    HttpResponse response;
    response.protocol = "HTTP/1.1";

    auto runRoute = [&]() {
        auto handler = registry.getHandler(request);
        if (handler != nullptr) {
            handler(request, response);
            return;
        }
        response.status(404).send(nlohmann::json{
            {"error", "Not Found"},
            {"path", request.path},
            {"method", request.method},
        });
    };

    // Middleware runs first whether or not a route matches, so that a
    // middleware can short-circuit (auth, for example) before routing.
    auto mws = registry.getMiddleWares();
    if (!mws.empty()) {
        executeMiddlewareChain(0, mws, request, response, runRoute);
    } else {
        runRoute();
    }

    shouldClose = !keepAlive;
    response.headers["Connection"] = shouldClose ? "close" : "keep-alive";
    response.headers["Content-Length"] = std::to_string(response.Body.length());
    return response.prepareResponse();
}

void HttpServer::use(Router& router) {
    use("/", router);
}
void HttpServer::use(const std::string& path, Router& router) {
    // Normalize the mount path: "api/" and "/api" both become "/api".
    std::string normalizedPath = path;
    if (normalizedPath.empty() || normalizedPath == "/") {
        normalizedPath = "/";
    } else {
        if (normalizedPath[0] != '/') {
            normalizedPath = "/" + normalizedPath;
        }
        if (normalizedPath.length() > 1 && normalizedPath.back() == '/') {
            normalizedPath.pop_back();
        }
    }

    auto routerMiddleware = [normalizedPath, &router](HttpRequest& req, HttpResponse& res,
                                                      NextFunction next) {
        bool matches = normalizedPath == "/";
        if (!matches) {
            // Match the mount point only on a segment boundary, so that mounting
            // at /api does not capture /apiary.
            matches = req.path.rfind(normalizedPath, 0) == 0 &&
                      (req.path.length() == normalizedPath.length() ||
                       req.path[normalizedPath.length()] == '/');
        }

        if (!matches) {
            next();
            return;
        }

        std::string originalPath = req.path;

        std::string relativePath;
        if (normalizedPath == "/") {
            relativePath = req.path;
        } else {
            relativePath = req.path.substr(normalizedPath.length());
            if (relativePath.empty() || relativePath[0] != '/') {
                relativePath = "/" + relativePath;
            }
        }

        // The router's handlers were registered against paths relative to the
        // mount point, so present them a relative path and restore it after.
        req.path = relativePath;

        auto routerMws = router.getMiddlewares();

        std::function<void(unsigned long)> executeRouterChain = [&](unsigned long index) {
            if (index >= routerMws.size()) {
                req.path = originalPath;
                next();
                return;
            }
            auto routerNext = [&, index]() { executeRouterChain(index + 1); };
            routerMws[index](req, res, routerNext);
        };

        executeRouterChain(0);
    };

    registry.RegisterMiddleWare(routerMiddleware);
}
