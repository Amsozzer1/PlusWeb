#include "../include/PlusWeb/HttpResponse.h"



HttpResponse::HttpResponse(){

}


std::string HttpResponse::prepareResponse() {
    std::string response = this->protocol + " " + std::to_string(this->status) + " " + getResponseMessage(this->status) + "\r\n";
    
    // Use the actual headers map
    for (const auto& header : this->headers) {
        response += header.first + ": " + header.second + "\r\n";
    }
    
    response += "\r\n" + body;
    return response;
}

void HttpResponse::updateResponseCode(int code, std::string message){
    ResponseCodes[code] = message;
}

std::string HttpResponse::getResponseMessage(int status){
    if(this->ResponseCodes.count(status)) {
        return this->ResponseCodes[status];
    }
    return "An error occured";
}

