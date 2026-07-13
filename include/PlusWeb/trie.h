#pragma once
#include "Types.h"

#include <map>
#include <string>
#include <unordered_map>

// One path segment in the routing trie. A literal segment ("users") matches by
// name; a parameter segment (":id") matches any single segment and binds its
// value into the request's params.
class Node {
public:
    std::string value;
    bool isLeaf = true;
    std::unordered_map<std::string, Node*> children;
    RouteHandler handler;
    bool isParameter = false;
    std::string parameterName;

    Node() = default;
    explicit Node(std::string v);
    ~Node();

    // Owns its children by raw pointer; copying would double-free them.
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    Node* insert(Node* curr, const std::string& path, RouteHandler func);

    // Returns the node matching `path`, filling `params` with any parameter
    // segments bound along the way. Returns nullptr if nothing matches.
    Node* find(Node* node, const std::string& path, std::map<std::string, std::string>& params);

private:
    Node* insertChild(Node* node, const std::string& segment);
};

// Segment-based trie keyed by "METHOD:/path/:param", storing a handler per route.
class Trie {
public:
    Trie();
    ~Trie();

    // Owns the node graph; copying would double-free it.
    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;

    void insert(const std::string& path, RouteHandler handler);
    Node* searchNode(const std::string& path, std::map<std::string, std::string>& params) const;

private:
    Node* root;
};
