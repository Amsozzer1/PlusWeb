#pragma once
#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#include "utils.h"           // Custom header - potential circular dependency risk
#include <sys/socket.h>      // System header - good
#include <errno.h>           // System header - good  
#include <iostream>          // Standard library - consider if needed in header
#include "RouteRegistry.h"   // Custom header - potential circular dependency risk
#endif






class HttpServer {
private:
    int port;
    int socket_fd;
    int client_socket;
    RouteRegistry registry;

public:
    HttpServer(int port);
    void GET(std::string, std::function<void(HttpRequest&, HttpResponse&)> handler);
    void DELETE(std::string, std::function<void(HttpRequest&, HttpResponse&)> handler);
    void PUT(std::string, std::function<void(HttpRequest&, HttpResponse&)> handler);
    void POST(std::string, std::function<void(HttpRequest&, HttpResponse&)> handler);


    void handleClient();
    

};
