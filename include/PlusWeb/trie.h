#include "HttpServer.h"
#include "utils.h"
#include <cctype>
#include <cstddef>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>

class Node {
public:
    char value;
    bool isLeaf;
    std::unordered_map<char, Node*> children;
    std::function<void(HttpRequest&, HttpResponse&)> handler;


    Node() {
        this->value = '\0';  // null character for root
        isLeaf = true;
    }

    Node(char v) {
        this->value = v;
        isLeaf = false;
    }

    Node* insertANode(Node* node, char v) {
        if (!v) return node;
        
        // Check if child already exists
        if (node->children.find(v) != node->children.end()) {
            return node->children[v];
        }
        
        // Create new child
        node->children[v] = new Node(v);
        node->isLeaf = false; // not a leaf anymore if it has children
        
        return node->children[v];
    }

    ~Node() {
        for (auto& pair : children) {
            delete pair.second;
        }
        children.clear();
    }

    Node* insert(Node* curr, std::string value) {
        if (value.empty()) {
            curr->isLeaf = true;
            return curr;
        }
        
        auto new_node = insertANode(curr, value[0]);
        value.erase(value.begin());
        
        return insert(new_node, value);
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
        
        if (value == '\0') {
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

    // Helper to find a word in the trie
    Node* find(Node* node, const std::string& word) {
        if (word.empty()) return node;
        
        char firstChar = word[0];
        if (node->children.find(firstChar) == node->children.end()) {
            return nullptr;
        }
        
        return find(node->children[firstChar], word.substr(1));
    }
};

class Trie {
public:
    Node* root;

    // Default constructor
    Trie();
    // Constructor with initial value
    Trie(const std::string& value);

    // Proper destructor - recursively delete all nodes
    ~Trie();

    // Insert a word into the trie
    void insert(const std::string& word);

    // Search for a word in the trie
    bool search(const std::string& word);

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