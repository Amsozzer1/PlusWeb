#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <iostream>
#include <cstddef>
#include <map>
#include <vector>
#include "HttpRequest.h"
#include "HttpResponse.h"

class Utils {
public:


    static std::string showEscapes(const char* buffer, size_t length);
    static void hexDump(const char* data, size_t len);
    static void printDebug(const std::string& message);
    static std::vector<std::string> split(const char* buffer, const char* delim);

    static HttpRequest headerExtractor(std::string line);


    
    
    // Prevent instantiation
    Utils() = delete;
};

#endif