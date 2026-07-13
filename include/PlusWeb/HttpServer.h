#pragma once
#include "Router.h"
#include "RoutingBase.h"
#include "ThreadPool.h"

#include <atomic>
#include <functional>
#include <string>

// HttpServer inherits all routing functionality from RoutingBase and adds the
// listening socket and the accept loop.
class HttpServer : public RoutingBase {
public:
    // Binds and listens on `port`. Throws std::system_error if the socket
    // cannot be created, bound, or listened on.
    explicit HttpServer(int port = 8080);
    ~HttpServer();

    // Holds a socket and a thread pool; copying would double-close the fd.
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    // Accepts connections until stop() is called, dispatching each to the thread
    // pool. `onListening` runs once, just before the first accept.
    void serve(std::function<void()> onListening = {});

    // Stops the accept loop and closes the listening socket. Safe to call from
    // another thread (that is how you unblock serve()), and safe to call twice.
    void stop();

    bool isRunning() const { return running; }
    int getPort() const { return port; }

    // Bring base overloads of use() into scope (avoid name hiding)
    using RoutingBase::use;
    void use(Router& router);
    void use(const std::string& path, Router& router);

private:
    void processClientConnection(int client_socket);

    int port;
    // Atomic because stop() closes the socket from a different thread than the
    // one blocked in accept().
    std::atomic<int> socket_fd{-1};
    std::atomic<int> activeConnections{0};
    std::atomic<bool> running{false};
    ThreadPool threadPool;

    static constexpr int MAX_CONNECTIONS = 1000;
};
