#pragma once
#include "utils.h"
#include <cctype>
#include <cstddef>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
// #include "HttpResponse.h"
class Node {
public:
    std::string value;
    bool isLeaf;
    std::unordered_map<std::string, Node*> children;
    std::function<void(HttpRequest&, HttpResponse&)> handler;
    bool isParameter = false;
    std::string parameterName = ""; 


    Node() {
        this->value = "";
        isLeaf = true;
    }

    Node(std::string v) {
        this->value = v;
        isLeaf = false;
        
        // NEW: Check if this is a parameter segment
        if (!v.empty() && v[0] == ':') {
            isParameter = true;
            parameterName = v.substr(1); // Store "id" from ":id"
        }
    }

    Node* insertANode(Node* node, std::string v) {
        if (v == "") return node;
        
        // Check if child already exists
        if (node->children.find(v) != node->children.end()) {
            return node->children[v];
        }
        
        // Create new child
        node->children[v] = new Node(v);
        node->isLeaf = false;
        return node->children[v];
    }

    ~Node() {
        for (auto& pair : children) {
            // delete pair.second;
        }
        // children.clear();
    }

    Node* insert(Node* curr, std::string value, std::function<void(HttpRequest&, HttpResponse&)> func) {
        if (value.empty()) {
            curr->isLeaf = true;
            curr->handler = func;
            return curr;
        }
        std::vector<std::string> segments = Utils::split(value.c_str(), "/");
        if (segments.empty()) {
            curr->isLeaf = true;
            curr->handler = func;
            return curr;
        }
        
        auto new_node = insertANode(curr, segments[0]);
        
        // Build remaining path from segments[1] onwards
        std::string remaining = "";
        for (size_t i = 1; i < segments.size(); i++) {
            if (i > 1) remaining += "/";
            remaining += segments[i];
        }
        
        return insert(new_node, remaining, func);
    }

    // Simple version - just show existing children
    void printChildren() {
        for (const auto& pair : children) {
            std::cout << pair.first << " ";
        }
        std::cout << std::endl;
    }

    // Detailed version - show structure
    void printChildrenDetailed() {
        std::cout << "Node '" << this->value << "' children:" << std::endl;
        for (const auto& pair : children) {
            std::cout << "  " << pair.first << " -> " << pair.second->value;
            if (pair.second->isLeaf) std::cout << " (end of word)";
            std::cout << std::endl;
        }
    }

    // Tree-like visualization
    void printTree(int depth = 0) {
        for (int i = 0; i < depth; i++) std::cout << "  ";
        
        if (value == "") {
            std::cout << "'ROOT'";
        } else {
            std::cout << "'" << this->value << "'";
        }
        
        if (this->isLeaf) std::cout << " *";
        std::cout << std::endl;
        
        for (const auto& pair : children) {
            pair.second->printTree(depth + 1);
        }
    }
    Node* find(Node* node, const std::string& word, std::map<std::string, std::string>& params) {
        if (word.empty()) return node;
        
        std::vector<std::string> segments = Utils::split(word.c_str(), "/");
        if (segments.empty()) return node;
        
        std::string currentSegment = segments[0];
        
        // Try exact match first
        if (node->children.find(currentSegment) != node->children.end()) {
            std::string remaining = "";
            for (size_t i = 1; i < segments.size(); i++) {
                if (i > 1) remaining += "/";
                remaining += segments[i];
            }
            return find(node->children[currentSegment], remaining, params);
        }
        
        // Try parameter match
        for (const auto& child : node->children) {
            if (child.second->isParameter) {
                // This parameter node matches the current segment
                params[child.second->parameterName] = currentSegment;
                
                std::string remaining = "";
                for (size_t i = 1; i < segments.size(); i++) {
                    if (i > 1) remaining += "/";
                    remaining += segments[i];
                }
                return find(child.second, remaining, params);
            }
        }
        
        return nullptr; // No match found
    }

    // Helper to find a word in the trie
    Node* find(Node* node, const std::string& word) {
        if (word.empty()) return node;
        
        std::vector<std::string> segments = Utils::split(word.c_str(), "/");
        if (segments.empty()) return node;
        
        // Check if first segment exists in children
        if (node->children.find(segments[0]) == node->children.end()) {
            return nullptr;
        }
        
        // Build remaining path from segments[1] onwards
        std::string remaining = "";
        for (size_t i = 1; i < segments.size(); i++) {
            if (i > 1) remaining += "/";
            remaining += segments[i];
        }
        
        return find(node->children[segments[0]], remaining);
    }
};

class Trie {
public:
    Node* root;

    // Default constructor
    Trie();
    // Constructor with initial value
    Trie(const std::string& value, std::function<void(HttpRequest&, HttpResponse&)> handler);

    // Proper destructor - recursively delete all nodes
    ~Trie();
    bool search(const std::string& word, std::map<std::string, std::string>& params);
    Node* searchNode(const std::string& word, std::map<std::string, std::string>& params);
    // Insert a word into the trie
    void insert(const std::string& word, std::function<void(HttpRequest&, HttpResponse&)> handler);

    // Search for a word in the trie
    bool search(const std::string& word);

    Node* searchNode(const std::string& word);

    // Check if any word starts with the given prefix
    bool startsWith(const std::string& prefix);

    // Print the trie structure
    void printTrie();

    // Print all words in the trie
    void printAllWords();

    // Get all words with a given prefix
    std::vector<std::string> getWordsWithPrefix(const std::string& prefix);

    // Check if trie is empty
    bool isEmpty();

    // Get number of words in trie
    int countWords();



private:
    // Helper function to print all words from a node
    void printWordsFromNode(Node* node, std::string& currentWord);

    // Helper function to collect words from a node
    void collectWordsFromNode(Node* node, std::string currentWord, std::vector<std::string>& result);

    // Helper function to check if any words exist
    bool hasAnyWords(Node* node);

    // Helper function to count words
    int countWordsFromNode(Node* node);
};