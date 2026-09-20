#pragma once
#ifndef ROUTEREGISTRY_H
#define ROUTEREGISTRY_H

#include <vector>
#include <functional>
#include "Types.h"

#include "trie.h"
#include "utils.h"

class RouteRegistry {
    private:
        std::vector<MiddlewareFunction> queue;

    public:
        Trie trie;
        RouteRegistry();
        void Register(std::string method, std::string path, std::function<void(HttpRequest&, HttpResponse&)> handler);
        void RegisterMiddleWare(MiddlewareFunction& mw);
        void RegisterMiddleWare(const MiddlewareFunction& mw);
        std::function<void(HttpRequest&, HttpResponse&)> getHandler(HttpRequest&) const;
        bool HandleRequest(HttpRequest& req, HttpResponse& res) const;

        // Returned by reference: the registry outlives every request that
        // reads it. Registering middleware while a request is in flight would
        // invalidate it, so routes and middleware are set up before serve().
        const std::vector<MiddlewareFunction>& getMiddleWares() const;
};

#endif // ROUTEREGISTRY_H