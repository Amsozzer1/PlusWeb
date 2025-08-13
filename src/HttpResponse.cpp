#include "../include/PlusWeb/HttpResponse.h"



HttpResponse::HttpResponse(){

}
HttpResponse& HttpResponse::status(int status){
    this->$status = status;
    return *this;  
}

std::string HttpResponse::prepareResponse() {
    std::string response = this->protocol + " " + std::to_string(this->$status) + " " + getResponseMessage(this->$status) + "\r\n";
    
    // Use the actual headers map
    for (const auto& header : this->headers) {
        response += header.first + ": " + header.second + "\r\n";
    }

    if(this->Body.isText()){
        response += "\r\n" + this->Body.getRaw();
    }
    else if (this->Body.isJson()) {
        response +=  "\r\n" + this->Body.getJson().dump();
    
    }
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

HttpResponse& HttpResponse::setHeaders(std::map<std::string,std::string> pairs){
    // for()
    for(auto pair:pairs){
        this->setHeader(pair.first, pair.second);
    }
    return *this;
}
HttpResponse& HttpResponse::setHeader(std::string key ,std::string value){
    this->headers[key] = value;
    return *this;
}

HttpResponse& HttpResponse::send(const HttpBody& body) {
    Body = body;
    // Set Content-Type based on body type
    switch(body.getType()) {
        case HttpBody::JSON:
            setHeader("Content-Type", "application/json");
            break;
        case HttpBody::TEXT:
            setHeader("Content-Type", "text/plain; charset=utf-8");
            break;
        case HttpBody::FORM_DATA:
            setHeader("Content-Type", "application/x-www-form-urlencoded");
            break;
        case HttpBody::MULTIPART:
            setHeader("Content-Type", "multipart/form-data");
            break;
        case HttpBody::BINARY:
            setHeader("Content-Type", "application/octet-stream");
            break;
        case HttpBody::EMPTY:
        default:
            // Don't set Content-Type for empty bodies
            break;
    }
    return *this;
}

HttpResponse& HttpResponse::send(HttpBody&& body) {
    // Same logic as above, but move the body
    auto body_type = body.getType();
    Body = std::move(body);
    
    switch(body_type) {
        case HttpBody::JSON:
            setHeader("Content-Type", "application/json");
            break;
        case HttpBody::TEXT:
            setHeader("Content-Type", "text/plain; charset=utf-8");
            break;
        case HttpBody::FORM_DATA:
            setHeader("Content-Type", "application/x-www-form-urlencoded");
            break;
        case HttpBody::MULTIPART:
            setHeader("Content-Type", "multipart/form-data");
            break;
        case HttpBody::BINARY:
            setHeader("Content-Type", "application/octet-stream");
            break;
    }
    return *this;
}

HttpResponse& HttpResponse::send(const json& j) {
    Body.setJson(j);
    setHeader("Content-Type", "application/json");
    setHeader("Content-Length", std::to_string(Body.length()));
    return *this;
}

HttpResponse& HttpResponse::send(const std::string& text) {
    Body.setText(text);
    setHeader("Content-Type", "text/plain; charset=utf-8");
    setHeader("Content-Length", std::to_string(Body.length()));
    return *this;
}

HttpResponse& HttpResponse::send(const char* text) {
    Body.setText(std::string(text));
    setHeader("Content-Type", "text/plain; charset=utf-8");
    setHeader("Content-Length", std::to_string(Body.length()));
    return *this;
}

HttpResponse& HttpResponse::send(const std::map<std::string, std::string>& form) {
    Body.setFormData(form);
    setHeader("Content-Type", "application/x-www-form-urlencoded");
    setHeader("Content-Length", std::to_string(Body.length()));
    return *this;
}

HttpResponse& HttpResponse::send(const std::vector<uint8_t>& data) {
    Body.setBinary(data);
    setHeader("Content-Type", "application/octet-stream");
    setHeader("Content-Length", std::to_string(Body.length()));
    return *this;
}



