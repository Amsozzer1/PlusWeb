#include <PlusWeb/HttpServer.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <system_error>
#include <thread>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

[[noreturn]] void throwSocketError(const std::string& what) {
    throw std::system_error(errno, std::generic_category(), what);
}

}  // namespace

HttpServer::HttpServer(int port)
    : port(port), threadPool(std::thread::hardware_concurrency()) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throwSocketError("failed to create socket");
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

    int bufsize = 65536;  // 64KB I/O buffers
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    struct sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        close(fd);
        throwSocketError("failed to bind port " + std::to_string(port));
    }

    if (listen(fd, 128) < 0) {
        close(fd);
        throwSocketError("failed to listen");
    }

    socket_fd = fd;
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::serve(std::function<void()> onListening) {
    running = true;

    if (onListening) {
        onListening();
    }

    while (running) {
        struct sockaddr_in client_address{};
        socklen_t client_len = sizeof(client_address);

        int client_socket =
            accept(socket_fd, (struct sockaddr*)&client_address, &client_len);

        if (client_socket < 0) {
            // stop() closes the listening socket to break us out of accept().
            if (!running) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "accept failed: " << strerror(errno) << std::endl;
            continue;
        }

        if (activeConnections >= MAX_CONNECTIONS) {
            close(client_socket);
            continue;
        }

        try {
            threadPool.enqueue([this, client_socket]() {
                ++activeConnections;
                processClientConnection(client_socket);
                --activeConnections;
            });
        } catch (const std::exception& e) {
            std::cerr << "failed to enqueue connection: " << e.what() << std::endl;
            close(client_socket);
        }
    }
}

void HttpServer::stop() {
    running = false;

    // Closing the listening socket is what unblocks a thread sitting in accept().
    int fd = socket_fd.exchange(-1);
    if (fd >= 0) {
        ::shutdown(fd, SHUT_RDWR);
        ::close(fd);
    }
}

void HttpServer::processClientConnection(int client_socket) {
    // Keep-alive loop - handle multiple requests on the same connection
    while (true) {
        char buffer[1024] = {0};
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            break;  // peer closed, or error: close the connection
        }

        try {
            std::vector<std::string> parts = Utils::split(buffer, "\r\n\r\n");
            HttpRequest request = Utils::headerExtractor(parts[0]);
            HttpResponse response;
            response.protocol = "HTTP/1.1";

            if (parts.size() > 1 && !parts[1].empty()) {
                if (request.headers["Content-Type"] == "application/json") {
                    request.body.setJson(nlohmann::json::parse(parts[1]));
                } else {
                    request.body.setText(parts[1]);
                }
            }

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

            bool should_close = request.headers["Connection"] == "close" ||
                                request.headers["Connection"] == "Close";
            response.headers["Connection"] = should_close ? "close" : "keep-alive";
            response.headers["Content-Length"] = std::to_string(response.Body.length());

            std::string response_str = response.prepareResponse();
            if (send(client_socket, response_str.c_str(), response_str.length(), 0) < 0) {
                std::cerr << "failed to send response: " << strerror(errno) << std::endl;
                break;
            }

            if (should_close) {
                break;
            }
        } catch (const std::exception& e) {
            std::cerr << "error processing request: " << e.what() << std::endl;
            break;
        }
    }

    close(client_socket);
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
