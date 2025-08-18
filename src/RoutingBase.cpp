#include "../include/PlusWeb/RoutingBase.h"

void RoutingBase::executeMiddlewareChain(int index, const std::vector<MiddlewareFunction>& mws, 
    HttpRequest& req, HttpResponse& res, std::function<void()> finalHandler) const {
    // Move your existing implementation here from HttpServer.cpp
    // This is just a placeholder - use your actual implementation
    if (index >= mws.size()) {
        finalHandler();
        return;
    }
    
    auto next = [this, index, &mws, &req, &res, finalHandler]() {
        executeMiddlewareChain(index + 1, mws, req, res, finalHandler);
    };
    
    mws[index](req, res, next);
}

void RoutingBase::use(MiddlewareFunction mw) {
    this->registry.RegisterMiddleWare(mw);
}

void RoutingBase::GET(const std::string& path, RouteHandler handler) {
    this->registry.Register("GET", path, handler);
}

void RoutingBase::POST(const std::string& path, RouteHandler handler) {
    this->registry.Register("POST", path, handler);
}

void RoutingBase::PUT(const std::string& path, RouteHandler handler) {
    this->registry.Register("PUT", path, handler);
}

void RoutingBase::DELETE(const std::string& path, RouteHandler handler) {
    this->registry.Register("DELETE", path, handler);
}

void RoutingBase::PATCH(const std::string& path, RouteHandler handler) {
    this->registry.Register("PATCH", path, handler);
}

void RoutingBase::OPTIONS(const std::string& path, RouteHandler handler) {
    this->registry.Register("OPTIONS", path, handler);
}

void RoutingBase::HEAD(const std::string& path, RouteHandler handler) {
    this->registry.Register("HEAD", path, handler);
}

void RoutingBase::ALL(const std::string& path, RouteHandler handler) {
    // Register for all HTTP methods
    this->registry.Register("GET", path, handler);
    this->registry.Register("POST", path, handler);
    this->registry.Register("PUT", path, handler);
    this->registry.Register("DELETE", path, handler);
    this->registry.Register("PATCH", path, handler);
    this->registry.Register("OPTIONS", path, handler);
    this->registry.Register("HEAD", path, handler);
}