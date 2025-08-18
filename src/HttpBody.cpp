#include "../include/PlusWeb/HttpBody.h"

void HttpBody::setJson(const json& j) {
    type = JSON;
    json_data = j;
    raw_data = j.dump();
}

void HttpBody::setText(const std::string& text) {
    type = TEXT;
    raw_data = text;
}

void HttpBody::setFormData(const std::map<std::string, std::string>& form) {
    type = FORM_DATA;
    form_data = form;
    raw_data = encodeFormData(form);
}

void HttpBody::setBinary(const std::vector<uint8_t>& data) {
    type = BINARY;
    binary_data = data;
}

size_t HttpBody::length() const {
    switch (type) {
        case JSON:
        case TEXT:
        case FORM_DATA:
        case MULTIPART:
            return raw_data.size();
        case BINARY:
            return binary_data.size();
        case EMPTY:
        default:
            return 0;
    }
}

size_t HttpBody::size() const {
    return length();
}

std::string HttpBody::encodeFormData(const std::map<std::string, std::string>& form) {
    std::string result;
    for (const auto& pair : form) {
        if (!result.empty()) result += "&";
        result += pair.first + "=" + pair.second; // TODO: URL-encode keys/values
    }
    return result;
}


