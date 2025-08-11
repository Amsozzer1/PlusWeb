#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "utils.h"
#include <sys/socket.h>
#include <errno.h>
#include <iostream>
#include "RouteRegistry.h"


class HttpServer {
private:
    int port;
    int socket_fd;
    int client_socket;
    RouteRegistry registry;

public:
    HttpServer(int port);
    void GET(std::string, std::function<void(HttpRequest&, HttpResponse&)> handler);
    void handleClient();
    

};

#endif