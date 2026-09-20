#pragma once
#include "HttpBody.h"


class HttpResponse{
    // Shared status-line text, built once on first use. Defined in
    // HttpResponse.cpp.
    static const std::map<int, std::string>& defaultResponseCodes();

    public:
        std::string message;
        int $status;
        std::string protocol;
        std::map<std::string, std::string> headers;
        // std::string body;
        HttpBody Body;
        HttpResponse();
        std::string prepareResponse();


        std::string getResponseMessage(int status);
        HttpResponse& status(int status);
        HttpResponse& setHeader(std::string,std::string);
        HttpResponse& setHeaders(std::map<std::string,std::string>);


        HttpResponse& send(const HttpBody& body);
    HttpResponse& send(HttpBody&& body);
    
    // JSON types
    HttpResponse& send(const nlohmann::json& j);
    
    // Text types
    HttpResponse& send(const std::string& text);
    HttpResponse& send(const char* text);
    HttpResponse& send(std::string_view text);
    
    // Form data
    HttpResponse& send(const std::map<std::string, std::string>& form);
    
    // Binary data
    HttpResponse& send(const std::vector<uint8_t>& data);
    

        

};


