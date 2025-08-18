#pragma once
#include "../include/PlusWeb/HttpServer.h"


// #include <curl/curl.h>

    HttpServer::HttpServer(){
        this->port = 8080;
    }
    HttpServer::~HttpServer() = default;
HttpServer::HttpServer(int port){
    this->port = port;
    this->socket_fd = socket(AF_INET, SOCK_STREAM, 0); 
    this->registry = RouteRegistry();

    // this->registry = RouteRegistry::RouteRegistry();

    if (this->socket_fd == -1) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        exit(1);
    }
    
    // Socket options 
    int opt = 1;
    setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // allow reuse of local addresses
    setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)); // allow reuse of local ports
    setsockopt(this->socket_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)); // enable keep-alive packets

    
    int bufsize = 65536; // 64KB size for I/O buffer
    setsockopt(this->socket_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(this->socket_fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(this->port);

    socklen_t addrlen = sizeof(server_address);

    
    // bind connection to the port
    if (bind(this->socket_fd, (struct sockaddr *)&server_address, addrlen) < 0){
        std::cerr << "error binding: " << errno << std::endl;
        exit(1);
    }

    // listen 
    if(listen(this->socket_fd, 128) < 0){
        std::cerr << "error listening: " << errno << std::endl;
        exit(1);
    }
    
}




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

void HttpServer::serve(){
    while(true){
        this->handleClient();
    }

}
void HttpServer::serve(int port, std::function<void()> handler){
    handler();
    this->port = port;
    while(true){
        this->handleClient();
    }
}

void HttpServer::serve(std::function<void()> handler){
    handler();
    while(true){
        this->handleClient();
    }

}
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
            std::function<void(int)> executeRouterChain = [&](int index) {
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
