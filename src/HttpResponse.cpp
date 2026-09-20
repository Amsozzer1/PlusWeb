#include <PlusWeb/HttpResponse.h>



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
    else if (this->Body.getType() == HttpBody::BINARY) {
        response += "\r\n";
        // Convert binary to string for transmission
        const auto& binaryData = this->Body.getBinary();
        response += std::string(binaryData.begin(), binaryData.end());
    }
    return response;
}

const std::map<int, std::string>& HttpResponse::defaultResponseCodes() {
    static const std::map<int, std::string> kCodes = {
        {100, "Continue"},
        {101, "Switching Protocols"},
        {102, "Processing"},
        {103, "Early Hints"},
        {200, "OK"},
        {201, "Created"},
        {202, "Accepted"},
        {203, "Non-Authoritative Information"},
        {204, "No Content"},
        {205, "Reset Content"},
        {206, "Partial Content"},
        {207, "Multi-Status"},
        {208, "Already Reported"},
        {218, "This is fine (Apache Web Server)"},
        {226, "IM Used"},
        {300, "Multiple Choices"},
        {301, "Moved Permanently"},
        {302, "Found"},
        {303, "See Other"},
        {304, "Not Modified"},
        {306, "Switch Proxy"},
        {307, "Temporary Redirect"},
        {308, "Resume Incomplete"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {402, "Payment Required"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {406, "Not Acceptable"},
        {407, "Proxy Authentication Required"},
        {408, "Request Timeout"},
        {409, "Conflict"},
        {410, "Gone"},
        {411, "Length Required"},
        {412, "Precondition Failed"},
        {413, "Request Entity Too Large"},
        {414, "Request-URI Too Long"},
        {415, "Unsupported Media Type"},
        {416, "Requested Range Not Satisfiable"},
        {417, "Expectation Failed"},
        {418, "I'm a teapot"},
        {419, "Page Expired (Laravel Framework)"},
        {420, "Method Failure (Spring Framework)"},
        {421, "Misdirected Request"},
        {422, "Unprocessable Entity"},
        {423, "Locked"},
        {424, "Failed Dependency"},
        {426, "Upgrade Required"},
        {428, "Precondition Required"},
        {429, "Too Many Requests"},
        {431, "Request Header Fields Too Large"},
        {440, "Login Time-out"},
        {444, "Connection Closed Without HttpResponse"},
        {449, "Retry With"},
        {450, "Blocked by Windows Parental Controls"},
        {451, "Unavailable For Legal Reasons"},
        {494, "Request Header Too Large"},
        {495, "SSL Certificate Error"},
        {496, "SSL Certificate Required"},
        {497, "HTTP Request Sent to HTTPS Port"},
        {498, "Invalid Token (Esri)"},
        {499, "Client Closed Request"},
        {500, "Internal Server Error"},
        {501, "Not Implemented"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Timeout"},
        {505, "HTTP Version Not Supported"},
        {506, "Variant Also Negotiates"},
        {507, "Insufficient Storage"},
        {508, "Loop Detected"},
        {509, "Bandwidth Limit Exceeded"},
        {510, "Not Extended"},
        {511, "Network Authentication Required"},
        {520, "Unknown Error"},
        {521, "Web Server Is Down"},
        {522, "Connection Timed Out"},
        {523, "Origin Is Unreachable"},
        {524, "A Timeout Occurred"},
        {525, "SSL Handshake Failed"},
        {526, "Invalid SSL Certificate"},
        {527, "Railgun Listener to Origin Error"},
        {530, "Origin DNS Error"},
        {598, "Network Read Timeout Error"}
        
    };
    return kCodes;
}

std::string HttpResponse::getResponseMessage(int status){
    const auto& codes = defaultResponseCodes();
    auto it = codes.find(status);
    return it == codes.end() ? "An error occured" : it->second;
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
        case HttpBody::EMPTY:
            setHeader("Content-Type", "text/plain; charset=utf-8");
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
    setHeader("Content-Type", "text/html; charset=utf-8");
    setHeader("Content-Length", std::to_string(Body.length()));
    return *this;
}

HttpResponse& HttpResponse::send(const char* text) {
    Body.setText(std::string(text));
    setHeader("Content-Type", "text/html; charset=utf-8");
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
    // Callers that know the real type (e.g. from Utils::mimeTypeFor) should
    // setHeader("Content-Type", ...) after send() to override this.
    setHeader("Content-Type", "application/octet-stream");
    setHeader("Content-Length", std::to_string(Body.length()));
    return *this;
}



