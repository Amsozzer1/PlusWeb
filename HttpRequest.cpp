#include "HttpRequest.h"


HttpRequest::HttpRequest(){

}
HttpRequest::HttpRequest(
    std::string method, 
    std::string path,  
    std::string protocol, 
    std::map<std::string, 
    std::string> headers,  
    std::string body
){
    this->method = method;
    this->path = path;
    this->protocol = protocol;
    this->headers = headers;
    this->body = body;

}

void HttpRequest::printRequestInfo() const {
    std::cout << "Method: " << method << std::endl;
    std::cout << "Path: " << path << std::endl;
    std::cout << "Protocol: " << protocol << std::endl;
    std::cout << "Headers:" << std::endl;
    for (const auto& header : headers) {
        std::cout << "  " << header.first << ": " << header.second << std::endl;
    }
    std::cout << "Body: " << body << std::endl;
}
