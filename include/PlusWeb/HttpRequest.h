#pragma once
#include <iostream>
#include <map>
#include "HttpBody.h"

// #include "Body.h"

class HttpRequest{

    public:
        std::string method;
        std::string path;
        std::string protocol;
        std::map<std::string, std::string> headers;
        // @suppress IntelliSense
        // std::string body;
        HttpBody body;
        
        // using BodyType = std::va

        std::map<std::string, std::string> params;
        
        
        std::string getParam(const std::string& name);

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