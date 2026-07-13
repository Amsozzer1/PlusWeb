#include <PlusWeb/RouteRegistry.h>

RouteRegistry::RouteRegistry() = default;

void RouteRegistry::Register(std::string method, std::string path, std::function<void(HttpRequest&, HttpResponse&)> handler){
    this->trie.insert(method+":"+path, handler);
};

std::function<void(HttpRequest&, HttpResponse&)> RouteRegistry::getHandler(HttpRequest& req) const {
    std::map<std::string, std::string> params;
    
    Node* node = this->trie.searchNode(req.method+":"+req.path, params);
    
    if (node && node->handler) {
        req.params = params;
        return node->handler;
    }
    
    return nullptr;
}

std::vector<MiddlewareFunction> RouteRegistry::getMiddleWares() const{
    return this->queue;
};

void RouteRegistry::RegisterMiddleWare(MiddlewareFunction& mw){
    this->queue.push_back(mw);
}

void RouteRegistry::RegisterMiddleWare(const MiddlewareFunction& mw){
    this->queue.push_back(mw);
}

bool RouteRegistry::HandleRequest(HttpRequest& req, HttpResponse& res) const {    
    auto handler = getHandler(const_cast<HttpRequest&>(req));
    if (handler) {
        handler(req, res);
        return true;
    }    
    return false;
}

