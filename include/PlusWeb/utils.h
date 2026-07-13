#pragma once

#include <cstddef>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Types.h"

class Utils {
public:
    static std::vector<std::string> split(const char* buffer, const char* delim);

    static HttpRequest headerExtractor(std::string line);

    // Reads `path` into `out` as raw bytes. Returns false if it cannot be
    // opened, leaving `out` untouched.
    static bool readFile(const std::string& path, std::vector<uint8_t>& out);

    // MIME type for a path's extension ("a/b/c.png" -> "image/png"), falling
    // back to application/octet-stream for unknown or missing extensions.
    static std::string mimeTypeFor(const std::string& path);

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

    static std::string urlEncode(const std::string& input) {
        std::ostringstream encoded;
        
        for (char c : input) {
            int ascii = static_cast<unsigned char>(c);
            
            if ((ascii >= 48 && ascii <= 57) ||    // 0-9
                (ascii >= 65 && ascii <= 90) ||    // A-Z
                (ascii >= 97 && ascii <= 122) ||   // a-z
                ascii == 45 ||                     // -
                ascii == 46 ||                     // .
                ascii == 95 ||                     // _
                ascii == 126) {                    // ~
                
                encoded << c;  
            } else {
                encoded << '%' 
                        << std::uppercase 
                        << std::hex 
                        << std::setw(2) 
                        << std::setfill('0') 
                        << ascii;
            }
        }
        
        return encoded.str();
     }

    // Not instantiable: every member is static.
    Utils() = delete;
};
