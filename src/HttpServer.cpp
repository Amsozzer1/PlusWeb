#include <PlusWeb/HttpServer.h>
#include <thread>


// #include <curl/curl.h>

    HttpServer::HttpServer(){
        this->port = 8080;
    }
    HttpServer::~HttpServer() = default;
// HttpServer::HttpServer(int port){
    
    
// }




void HttpServer::handleClient(){
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
    
    this->client_socket = accept(this->socket_fd, (struct sockaddr *)&client_address, &client_len);
    
    if(this->client_socket < 0) {
        std::cerr << "Accept failed: " << strerror(errno) << std::endl;
        return;
    }
    
    // Keep-alive loop - handle multiple requests on same connection
    while(true) {
        char buffer[1024] = {0};
        ssize_t bytes_received = recv(this->client_socket, buffer, 1023, 0);
        
        if (bytes_received <= 0) {
            break; // Exit loop and close connection
        }

        std::vector<std::string> parts = Utils::split(buffer, "\r\n\r\n");
        HttpRequest request = Utils::headerExtractor(parts[0]);
        HttpResponse response = HttpResponse();
        response.protocol = "HTTP/1.1";

        // Body processing...
        if(parts.size() > 1 && parts[1] != "") {
            if(request.headers["Content-Type"] == "application/json") {
                request.body.setJson(nlohmann::json::parse(parts[1]));
            } else {
                request.body.setText(parts[1]);
            }
        }

        // FIXED: Always run middleware first, regardless of whether direct handler exists
        auto mws = this->registry.getMiddleWares();

            if (!mws.empty()) {
                executeMiddlewareChain(0, mws, request, response, [&]() {
                    auto handler = this->registry.getHandler(request);
                    if (handler != nullptr) {
                        handler(request, response);
                    } else {
                        response.status(404).setHeader("Content-Type","text/html")
                            .send("<html><body>Not Found</body></html>");
                    }
                });
            } else {
            // No middleware, check direct routes only
            auto handler = this->registry.getHandler(request);
            if (handler != nullptr) {
                handler(request, response);
            } else {
                response.status(404).setHeader("Content-Type","text/html")
                       .send("<html><body>Not Found</body></html>");
            }
        }

        // Set connection behavior
        bool should_close = false;
        if(request.headers["Connection"] == "close" || 
           request.headers["Connection"] == "Close") {
            response.headers["Connection"] = "close";
            should_close = true;
        } else {
            response.headers["Connection"] = "keep-alive";
        }

        response.headers["Content-Length"] = std::to_string(response.Body.length());

        // Send response
        std::string response_str = response.prepareResponse();
        if (send(this->client_socket, response_str.c_str(), response_str.length(), 0) < 0) {
            std::cerr << "Response couldn't be sent" << std::endl;
            break;
        }

        // Close connection if requested
        if(should_close) {
            std::cout << "Closing connection as requested" << std::endl;
            break;
        }
    }
    
    close(this->client_socket);
}

void HttpServer::serve() {
    std::cout << "Server starting with " << threadPool.getThreadCount() << " worker threads" << std::endl;
    
    while (true) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        
        // Accept new connection
        int client_socket = accept(this->socket_fd, (struct sockaddr*)&client_address, &client_len);
        
        if (client_socket < 0) {
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
            continue;
        }
        
        // Check connection limit
        if (activeConnections >= MAX_CONNECTIONS) {
            std::cout << "Connection limit reached, rejecting connection" << std::endl;
            close(client_socket);
            continue;
        }
        
        // Submit connection handling to thread pool
        try {
            threadPool.enqueue([this, client_socket]() {
                activeConnections++;
                this->processClientConnection(client_socket);
                activeConnections--;
            });
        } catch (const std::exception& e) {
            std::cerr << "Failed to enqueue connection: " << e.what() << std::endl;
            close(client_socket);
        }
    }
}

