#pragma once

#include <string>
#include <iostream>
#include <cstddef>
#include <sstream>
#include <map>
#include <vector>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <string>
// #include <curl/curl.h>
#include "json.hpp"



class Utils {
public:


    static std::string showEscapes(const char* buffer, size_t length);
    static void hexDump(const char* data, size_t len);
    static void printDebug(const std::string& message);
    static std::vector<std::string> split(const char* buffer, const char* delim);

    static HttpRequest headerExtractor(std::string line);


    static char from_hex(char ch) {
        return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
    }
    
    static std::string url_decode(std::string text) {
        char h;
        std::ostringstream escaped;
        escaped.fill('0');
    
        for (auto i = text.begin(), n = text.end(); i != n; ++i) {
            std::string::value_type c = (*i);
    
            if (c == '%') {
                if (i[1] && i[2]) {
                    h = from_hex(i[1]) << 4 | from_hex(i[2]);
                    escaped << h;
                    i += 2;
                }
            } else if (c == '+') {
                escaped << ' ';
            } else {
                escaped << c;
            }
        }
    
        return escaped.str();
    }

    
    
    // Prevent instantiation
    Utils() = delete;
};
