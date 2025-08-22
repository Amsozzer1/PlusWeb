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
#include "ThreadPool.h"
#include <fstream>
#include <iterator>
#include <vector>

// HttpServer inherits all routing functionality from RoutingBase
// and adds networking/server-specific functionality
class HttpServer : public RoutingBase {
private:
    int port;
    int socket_fd;
    int client_socket;
    ThreadPool threadPool;  // Add this member
    std::atomic<int> activeConnections{0};  // Track connections
    static const int MAX_CONNECTIONS = 1000;

public:
    HttpServer();
    // HttpServer(int port);
    HttpServer(int port) : threadPool(std::thread::hardware_concurrency()){
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
    };
    ~HttpServer();
    void processClientConnection(int client_socket);

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
    // express.static() implementation:
    

private:
    // Networking helper methods
    void initializeSocket();
    void bindSocket();
    void startListening();
    void acceptConnections();
    void cleanup();
};