#ifndef MYHEADER_H
#define MYHEADER_H
#include "include/PlusWeb/HttpRequest.h"
#include "include/PlusWeb/HttpResponse.h"
#include "include/PlusWeb/HttpServer.h"
#endif


int main() {
      

    HttpServer server(8081);
    server.GET("/", [](HttpRequest& req, HttpResponse& res) {
        // res.body = "<html><body>HELLO</body></html>";
        res.status(200).headers["Content-Type"] = "text/html";
    });

    server.GET("/me/:id", [](HttpRequest& req, HttpResponse& res) {
        // res.body = "{\"name\":\"ahmed\"}";
        res.status(200).headers["Content-Type"] = "application/json";

    });
    server.GET("/users", [](HttpRequest& req, HttpResponse& res) {
        // res.body = "[{\"name\":\"sozzer\"},{\"name\":\"sozzer\"}]";
        res.status(200).headers["Content-Type"] = "application/json";
    });
    

    server.GET("/users/:id", [](HttpRequest& req, HttpResponse& res) {
        std::string userId = req.params["id"];
        std::cout << "ID: "<<userId <<std::endl;
        // res.body = "{\"name\":\"ahmed\",\"id\":\"id\"}";
        res.status(200).headers["Content-Type"] = "application/json";
    });

    
    server.GET("/users/:id/policy/:policyId", [](HttpRequest& req, HttpResponse& res) {
        std::string userId = req.params["id"];
        std::string pId = req.params["policyId"];
        // res.body = "{\"name\":\"ahmed\",\"id\":\"id\"}";
        res.status(200).headers["Content-Type"] = "application/json";
    });



    server.GET("/names/:name", [](HttpRequest& req, HttpResponse& res) {
        std::string name = req.params["name"];        
        // res.body = "<html><body>"+name+"</body></html>";
        // res.body = "<html><body>"+req.method+"</body></html>";
        res.status(200).headers["Content-Type"] = "text/html";
    });
    server.DELETE("/names/:name", [](HttpRequest& req, HttpResponse& res) {
        // const {abc, def} = req.body();
        std::string name = req.params["name"];        
        std::cout << req.body.getRaw() << std::endl;
        nlohmann::json ex3 = {
            {"id", name},
            {"data", "Some Data here"},
        };
        res.Body.setJson(ex3["id"]);

        res.status(200).headers["Content-Type"] = "application/json";
    });
    
    server.GET("/users/:id/data", [](HttpRequest& req, HttpResponse& res) {
        std::string userId = req.params["id"];
        nlohmann::json ex3 = {
            {"id", userId},
            {"data", "Some Data here"},
        };
        // res.body = ex3;
        res.status(200).headers["Content-Type"] = "application/json";
    });
    
    server.POST("/users/:id",[](HttpRequest& req, HttpResponse& res){

        res.status(200).headers["Content-Type"] = "application/json";
        std:: cout << req.body.getJson() << std::endl;
        std:: cout << req.body.getJson()["abc"] << std::endl;


        res.Body.setJson(nlohmann::json{
            {"id", 1},
            {"data", "Some Data here"},
        });
    });



    while (true) {
        server.handleClient();
    }

    return 0;
}
