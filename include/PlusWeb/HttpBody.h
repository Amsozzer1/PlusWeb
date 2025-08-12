#pragma once
#include "json.hpp"
#include <map>
#include <vector>
#include <string>

using json = nlohmann::json;

class HttpBody {
public:
    enum Type {
        EMPTY,
        TEXT,
        JSON,
        FORM_DATA,
        MULTIPART,
        BINARY
    };

private:
    Type type = EMPTY;
    std::string raw_data;           // Original raw body
    json json_data;                 // Parsed JSON
    std::map<std::string, std::string> form_data;  // URL-encoded form
    std::vector<uint8_t> binary_data; // Binary data

public:
    // Constructors
    HttpBody() = default;
    
    // Setters based on Content-Type
    void setJson(const json& j) {
        type = JSON;
        json_data = j;
        raw_data = j.dump();
    }
    
    void setText(const std::string& text) {
        type = TEXT;
        raw_data = text;
    }
    
    void setFormData(const std::map<std::string, std::string>& form) {
        type = FORM_DATA;
        form_data = form;
        // Convert to URL-encoded string
        raw_data = encodeFormData(form);
    }
    
    void setBinary(const std::vector<uint8_t>& data) {
        type = BINARY;
        binary_data = data;
    }
    
    // Getters
    Type getType() const { return type; }
    const std::string& getRaw() const { return raw_data; }
    const json& getJson() const { return json_data; }
    const std::map<std::string, std::string>& getFormData() const { return form_data; }
    const std::vector<uint8_t>& getBinary() const { return binary_data; }
    
    // Convenience methods
    bool isEmpty() const { return type == EMPTY; }
    bool isJson() const { return type == JSON; }
    bool isText() const { return type == TEXT; }
    bool isForm() const { return type == FORM_DATA; }

    size_t length() const {
        switch (type) {
            case JSON:
            case TEXT:
            case FORM_DATA:
            case MULTIPART:
                return raw_data.size();        // serialized form
            case BINARY:
                return binary_data.size();
            case EMPTY:
            default:
                return 0;
        }
    }

    // Optional alias
    size_t size() const { return length(); }
    
private:
    std::string encodeFormData(const std::map<std::string, std::string>& form) {
        // Implementation for URL encoding
        std::string result;
        for (const auto& pair : form) {
            if (!result.empty()) result += "&";
            result += pair.first + "=" + pair.second; // URL encode these
        }
        return result;
    }
};