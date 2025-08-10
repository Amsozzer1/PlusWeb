#include "include/PlusWeb/HttpServer.h"
int main() {
    HttpServer server = HttpServer(8081);
    while (true) {
        server.handleClient();
    }
    // server.GET("/me");
    std::cout << "Ahmed" << std::endl;
    return 0;
}
