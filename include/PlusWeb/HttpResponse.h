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
        // Whole response as one string. Kept for callers that want it; the
        // server uses serialize() so it can write head and body separately.
        std::string prepareResponse();

        // Fills `head` with the status line and headers. A body larger than
        // `splitAbove` is left in `body` so the two can go out as one writev
        // without being concatenated; anything smaller is appended straight to
        // `head`, because one buffer beats saving a cheap copy. `includeBody`
        // false omits the body entirely, which is what a HEAD response needs.
        void serialize(std::string& head, std::string& body,
                       size_t splitAbove = static_cast<size_t>(-1),
                       bool includeBody = true) const;


        std::string getResponseMessage(int status) const;
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


