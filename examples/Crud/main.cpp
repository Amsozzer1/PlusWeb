#include "../../include/PlusWeb/HttpServer.h"

int main(int argc, char * argv[]){


    HttpServer app = HttpServer(3000);
    int port = 3000;

    app.GET("/", [](HttpRequest& req, HttpResponse& res) {
        // res.send('Hello World!')
        res.Body.setText("Hello World");
    });

    app.serve(port, [port]() {
        std::cout << "Server listening " << std::endl;
    });
}