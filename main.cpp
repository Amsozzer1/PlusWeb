#include "include/PlusWeb/HttpServer.h"
#include "include/PlusWeb/utils.h"

int main() {




    // Trie trie = Trie();
    // HttpRequest req = HttpRequest();
    // HttpResponse res = HttpResponse();
    // trie.insert("User", [](HttpRequest& req, HttpResponse& res){
    //     std::cout << "User inserted to the Trie";
    // });
    // trie.insert("User", [](HttpRequest& req, HttpResponse& res){
    //     std::cout << "User inserted to the Trie";
    // });
    // trie.root->children["User"]->handler(req,res);
    // trie.root->children[0]->handler(req,res);

    HttpServer server = HttpServer(8081);
    
    server.GET("/me/:id", [](HttpRequest& req, HttpResponse& res) {
        res.body = "{\"name\":\"ahmed\"}";
        res.status(200).headers["Content-Type"] = "application/json";

    });
    server.GET("/users", [](HttpRequest& req, HttpResponse& res) {
        res.body = "[{\"name\":\"sozzer\"},{\"name\":\"sozzer\"}]";
        res.status(200).headers["Content-Type"] = "application/json";
    });
    server.GET("/", [](HttpRequest& req, HttpResponse& res) {
        res.body = "<html><body>HELLO</body></html>";
        res.status(200).headers["Content-Type"] = "text/html";
    });

    server.GET("/users/:id", [](HttpRequest& req, HttpResponse& res) {
        std::string userId = req.params["id"];
        std::cout << "ID: "<<userId <<std::endl;
        res.body = "{\"name\":\"ahmed\",\"id\":\"id\"}";
        res.status(200).headers["Content-Type"] = "application/json";
    });


    
    server.GET("/users/:id/policy/:policyId", [](HttpRequest& req, HttpResponse& res) {
        std::string userId = req.params["id"];
        std::string pId = req.params["policyId"];

        std::cout << "ID: "<<userId <<std::endl;
        std::cout << "pId: "<<pId <<std::endl;

        res.body = "{\"name\":\"ahmed\",\"id\":\"id\"}";
        res.status(200).headers["Content-Type"] = "application/json";
    });
    server.GET("/users/1", [](HttpRequest& req, HttpResponse& res) {
        res.body = "{\"name\":\"ahmed\",\"id\":\"2\"}";
        res.status(200).headers["Content-Type"] = "application/json";
    });
    
    
    while (true) {
        server.handleClient();
    }

    return 0;
}
