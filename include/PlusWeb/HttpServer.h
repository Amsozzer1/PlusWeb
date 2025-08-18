#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RouteRegistry.h"
#include "Types.h"
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
#include "Types.h"
// See Types.h for RouteHandler / NextFunction / MiddlewareFunction






class HttpServer {
private:
    int port;
    int socket_fd;
    int client_socket;
    RouteRegistry registry;
    void executeMiddlewareChain(int index, const std::vector<MiddlewareFunction>& mws, 
        HttpRequest& req, HttpResponse& res, std::function<void()> finalHandler);
    
   

public:
    HttpServer();
    HttpServer(int port);

    
    

    // TODO: Next functions for errors
    // using NextFunctionError = std::function<void(std::string)>;



    void use(MiddlewareFunction mw);
    template< typename... Arguments >
    void use( MiddlewareFunction arg, Arguments ... args ){
        // Register the first middleware
        this->registry.RegisterMiddleWare(arg);
        
        // If there are more arguments, recursively process them
        if constexpr (sizeof...(args) > 0) {
            use(args...);
        }
    }
    template<typename... Arguments>
    void use(const std::string& path, MiddlewareFunction arg, Arguments... args) {
        auto pathWrapper = [path](MiddlewareFunction mw) {
            return [path, mw](HttpRequest& req, HttpResponse& res, NextFunction next) {
                if (req.path.rfind(path, 0) == 0) {
                    mw(req, res, next);
                } else {
                    next();
                }
            };
        };
        
        this->registry.RegisterMiddleWare(pathWrapper(arg));
        if constexpr (sizeof...(args) > 0) {
            (this->registry.RegisterMiddleWare(pathWrapper(args)), ...);
        }
    }
    
    void GET(std::string, RouteHandler handler);
    // template<typename... Handlers>
    // void GET(std::string path, Handlers&&... handlers) {
    //     static_assert(sizeof...(handlers) > 0, "At least one handler required");
        
    //     // Convert all to RouteHandler and register
    //     (this->registry.Register("GET", path, RouteHandler{std::forward<Handlers>(handlers)}), ...);
    // }

    void DELETE(std::string, RouteHandler handler);
    void PUT(std::string, RouteHandler handler);
    void POST(std::string, RouteHandler handler);



    void serve();
    void serve(std::function<void()> handler);
    void serve(int port, std::function<void()> handler);




    void handleClient();
    

};
