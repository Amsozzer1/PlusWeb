#include <PlusWeb/utils.h>

#include <fstream>
#include <iterator>

std::vector<std::string> Utils::split(const char *buffer, const char* delim) {
    std::vector<std::string> parts;
    std::string str(buffer);
    std::string delimiter(delim);

    if (str.empty() || delimiter.empty()) {
        if (!str.empty()) parts.push_back(str);
        return parts;
    }

    size_t start = 0;
    size_t end = 0;
    while ((end = str.find(delimiter, start)) != std::string::npos) {
        std::string token = str.substr(start, end - start);
        if (!token.empty()) {
            parts.push_back(token);
        }
        start = end + delimiter.length();
    }

    std::string remaining = str.substr(start);
    if (!remaining.empty()) {
        parts.push_back(remaining);
    }
    return parts;
}

bool Utils::readFile(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }

    out.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    return true;
}

std::string Utils::mimeTypeFor(const std::string& path) {
    static const std::map<std::string, std::string> kMimeTypes = {
        // Text
        {".txt", "text/plain"},
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".mjs", "application/javascript"},
        {".json", "application/json"},
        {".xml", "text/xml"},
        {".csv", "text/csv"},
        {".md", "text/markdown"},

        // Images
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".webp", "image/webp"},
        {".ico", "image/x-icon"},

        // Audio / video
        {".mp3", "audio/mpeg"},
        {".wav", "audio/wav"},
        {".ogg", "audio/ogg"},
        {".mp4", "video/mp4"},
        {".webm", "video/webm"},

        // Documents / archives
        {".pdf", "application/pdf"},
        {".zip", "application/zip"},
        {".gz", "application/gzip"},
        {".tar", "application/x-tar"},

        // Fonts
        {".ttf", "font/ttf"},
        {".otf", "font/otf"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},

        // Web
        {".wasm", "application/wasm"},
        {".yaml", "application/x-yaml"},
        {".yml", "application/x-yaml"},
    };

    // Look at the last path segment only, so that "/v1.2/README" is not treated
    // as having a ".2/README" extension.
    const size_t slash = path.find_last_of('/');
    const std::string filename = slash == std::string::npos ? path : path.substr(slash + 1);

    const size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos) {
        return "application/octet-stream";
    }

    std::string ext = filename.substr(dot);
    for (char& c : ext) {
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }

    auto it = kMimeTypes.find(ext);
    return it == kMimeTypes.end() ? "application/octet-stream" : it->second;
}
