#include "include/PlusWeb/HttpServer.h"
#include "include/PlusWeb/utils.h"


int main() {

    HttpServer server = HttpServer(8081);
    
    server.GET("/", [](HttpRequest& req, HttpResponse& res) {
        res.body = "<html><body>HELLO</body></html>";
        res.status(200).headers["Content-Type"] = "text/html";
    });

    server.GET("/me/:id", [](HttpRequest& req, HttpResponse& res) {
        res.body = "{\"name\":\"ahmed\"}";
        res.status(200).headers["Content-Type"] = "application/json";

    });
    server.GET("/users", [](HttpRequest& req, HttpResponse& res) {
        res.body = "[{\"name\":\"sozzer\"},{\"name\":\"sozzer\"}]";
        res.status(200).headers["Content-Type"] = "application/json";
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
        res.body = "{\"name\":\"ahmed\",\"id\":\"id\"}";
        res.status(200).headers["Content-Type"] = "application/json";
    });



    server.GET("/names/:name", [](HttpRequest& req, HttpResponse& res) {
        std::string name = req.params["name"];
        
        res.body = "<html><body>"+name+"</body></html>";
        res.status(200).headers["Content-Type"] = "text/html";
    });
    
    
    while (true) {
        server.handleClient();
    }

    return 0;
}
