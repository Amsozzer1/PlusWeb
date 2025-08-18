#pragma once
#include "RoutingBase.h"
#include "trie.h"
#include <string>
#include <vector>

class Router : public RoutingBase {
private:
    std::string mountPath;  // Path where this router is mounted
    Trie trie;             // Your existing trie for additional routing logic
    
public:
    Router();
    Router(const std::string& basePath);
    ~Router();
    
    // Router-specific functionality
    void setMountPath(const std::string& path) { mountPath = path; }
    std::string getMountPath() const { return mountPath; }
    
    // Get all middleware/routes from this router (for mounting on server)
    std::vector<MiddlewareFunction> getMiddlewares() const;
    
    // Check if a path matches this router's mount path
    bool matches(const std::string& path) const;
    
    // Get the relative path (strip mount path prefix)
    std::string getRelativePath(const std::string& fullPath) const;
    
    // Access to registry for server integration
    RouteRegistry& getRegistry() { return registry; }
    const RouteRegistry& getRegistry() const { return registry; }
    
    // Router mounting (for sub-routers)
    using RoutingBase::use;
    void use(Router& subRouter);
    void use(const std::string& path, Router& subRouter);
    
    // Express.js-style route method for method chaining
    // TODO: Implement Route class for chaining
    // Route route(const std::string& path);
    
    // Express.js-style param handling
    // TODO: Implement parameter preprocessing
    // void param(const std::string& paramName, 
    //           std::function<void(HttpRequest&, HttpResponse&, NextFunction, const std::string&)> handler);

private:
    // Helper methods for path manipulation
    std::string normalizePath(const std::string& path) const;
    bool pathStartsWith(const std::string& fullPath, const std::string& prefix) const;
};