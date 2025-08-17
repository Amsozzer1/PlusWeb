#pragma once
#include "RouteRegistry.h"
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
