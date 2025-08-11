#include "include/PlusWeb/HttpServer.h"
#include "include/PlusWeb/utils.h"

int main() {
    HttpServer server = HttpServer(8081);
    
    server.GET("/me", [](HttpRequest& req, HttpResponse& res) {
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
        res.body = "{\"name\":\"ahmed\",\"id\":\"id\"}";
        res.status(200).headers["Content-Type"] = "application/json";
    });
    server.GET("/users/:1", [](HttpRequest& req, HttpResponse& res) {
        res.body = "{\"name\":\"ahmed\",\"id\":\"2\"}";
        res.status(200).headers["Content-Type"] = "application/json";
    });
    
    
    while (true) {
        server.handleClient();
    }

    return 0;
}
