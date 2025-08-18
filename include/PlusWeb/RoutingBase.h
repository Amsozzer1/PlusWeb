#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RouteRegistry.h"
#include "Types.h"
// #include "router.h"
#include <string>
#include <vector>
#include <functional>

// Base class that provides all routing functionality
// Can be inherited by both HttpServer and Router classes
class RoutingBase {
protected:
    RouteRegistry registry;
    
    void executeMiddlewareChain(int index, const std::vector<MiddlewareFunction>& mws, 
        HttpRequest& req, HttpResponse& res, std::function<void()> finalHandler) const;

public:
    RoutingBase() = default;
    virtual ~RoutingBase() = default;

    // Middleware registration methods
    void use(MiddlewareFunction mw);
    
    template<typename... Arguments>
    void use(MiddlewareFunction arg, Arguments... args) {
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

    // void use(std::string, Router);

    // HTTP verb methods
    void GET(const std::string& path, RouteHandler handler);
    void POST(const std::string& path, RouteHandler handler);
    void PUT(const std::string& path, RouteHandler handler);
    void DELETE(const std::string& path, RouteHandler handler);
    void PATCH(const std::string& path, RouteHandler handler);
    void OPTIONS(const std::string& path, RouteHandler handler);
    void HEAD(const std::string& path, RouteHandler handler);
    void ALL(const std::string& path, RouteHandler handler);

    // Lowercase versions for Express.js compatibility
    void get(const std::string& path, RouteHandler handler) { GET(path, handler); }
    void post(const std::string& path, RouteHandler handler) { POST(path, handler); }
    void put(const std::string& path, RouteHandler handler) { PUT(path, handler); }
    void del(const std::string& path, RouteHandler handler) { DELETE(path, handler); }
    void patch(const std::string& path, RouteHandler handler) { PATCH(path, handler); }
    void options(const std::string& path, RouteHandler handler) { OPTIONS(path, handler); }
    void head(const std::string& path, RouteHandler handler) { HEAD(path, handler); }
    void all(const std::string& path, RouteHandler handler) { ALL(path, handler); }

    // TODO: Future overloads for middleware + handler combinations
    // template<typename... Middlewares>
    // void get(const std::string& path, MiddlewareFunction mw, Middlewares... mws, RouteHandler handler);
    
    // TODO: Express.js-like param method
    // void param(const std::string& paramName, std::function<void(HttpRequest&, HttpResponse&, NextFunction, const std::string&)> handler);
    
    // TODO: Express.js-like route method for chaining
    // Route route(const std::string& path);

protected:
    // Allow derived classes to access the registry
    RouteRegistry& getRegistry() { return registry; }
    const RouteRegistry& getRegistry() const { return registry; }
};