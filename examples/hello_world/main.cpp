#include <PlusWeb/HttpServer.h>

#include <iostream>

int main() {
    constexpr int port = 3000;
    HttpServer app(port);

    app.GET("/", [](HttpRequest& req, HttpResponse& res) {
        res.status(200).send("Hello World!");
    });

    app.serve([] {
        std::cout << "Listening on http://localhost:" << port << std::endl;
    });
}
