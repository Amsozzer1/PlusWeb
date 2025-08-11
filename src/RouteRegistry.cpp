#include "../include/PlusWeb/RouteRegistry.h"



RouteRegistry::RouteRegistry(){
    this->trie = Trie();
   
}
void RouteRegistry::Register(std::string method, std::string path, std::function<void(HttpRequest&, HttpResponse&)> handler){
    
    this->trie.insert(path, handler);
};
std::function<void(HttpRequest&, HttpResponse&)> RouteRegistry::getHandler(std::string path){
    Node* node = this->trie.searchNode(path);
    if(!node) return nullptr;
    return node->handler;
}
