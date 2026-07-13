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
#include <nlohmann/json.hpp>
#include "Types.h"
#include <fstream>#include <map>

class Utils {
public:
    static std::string showEscapes(const char* buffer, size_t length);
    static void hexDump(const char* data, size_t len);
    static void printDebug(const std::string& message);
    static std::vector<std::string> split(const char* buffer, const char* delim);

    static HttpRequest headerExtractor(std::string line);

    static std::vector<uint8_t> readFileToBuff(const std::string& filePath) {
        // 1. Read file as binary
        std::cout << "HUNG" << std::endl;
        
        std::ifstream input( "./file.txt", std::ios::binary );

        // // copies all data into buffer
        std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(input), {});
        std::string out;
        for(auto c: buffer){
            std::cout << c << std::endl;
        }
        // buffer += "\0";
        
        return buffer;
    }
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

    
    
    // Prevent instantiation
    const std::map<std::string, std::string> mimeTypes = {
    {".txt",    "text/plain"},
    {".html",   "text/html"},
    {".htm",    "text/html"},
    {".css",    "text/css"},
    {".js",     "application/javascript"},
    {".mjs",    "application/javascript"},
    {".json",   "application/json"},
    {".xml",    "text/xml"},
    {".csv",    "text/csv"},
    {".md",     "text/markdown"},
    {".rtf",    "application/rtf"},
    
    // Images
    {".jpg",    "image/jpeg"},
    {".jpeg",   "image/jpeg"},
    {".png",    "image/png"},
    {".gif",    "image/gif"},
    {".svg",    "image/svg+xml"},
    {".bmp",    "image/bmp"},
    {".webp",   "image/webp"},
    {".ico",    "image/x-icon"},
    {".tiff",   "image/tiff"},
    {".tif",    "image/tiff"},
    
    // Audio
    {".mp3",    "audio/mpeg"},
    {".wav",    "audio/wav"},
    {".ogg",    "audio/ogg"},
    {".m4a",    "audio/mp4"},
    {".aac",    "audio/aac"},
    {".flac",   "audio/flac"},
    {".wma",    "audio/x-ms-wma"},
    
    // Video
    {".mp4",    "video/mp4"},
    {".avi",    "video/x-msvideo"},
    {".mov",    "video/quicktime"},
    {".wmv",    "video/x-ms-wmv"},
    {".flv",    "video/x-flv"},
    {".webm",   "video/webm"},
    {".mkv",    "video/x-matroska"},
    {".m4v",    "video/mp4"},
    
    // Documents
    {".pdf",    "application/pdf"},
    {".doc",    "application/msword"},
    {".docx",   "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".xls",    "application/vnd.ms-excel"},
    {".xlsx",   "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".ppt",    "application/vnd.ms-powerpoint"},
    {".pptx",   "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {".odt",    "application/vnd.oasis.opendocument.text"},
    {".ods",    "application/vnd.oasis.opendocument.spreadsheet"},
    {".odp",    "application/vnd.oasis.opendocument.presentation"},
    
    // Archives
    {".zip",    "application/zip"},
    {".rar",    "application/vnd.rar"},
    {".7z",     "application/x-7z-compressed"},
    {".tar",    "application/x-tar"},
    {".gz",     "application/gzip"},
    {".bz2",    "application/x-bzip2"},
    {".xz",     "application/x-xz"},
    
    // Fonts
    {".ttf",    "font/ttf"},
    {".otf",    "font/otf"},
    {".woff",   "font/woff"},
    {".woff2",  "font/woff2"},
    {".eot",    "application/vnd.ms-fontobject"},
    
    // Programming files
    {".c",      "text/x-c"},
    {".cpp",    "text/x-c++"},
    {".h",      "text/x-c"},
    {".hpp",    "text/x-c++"},
    {".py",     "text/x-python"},
    {".java",   "text/x-java-source"},
    {".php",    "application/x-httpd-php"},
    {".rb",     "text/x-ruby"},
    {".go",     "text/x-go"},
    {".rs",     "text/x-rust"},
    {".sh",     "application/x-sh"},
    {".bat",    "application/x-msdos-program"},
    {".ps1",    "application/x-powershell"},
    
    // Data files
    {".yaml",   "application/x-yaml"},
    {".yml",    "application/x-yaml"},
    {".toml",   "application/toml"},
    {".ini",    "text/plain"},
    {".cfg",    "text/plain"},
    {".conf",   "text/plain"},
    {".log",    "text/plain"},
    
    // Web files
    {".wasm",   "application/wasm"},
    {".map",    "application/json"},
    {".manifest", "text/cache-manifest"},
    {".webmanifest", "application/manifest+json"},
    
    // Binary/Executable
    {".exe",    "application/vnd.microsoft.portable-executable"},
    {".msi",    "application/x-msdownload"},
    {".deb",    "application/vnd.debian.binary-package"},
    {".rpm",    "application/x-rpm"},
    {".dmg",    "application/x-apple-diskimage"},
    {".iso",    "application/x-iso9660-image"},
    
    // Default fallback
    {"",        "application/octet-stream"}
};



    Utils() = delete;
};
