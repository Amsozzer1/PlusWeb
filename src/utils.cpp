#include "../include/PlusWeb/utils.h"
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <vector>

HttpRequest Utils::headerExtractor(std::string line){
    HttpRequest req = HttpRequest();
    std::vector<std::string>  parts = Utils::split(line.c_str(), "\r\n");
    // Utils::MetaData meta = Utils::metaDataExcractor(parts[0]);
    std::vector<std::string>  metaData= Utils::split(parts[0].c_str(), " ");
    req.method = metaData[0];
    req.path = metaData[1];
    req.protocol = metaData[2];
    




    parts.erase(parts.begin()+0);
    
    for(auto p:parts){
        auto pair = split(p.c_str(), ": ");
        req.headers[pair[0]] = pair[1];
    }

    return req;
}



std::string Utils::showEscapes(const char* buffer, size_t length) {
    std::string result;
    for (size_t i = 0; i < length; i++) {
        char c = buffer[i];
        switch(c) {
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '\\': result += "\\\\"; break;
            case '\0': result += "\\0"; break;
            default: result += c; break;
        }
    }
    return result;
}


std::vector<std::string> Utils::split(const char *buffer, const char* delim) {
    std::vector<std::string> parts;
    std::string str(buffer);  // Convert to string for easier manipulation
    std::string delimiter(delim);
    
    if (str.empty() || delimiter.empty()) {
        if (!str.empty()) parts.push_back(str);
        return parts;
    }
    
    size_t start = 0;
    size_t end = 0;
    
    while ((end = str.find(delimiter, start)) != std::string::npos) {
        std::string token = str.substr(start, end - start);
        if (!token.empty()) {  // Skip empty tokens
            parts.push_back(token);
        }
        start = end + delimiter.length();
    }
    
    // Add the remaining part
    std::string remaining = str.substr(start);
    if (!remaining.empty()) {
        parts.push_back(remaining);
    }
    
    return parts;
}

void Utils::hexDump(const char* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (i % 16 == 0) std::cout << std::endl;
        std::cout << std::hex << std::setfill('0') << std::setw(2) 
                  << (unsigned char)data[i] << " ";
    }
    std::cout << std::dec << std::endl;
}

void Utils::printDebug(const std::string& message) {
    std::cout << "[DEBUG] " << message << std::endl;
}