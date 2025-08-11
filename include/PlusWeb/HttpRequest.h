#include <iostream>
#include <map>
class HttpRequest{

    public:
        std::string method;
        std::string path;
        std::string protocol;
        std::map<std::string, std::string> headers;
        std::string body;
        std::map<std::string, std::string> params;
        
        
        std::string getParam(const std::string& name);

        HttpRequest();
        HttpRequest(
            std::string method, 
            std::string path,  
            std::string protocol, 
            std::map<std::string, std::string> headers,  
            std::string body
        );

        void printRequestInfo() const;

};