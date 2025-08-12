#pragma once
#include <functional>
#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#include "utils.h"           // Custom header - potential circular dependency risk
#include <sys/socket.h>      // System header - good
#include <errno.h>           // System header - good  
#include <iostream>          // Standard library - consider if needed in header
#include "RouteRegistry.h"   // Custom header - potential circular dependency risk
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <errno.h>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#endif






class HttpServer {
private:
    int port;
    int socket_fd;
    int client_socket;
    RouteRegistry registry;

public:
    HttpServer();
    HttpServer(int port);
    void GET(std::string, std::function<void(HttpRequest&, HttpResponse&)> handler);
    void DELETE(std::string, std::function<void(HttpRequest&, HttpResponse&)> handler);
    void PUT(std::string, std::function<void(HttpRequest&, HttpResponse&)> handler);
    void POST(std::string, std::function<void(HttpRequest&, HttpResponse&)> handler);
    void serve();
    void serve(std::function<void()> handler);
    void serve(int port, std::function<void()> handler);




    void handleClient();
    

};
