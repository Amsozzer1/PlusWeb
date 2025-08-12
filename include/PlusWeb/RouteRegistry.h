#pragma once
#ifndef ROUTEREGISTRY_H
#define ROUTEREGISTRY_H
#include "trie.h"
#include "utils.h"
#endif

class RouteRegistry {
    public:
    Trie trie;

    RouteRegistry();

    void Register(std::string method, std::string path, std::function<void(HttpRequest&, HttpResponse&)> handler);
    std::function<void(HttpRequest&, HttpResponse&)> getHandler( HttpRequest&);


};