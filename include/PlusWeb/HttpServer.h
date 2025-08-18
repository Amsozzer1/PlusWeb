#pragma once
#include "Router.h"
#include "RoutingBase.h"
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <errno.h>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>

// HttpServer inherits all routing functionality from RoutingBase
// and adds networking/server-specific functionality
class HttpServer : public RoutingBase {
private:
    int port;
    int socket_fd;
    int client_socket;

public:
    HttpServer();
    HttpServer(int port);
    ~HttpServer();

    // Bring base overloads of use() into scope (avoid name hiding)
    using RoutingBase::use;

    // Server-specific methods
    void serve();
    void serve(std::function<void()> handler);
    void serve(int port, std::function<void()> handler);
    
    // Client handling
    void handleClient();
    
    // Server configuration methods
    void setPort(int port) { this->port = port; }
    int getPort() const { return port; }

    void use(Router& router);
    void use(const std::string& path, Router& router);
    
    // TODO: Express.js-like app settings (stubs for API parity)
    // void set(const std::string& name, const std::string& value);
    // std::string get(const std::string& name);
    
    // TODO: Router mounting functionality
    // void use(Router& router);
    // void use(const std::string& path, Router& router);

private:
    // Networking helper methods
    void initializeSocket();
    void bindSocket();
    void startListening();
    void acceptConnections();
    void cleanup();
};