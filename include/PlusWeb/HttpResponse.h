#pragma once
#include "HttpBody.h"


class HttpResponse{
    // Status-line text. Shared across every response and built once, on first
    // use: this used to be a per-instance member with a default initializer,
    // which rebuilt an 85-entry std::map (187 heap allocations) for every
    // single response. Defined in HttpResponse.cpp.
    static const std::map<int, std::string>& defaultResponseCodes();

    // Per-response overrides installed by updateResponseCode(). Default
    // constructed and therefore free unless a caller actually uses it.
    std::map<int, std::string> responseCodeOverrides;

    public:
        std::string message;
        int $status;
        std::string protocol;
        std::map<std::string, std::string> headers;
        // std::string body;
        HttpBody Body;
        HttpResponse();
        std::string prepareResponse();

        void updateResponseCode(int code, std::string message);

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


