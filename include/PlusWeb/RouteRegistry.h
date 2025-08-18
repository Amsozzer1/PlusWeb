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
        std::function<void(HttpRequest&, HttpResponse&)> getHandler(HttpRequest&);

        // getMiddlewares
        std::vector<MiddlewareFunction> getMiddleWares();
};

#endif // ROUTEREGISTRY_H