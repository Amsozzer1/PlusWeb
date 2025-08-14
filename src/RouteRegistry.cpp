#include "../include/PlusWeb/RouteRegistry.h"

RouteRegistry::RouteRegistry(){
    this->trie = Trie();
   
}
void RouteRegistry::Register(std::string method, std::string path, std::function<void(HttpRequest&, HttpResponse&)> handler){
    this->trie.insert(method+":"+path, handler);
};

std::function<void(HttpRequest&, HttpResponse&)> RouteRegistry::getHandler(HttpRequest& req) {
    std::map<std::string, std::string> params;
    
    // Search with parameter extraction
    Node* node = this->trie.searchNode(req.method+":"+req.path, params);
    
    if (node && node->handler) {
        // Populate request object with extracted parameters
        req.params = params;
        return node->handler;
    }
    
    return nullptr;
}
