#pragma once
#include "HttpBody.h"


// #include "Body.h"

class HttpRequest{

    public:
        std::string method;
        std::string path;
        std::string protocol;
        std::map<std::string, std::string> headers;
        HttpBody body;
        std::map<std::string, std::string> params;
        std::string getParam(const std::string& name);
        std::map<std::string, std::string> query;
        std::map<std::string, std::string> cookies;

        HttpRequest();
        HttpRequest(
            std::string method, 
            std::string path,  
            std::string protocol, 
            std::map<std::string, std::string> headers,  
            HttpBody body
        );

        void printRequestInfo() const;

        

};