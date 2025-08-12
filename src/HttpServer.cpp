#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <errno.h>
#include <cstring>
#include "../include/PlusWeb/HttpServer.h"
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include "../include/PlusWeb/utils.h"
#include <curl/curl.h>


HttpServer::HttpServer(int port){
    this->port = port;
    this->socket_fd = socket(AF_INET, SOCK_STREAM, 0); 
    this->registry = RouteRegistry();

    // this->registry = RouteRegistry::RouteRegistry();

    if (this->socket_fd == -1) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        exit(1);
    }
    
    // Socket options 
    int opt = 1;
    setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // allow reuse of local addresses
    setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)); // allow reuse of local ports
    setsockopt(this->socket_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)); // enable keep-alive packets

    
    int bufsize = 65536; // 64KB size for I/O buffer
    setsockopt(this->socket_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    setsockopt(this->socket_fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(this->port);

    socklen_t addrlen = sizeof(server_address);

    
    // bind connection to the port
    if (bind(this->socket_fd, (struct sockaddr *)&server_address, addrlen) < 0){
        std::cerr << "error binding: " << errno << std::endl;
        exit(1);
    }

    // listen 
    if(listen(this->socket_fd, 128) < 0){
        std::cerr << "error listening: " << errno << std::endl;
        exit(1);
    }

    std::cout << "Server ready on port " << port << std::endl;
    
}




void HttpServer::handleClient(){
    // this->registry.trie.printTrie();

    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
    
    std::cout << "Waiting for client..." << std::endl;
    this->client_socket = accept(this->socket_fd, (struct sockaddr *)&client_address, &client_len);
    
    if(this->client_socket < 0) {
        std::cerr << "Accept failed: " << strerror(errno) << std::endl;
        return;
    }
    
    std::cout << "Client connected!" << std::endl << std::endl << std::endl;
    
    char buffer[1024] = {0};
    ssize_t bytes_received = recv(this->client_socket, buffer, 1023, 0);
    if (bytes_received <= 0) {
        std::cerr << "No data received or error occurred." << std::endl;
        close(this->client_socket);
        return;
    }


    std::vector<std::string> parts = Utils::split(buffer, "\r\n\r\n");
    
    HttpRequest request = Utils::headerExtractor(parts[0]);




    HttpResponse response = HttpResponse();

    // Body does exist
    if(parts[1]!="")
    {
        if(request.headers["Content-Type"] == "application/json") {

            request.body.setJson(nlohmann::json::parse(parts[1]));
            //  = nlohmann::json::parse(parts[1]).dump();
        }
        else{
            request.body.setText(parts[1]);
        }
    }
    request.printRequestInfo();


    // Call the registered handler for the request path, if it exists
    auto handler = this->registry.getHandler(request);
    // if(false){
    if (handler!=nullptr) {
        handler(request, response);
        response.protocol = "HTTP/1.1";


    } else {
        // response.body = "<html><body>Not Found</body></html>";
        response.$status = 404;
        response.protocol = "HTTP/1.1";
        response.headers["Content-Type"] = "text/html";
    }
    response.headers["Connection"] = "Close";
    response.headers["Content-Length"] = std::to_string(response.Body.length());



    if (send(this->client_socket,response.prepareResponse().c_str(), strlen(response.prepareResponse().c_str()), 0) < 0){
        std::cerr << "Response couldn't be sent " << std::endl;
        close(this->client_socket);
        return;
    }

    
    if(request.headers["Connection"]=="Close") close(this->client_socket);
};

void HttpServer::GET(std::string path, std::function<void(HttpRequest&, HttpResponse&)> handler)
{

    this->registry.Register("GET", path,  handler);
    
}

void HttpServer::DELETE(std::string path, std::function<void(HttpRequest&, HttpResponse&)> handler)
{

    this->registry.Register("DELETE", path,  handler);
    
}

void HttpServer::PUT(std::string path, std::function<void(HttpRequest&, HttpResponse&)> handler)
{

    this->registry.Register("PUT", path,  handler);
    
}
void HttpServer::POST(std::string path, std::function<void(HttpRequest&, HttpResponse&)> handler)
{

    this->registry.Register("POST", path,  handler);
    
}