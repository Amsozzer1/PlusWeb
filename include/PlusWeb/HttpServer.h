#pragma once
#include "Router.h"
#include "RoutingBase.h"

#include <atomic>
#include <functional>
#include <memory>
#include <string>

// HttpServer inherits all routing functionality from RoutingBase and adds the
// listening socket and the event loop.
//
// Handlers run on the event-loop thread, so a handler that blocks stops the
// whole server.
class HttpServer : public RoutingBase {
public:
    // Binds and listens on `port`. Throws std::system_error if the socket
    // cannot be created, bound, or listened on.
    explicit HttpServer(int port = 8080);
    ~HttpServer();

    // Owns a loop and a listening socket; copying would double-close them.
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    // Runs the event loop until stop() is called. `onListening` runs once, just
    // before the loop starts.
    void serve(std::function<void()> onListening = {});

    // Stops the loop and closes every open connection. Safe to call from another
    // thread, and safe to call twice.
    void stop();

    bool isRunning() const { return running; }
    int getPort() const { return port; }

    // Bring base overloads of use() into scope (avoid name hiding)
    using RoutingBase::use;
    void use(Router& router);
    void use(const std::string& path, Router& router);

private:
    // Defined in HttpServer.cpp, so uv.h stays out of this header.
    struct Loop;
    std::unique_ptr<Loop> loop;

    // Runs middleware + routing for one request; returns the bytes to write back.
    std::string handleRequest(HttpRequest& request, bool keepAlive, bool& shouldClose);

    static std::string badRequestResponse();

    int port;
    std::atomic<bool> running{false};
    std::atomic<bool> stopRequested{false};
};
