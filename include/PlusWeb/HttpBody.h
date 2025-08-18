#pragma once
#include "json.hpp"
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <cstdint>
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
    void setJson(const json& j);
    
    void setText(const std::string& text);
    
    void setFormData(const std::map<std::string, std::string>& form);
    
    void setBinary(const std::vector<uint8_t>& data);
    
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

    size_t length() const;

    // Optional alias
    size_t size() const;
    
private:
    std::string encodeFormData(const std::map<std::string, std::string>& form);
};