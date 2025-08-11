#include "trie.h"
#include "utils.h"

class RouteRegistry {
    public:
    Trie trie;

    RouteRegistry();
    void Register(std::string method, std::string path, std::function<void(HttpRequest&, HttpResponse&)> handler);
    
    std::function<void(HttpRequest&, HttpResponse&)> getHandler(std::string path);
    std::function<void(HttpRequest&, HttpResponse&)> getHandler(std::string path, HttpRequest&);









};