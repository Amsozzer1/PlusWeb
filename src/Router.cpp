#include "../include/PlusWeb/Router.h"
#include <algorithm>

Router::Router() : mountPath("") {
}

Router::Router(const std::string& basePath) : mountPath(normalizePath(basePath)) {
}

Router::~Router() = default;

std::vector<MiddlewareFunction> Router::getMiddlewares() const{
    std::vector<MiddlewareFunction> middlewares;
    
    // Get the router's own middleware
    auto routerOwnMiddlewares = this->registry.getMiddleWares();
    
    // Add each router middleware to the list
    for (const auto& mw : routerOwnMiddlewares) {
        middlewares.push_back(mw);
    }
    
    // Create a final middleware that handles route matching
    auto routeHandlerMiddleware = [this](HttpRequest& req, HttpResponse& res, NextFunction next) {
        // Try to handle routes in this router's registry
        if (this->registry.HandleRequest(req, res)) {
            // Route was handled successfully, don't call next()
            return;
        }
        
        // No route matched, continue to next middleware
        next();
    };
    
    middlewares.push_back(routeHandlerMiddleware);
    return middlewares;
}

bool Router::matches(const std::string& path) const {
    if (mountPath.empty()) {
        return true; // Root router matches all paths
    }
    return pathStartsWith(path, mountPath);
}

std::string Router::getRelativePath(const std::string& fullPath) const {
    if (mountPath.empty()) {
        return fullPath;
    }
    
    if (pathStartsWith(fullPath, mountPath)) {
        std::string relative = fullPath.substr(mountPath.length());
        return relative.empty() ? "/" : relative;
    }
    
    return fullPath;
}

void Router::use(Router& subRouter) {
    // Mount sub-router at root of this router
    use("/", subRouter);
}

void Router::use(const std::string& path, Router& subRouter) {
    std::string normalizedPath = normalizePath(path);
    subRouter.setMountPath(mountPath + normalizedPath);
    
    // Create a middleware that forwards requests to the sub-router
    auto subRouterMiddleware = [normalizedPath, &subRouter](
        HttpRequest& req, HttpResponse& res, NextFunction next) {
        
        // Check if the request path matches the sub-router's mount path
        if (req.path.rfind(normalizedPath, 0) == 0) {
            // Store original path
            std::string originalPath = req.path;
            
            // Create relative path for sub-router
            std::string relativePath = req.path.substr(normalizedPath.length());
            if (relativePath.empty() || relativePath[0] != '/') {
                relativePath = "/" + relativePath;
            }
            
            // Temporarily modify request path
            req.path = relativePath;
            
            // Try to handle with sub-router
            if (subRouter.registry.HandleRequest(req, res)) {
                // Request was handled, restore path and return
                req.path = originalPath;
                return;
            }
            
            // Restore original path and continue
            req.path = originalPath;
        }
        
        // Path didn't match or wasn't handled, continue to next middleware
        next();
    };
    
    this->registry.RegisterMiddleWare(subRouterMiddleware);
}

std::string Router::normalizePath(const std::string& path) const {
    if (path.empty() || path == "/") {
        return "/";
    }
    
    std::string normalized = path;
    
    // Ensure path starts with /
    if (normalized[0] != '/') {
        normalized = "/" + normalized;
    }
    
    // Remove trailing slash unless it's root
    if (normalized.length() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }
    
    return normalized;
}

bool Router::pathStartsWith(const std::string& fullPath, const std::string& prefix) const {
    if (prefix == "/" || prefix.empty()) {
        return true;
    }
    
    return fullPath.rfind(prefix, 0) == 0 && 
           (fullPath.length() == prefix.length() || fullPath[prefix.length()] == '/');
}