#pragma once

#include <functional>
// #include <vector>
// #include <filesystem>

// Forward Declarations
class HttpRequest;
class HttpResponse;

using RouteHandler = std::function<void(HttpRequest&, HttpResponse&)>;
using NextFunction = std::function<void()>;
using MiddlewareFunction = std::function<void(HttpRequest&, HttpResponse&, NextFunction)>;
// struct stat {
//     // Basic file properties (OS-agnostic)
//     uint64_t size = 0;           // File size in bytes
//     uint64_t mtime_ms = 0;       // Modified time (Unix epoch milliseconds)
//     uint64_t atime_ms = 0;       // Access time
//     uint64_t ctime_ms = 0;       // Changed time
    
//     // File type enumeration (no OS constants)
//     enum FileType {
//         REGULAR_FILE = 1,
//         DIRECTORY = 2,
//         SYMLINK = 3,
//         UNKNOWN = 0
//     };
    
//     FileType type = UNKNOWN;
//     bool readable = true;
//     bool writable = false;
//     bool executable = false;
    
//     // Helper methods with no OS dependencies
//     std::string getLastModified() const {
//         if (mtime_ms == 0) {
//             return "Thu, 01 Jan 1970 00:00:00 GMT"; // Fallback
//         }
        
//         uint64_t seconds = mtime_ms / 1000;
        
//         // Simple GMT date formatting (RFC 7231 format)
//         // Using basic math instead of OS time functions
//         uint64_t days = seconds / 86400;
//         uint64_t timeOfDay = seconds % 86400;
//         uint64_t hours = timeOfDay / 3600;
//         uint64_t minutes = (timeOfDay % 3600) / 60;
//         uint64_t secs = timeOfDay % 60;
        
//         // Approximate date (good enough for HTTP headers)
//         uint64_t year = 1970 + (days / 365);
//         uint64_t remainingDays = days % 365;
//         uint64_t month = std::min(12ULL, (remainingDays / 30) + 1);
//         uint64_t day = std::min(31ULL, (remainingDays % 30) + 1);
        
//         // Day of week calculation
//         uint64_t dayOfWeek = (days + 4) % 7; // Jan 1, 1970 was Thursday
        
//         const char* dayNames[] = {"Thu", "Fri", "Sat", "Sun", "Mon", "Tue", "Wed"};
//         const char* monthNames[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
//                                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        
//         char buffer[64];
//         snprintf(buffer, sizeof(buffer), "%s, %02llu %s %llu %02llu:%02llu:%02llu GMT",
//                 dayNames[dayOfWeek], day, monthNames[month], year, hours, minutes, secs);
        
//         return std::string(buffer);
//     }
    
//     std::string generateETag() const {
//         // Simple ETag based on size and modification time
//         return "\"" + std::to_string(size) + "-" + std::to_string(mtime_ms) + "\"";
//     }
    
//     bool isDirectory() const {
//         return type == DIRECTORY;
//     }
    
//     bool isRegularFile() const {
//         return type == REGULAR_FILE;
//     }
    
//     bool isSymlink() const {
//         return type == SYMLINK;
//     }
    
//     // Factory method - populate from platform-specific code elsewhere
//     static stat create(uint64_t fileSize, uint64_t modifiedTime, FileType fileType) {
//         stat result;
//         result.size = fileSize;
//         result.mtime_ms = modifiedTime;
//         result.atime_ms = modifiedTime; // Default to same
//         result.ctime_ms = modifiedTime;
//         result.type = fileType;
//         result.readable = true;
//         return result;
//     }
    
//     // Create from basic file info (no OS calls in this struct)
//     static stat createFile(uint64_t size, uint64_t mtime) {
//         return create(size, mtime, REGULAR_FILE);
//     }
    
//     static stat createDirectory(uint64_t mtime) {
//         return create(0, mtime, DIRECTORY);
//     }
// };
// struct StaticOptions {
//     std::string dotfiles = "ignore";        // "allow", "deny", "ignore"
//     bool etag = true;
//     bool fallthrough = true;
//     bool immutable = false;
//     std::vector<std::string> extensions = {}; // empty = serve all
//     std::string index = "index.html";       // "" = disable indexing
//     bool lastModified = true;
//     long maxAge = 0;                        // milliseconds
//     bool redirect = true;
    
//     // Pure function - no OS dependencies
//     std::function<void(HttpResponse&, const std::string&, const stat&)> setHeaders;
    
//     // Additional options
//     bool acceptRanges = true;
//     std::string cacheControl = "";
    
//     // Helper methods (no OS calls)
//     bool shouldServeFile(const std::string& filename) const {
//         // Check dotfiles
//         if (!filename.empty() && filename[0] == '.') {
//             if (dotfiles == "deny" || dotfiles == "ignore") {
//                 return false;
//             }
//         }
        
//         // Check extensions
//         if (!extensions.empty()) {
//             size_t dotPos = filename.find_last_of('.');
//             if (dotPos != std::string::npos) {
//                 std::string ext = filename.substr(dotPos);
//                 auto it = std::find(extensions.begin(), extensions.end(), ext);
//                 return it != extensions.end();
//             }
//             return false; // No extension, but extensions filter is active
//         }
        
//         return true;
//     }
    
//     std::string getCacheControl(const stat& /*fileStat*/) const {
//         if (!cacheControl.empty()) {
//             return cacheControl;
//         }
        
//         std::string result = "public";
//         if (maxAge > 0) {
//             result += ", max-age=" + std::to_string(maxAge / 1000);
//         }
//         if (immutable) {
//             result += ", immutable";
//         }
        
//         return result;
//     }
    
//     // Get MIME type without OS dependencies
//     std::string getMimeType(const std::string& filename) const {
//         size_t dotPos = filename.find_last_of('.');
//         if (dotPos == std::string::npos) {
//             return "application/octet-stream";
//         }
        
//         std::string ext = filename.substr(dotPos);
        
//         // Basic MIME type mapping (no OS registry needed)
//         if (ext == ".html" || ext == ".htm") return "text/html";
//         if (ext == ".css") return "text/css";
//         if (ext == ".js") return "application/javascript";
//         if (ext == ".json") return "application/json";
//         if (ext == ".png") return "image/png";
//         if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
//         if (ext == ".gif") return "image/gif";
//         if (ext == ".svg") return "image/svg+xml";
//         if (ext == ".ico") return "image/x-icon";
//         if (ext == ".txt") return "text/plain";
//         if (ext == ".pdf") return "application/pdf";
//         if (ext == ".zip") return "application/zip";
//         if (ext == ".xml") return "application/xml";
        
//         return "application/octet-stream";
//     }
// };