void HttpServer::processClientConnection(int client_socket) {
    // Keep-alive loop - handle multiple requests on same connection
    while (true) {
        char buffer[1024] = {0};
        ssize_t bytes_received = recv(client_socket, buffer, 1023, 0);
        
        if (bytes_received <= 0) {
            break; // Exit loop and close connection
        }

        try {
            std::vector<std::string> parts = Utils::split(buffer, "\r\n\r\n");
            HttpRequest request = Utils::headerExtractor(parts[0]);
            HttpResponse response = HttpResponse();
            response.protocol = "HTTP/1.1";

            // Body processing...
            if (parts.size() > 1 && parts[1] != "") {
                if (request.headers["Content-Type"] == "application/json") {
                    request.body.setJson(nlohmann::json::parse(parts[1]));
                } else {
                    request.body.setText(parts[1]);
                }
            }

            // Middleware and route handling (existing code)
            auto mws = this->registry.getMiddleWares();
            if (!mws.empty()) {
                executeMiddlewareChain(0, mws, request, response, [&]() {
                    auto handler = this->registry.getHandler(request);
                    if (handler != nullptr) {
                        handler(request, response);
                    } else {
                        response.status(404).setHeader("Content-Type", "text/html")
                            .send("<html><body>Not Found</body></html>");
                    }
                });
            } else {
                auto handler = this->registry.getHandler(request);
                if (handler != nullptr) {
                    handler(request, response);
                } else {
                    response.status(404).setHeader("Content-Type", "text/html")
                           .send("<html><body>Not Found</body></html>");
                }
            }

            // Set connection behavior
            bool should_close = false;
            if (request.headers["Connection"] == "close" || 
               request.headers["Connection"] == "Close") {
                response.headers["Connection"] = "close";
                should_close = true;
            } else {
                response.headers["Connection"] = "keep-alive";
            }

            response.headers["Content-Length"] = std::to_string(response.Body.length());

            // Send response
            std::string response_str = response.prepareResponse();
            if (send(client_socket, response_str.c_str(), response_str.length(), 0) < 0) {
                std::cerr << "Response couldn't be sent" << std::endl;
                break;
            }

            // Close connection if requested
            if (should_close) {
                break;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error processing request: " << e.what() << std::endl;
            break;
        }
    }
    
    close(client_socket);
}
void HttpServer::serve(int port, std::function<void()> handler){
    handler();
    this->port = port;
    while (true) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        
        // Accept new connection
        int client_socket = accept(this->socket_fd, (struct sockaddr*)&client_address, &client_len);
        
        if (client_socket < 0) {
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
            continue;
        }
        
        // Check connection limit
        if (activeConnections >= MAX_CONNECTIONS) {
            std::cout << "Connection limit reached, rejecting connection" << std::endl;
            close(client_socket);
            continue;
        }
        
        // Submit connection handling to thread pool
        try {
            threadPool.enqueue([this, client_socket]() {
                activeConnections++;
                this->processClientConnection(client_socket);
                activeConnections--;
            });
        } catch (const std::exception& e) {
            std::cerr << "Failed to enqueue connection: " << e.what() << std::endl;
            close(client_socket);
        }
    }
}

void HttpServer::serve(std::function<void()> handler){
    handler();
    while (true) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        
        // Accept new connection
        int client_socket = accept(this->socket_fd, (struct sockaddr*)&client_address, &client_len);
        
        if (client_socket < 0) {
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
            continue;
        }
        
        // Check connection limit
        if (activeConnections >= MAX_CONNECTIONS) {
            std::cout << "Connection limit reached, rejecting connection" << std::endl;
            close(client_socket);
            continue;
        }
        
        // Submit connection handling to thread pool
        try {
            threadPool.enqueue([this, client_socket]() {
                activeConnections++;
                this->processClientConnection(client_socket);
                activeConnections--;
            });
        } catch (const std::exception& e) {
            std::cerr << "Failed to enqueue connection: " << e.what() << std::endl;
            close(client_socket);
        }
    }

}
// void HttpServer::serve(){
//     while(true){
//         std::thread clientThread(&HttpServer::handleClient, this);
//         clientThread.detach(); // Let it run independently
//     }
// }
// Add these methods to HttpServer.cpp

void HttpServer::use(Router& router) {
    // Mount router at root path
    use("/", router);
}

void HttpServer::use(const std::string& path, Router& router) {
    auto routerMiddlewares = router.getMiddlewares();

    // Normalize the mount path
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

    auto routerMiddleware = [normalizedPath, &router](HttpRequest& req, HttpResponse& res, NextFunction next) {
        
        // Check if request path matches the mount path
        bool matches = false;
        if (normalizedPath == "/") {
            matches = true;
        } else {
            matches = (req.path.rfind(normalizedPath, 0) == 0) &&
                     (req.path.length() == normalizedPath.length() || 
                      req.path[normalizedPath.length()] == '/');
        }
        
        
        if (matches) {
            // Store original path
            std::string originalPath = req.path;
            
            // Create relative path for router
            std::string relativePath;
            if (normalizedPath == "/") {
                relativePath = req.path;
            } else {
                relativePath = req.path.substr(normalizedPath.length());
                if (relativePath.empty() || relativePath[0] != '/') {
                    relativePath = "/" + relativePath;
                }
            }
            
            
            // Temporarily set relative path
            req.path = relativePath;
            
            // Get router's middlewares and execute them
            auto routerMws = router.getMiddlewares();
            
            // Create a custom executeMiddlewareChain call for the router
            std::function<void(int)> executeRouterChain = [&](unsigned long index) {
                if (index >= routerMws.size()) {
                    // All router middlewares executed, restore path and continue
                    req.path = originalPath;
                    next();
                    return;
                }
                
                auto routerNext = [&, index]() {
                    executeRouterChain(index + 1);
                };
                
                routerMws[index](req, res, routerNext);
            };
            
            executeRouterChain(0);
            
        } else {
            next();
        }
    };

    this->registry.RegisterMiddleWare(routerMiddleware);

    // Check server's middleware count
    auto serverMiddlewares = this->registry.getMiddleWares();
}
