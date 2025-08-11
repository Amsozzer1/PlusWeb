#include "../include/PlusWeb/RouteRegistry.h"



RouteRegistry::RouteRegistry(){
    this->trie = Trie();
   
}
void RouteRegistry::Register(std::string method, std::string path, std::function<void(HttpRequest&, HttpResponse&)> handler){
    // static route
    if (path.find(":")==std::string::npos){
        
        this->trie.insert(path, handler);
    }
    else{
        std::cout << "Dynamic Routing found: " << path<<std::endl;
        this->trie.insert(path, handler);
    }


};

std::function<void(HttpRequest&, HttpResponse&)> RouteRegistry::getHandler(std::string path, HttpRequest& req) {
    std::map<std::string, std::string> params;
    
    // Search with parameter extraction
    Node* node = this->trie.searchNode(path, params);
    
    if (node && node->handler) {
        // Populate request object with extracted parameters
        req.params = params;
        return node->handler;
    }
    
    return nullptr;
}
