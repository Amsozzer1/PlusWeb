#pragma once
#include "utils.h"
#include <cctype>
#include <cstddef>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <map>


class Node {
public:
    std::string value;
    bool isLeaf;
    std::unordered_map<std::string, Node*> children;
    std::function<void(HttpRequest&, HttpResponse&)> handler;
    bool isParameter = false;
    std::string parameterName = ""; 


    Node();

    Node(std::string v);

    Node* insertANode(Node* node, std::string v);

    ~Node();

    Node* insert(Node* curr, std::string value, std::function<void(HttpRequest&, HttpResponse&)> func);

    // Simple version - just show existing children
    void printChildren();

    // Detailed version - show structure
    void printChildrenDetailed();

    // Tree-like visualization
    void printTree(int depth = 0);
    Node* find(Node* node, const std::string& word, std::map<std::string, std::string>& params);

    // Helper to find a word in the trie
    Node* find(Node* node, const std::string& word);
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
    Node* searchNode(const std::string& word, std::map<std::string, std::string>& params) const;
    // Insert a word into the trie
    void insert(const std::string& word, std::function<void(HttpRequest&, HttpResponse&)> handler);

    // Search for a word in the trie
    bool search(const std::string& word);

    Node* searchNode(const std::string& word) const;

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