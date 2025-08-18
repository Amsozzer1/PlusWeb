#include "../include/PlusWeb/trie.h"


// ==========================
// Node method implementations
// ==========================

Node::Node() {
    this->value = "";
    isLeaf = true;
}

Node::Node(std::string v) {
    this->value = v;
    isLeaf = false;

    if (!v.empty() && v[0] == ':') {
        isParameter = true;
        parameterName = v.substr(1);
    }
}

Node* Node::insertANode(Node* node, std::string v) {
    if (v == "") return node;

    if (node->children.find(v) != node->children.end()) {
        return node->children[v];
    }

    node->children[v] = new Node(v);
    node->isLeaf = false;
    return node->children[v];
}

Node::~Node() {
    for (auto& pair : children) {
        // delete pair.second;
    }
    // children.clear();
}

Node* Node::insert(Node* curr, std::string value, std::function<void(HttpRequest&, HttpResponse&)> func) {
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

    std::string remaining = "";
    for (size_t i = 1; i < segments.size(); i++) {
        if (i > 1) remaining += "/";
        remaining += segments[i];
    }

    return insert(new_node, remaining, func);
}

void Node::printChildren() {
    for (const auto& pair : children) {
        std::cout << pair.first << " ";
    }
    std::cout << std::endl;
}

void Node::printChildrenDetailed() {
    std::cout << "Node '" << this->value << "' children:" << std::endl;
    for (const auto& pair : children) {
        std::cout << "  " << pair.first << " -> " << pair.second->value;
        if (pair.second->isLeaf) std::cout << " (end of word)";
        std::cout << std::endl;
    }
}

void Node::printTree(int depth) {
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

Node* Node::find(Node* node, const std::string& word, std::map<std::string, std::string>& params) {
    if (word.empty()) return node;

    std::vector<std::string> segments = Utils::split(word.c_str(), "/");
    if (segments.empty()) return node;

    std::string currentSegment = segments[0];

    if (node->children.find(currentSegment) != node->children.end()) {
        std::string remaining = "";
        for (size_t i = 1; i < segments.size(); i++) {
            if (i > 1) remaining += "/";
            remaining += segments[i];
        }
        return find(node->children[currentSegment], remaining, params);
    }

    for (const auto& child : node->children) {
        if (child.second->isParameter) {
            params[child.second->parameterName] = currentSegment;

            std::string remaining = "";
            for (size_t i = 1; i < segments.size(); i++) {
                if (i > 1) remaining += "/";
                remaining += segments[i];
            }
            return find(child.second, remaining, params);
        }
    }

    return nullptr;
}

Node* Node::find(Node* node, const std::string& word) {
    if (word.empty()) return node;

    std::vector<std::string> segments = Utils::split(word.c_str(), "/");
    if (segments.empty()) return node;

    if (node->children.find(segments[0]) == node->children.end()) {
        return nullptr;
    }

    std::string remaining = "";
    for (size_t i = 1; i < segments.size(); i++) {
        if (i > 1) remaining += "/";
        remaining += segments[i];
    }

    return find(node->children[segments[0]], remaining);
}


// Default constructor
Trie::Trie() {
    this->root = new Node();
}

// Constructor with initial value
Trie::Trie(const std::string& value, std::function<void(HttpRequest&, HttpResponse&)> handler) {
    this->root = new Node();
    this->insert(value, handler);
}

// Destructor
Trie::~Trie() {
    // delete root; // Uncommented - Node destructor handles recursive cleanup
}

// Insert a word into the trie
void Trie::insert(const std::string& word, std::function<void(HttpRequest&, HttpResponse&)> handler) {
    this->root->insert(this->root, word, handler);
}

// Search for a word in the trie
bool Trie::search(const std::string& word) {
    Node* result = this->root->find(this->root, word);
    return result != nullptr && result->isLeaf;
}

// NEW: Search with parameter extraction
bool Trie::search(const std::string& word, std::map<std::string, std::string>& params) {
    Node* result = this->root->find(this->root, word, params);
    return result != nullptr && result->isLeaf;
}

// Check if any word starts with the given prefix
bool Trie::startsWith(const std::string& prefix) {
    Node* result = this->root->find(this->root, prefix);
    return result != nullptr;
}

// Print the trie structure
void Trie::printTrie() {
    if (this->root != nullptr) {
        this->root->printTree(0);
    }
}

// Print all words in the trie
void Trie::printAllWords() {
    std::string currentWord = "";
    printWordsFromNode(this->root, currentWord);
}

// Get all words with a given prefix
std::vector<std::string> Trie::getWordsWithPrefix(const std::string& prefix) {
    std::vector<std::string> result;
    Node* prefixNode = this->root->find(this->root, prefix);
    if (prefixNode != nullptr) {
        std::string currentWord = prefix;
        collectWordsFromNode(prefixNode, currentWord, result);
    }
    return result;
}

// Check if trie is empty
bool Trie::isEmpty() {
    return !hasAnyWords(this->root);
}

// Get number of words in trie
int Trie::countWords() {
    return countWordsFromNode(this->root);
}

void Trie::printWordsFromNode(Node* node, std::string& currentWord) {
    if (node == nullptr) return;
    if (node->isLeaf && !currentWord.empty()) {
        std::cout << currentWord << std::endl;
    }
    for (const auto& pair : node->children) {
        std::string separator = currentWord.empty() ? "" : "/";
        currentWord += separator + pair.first;
        printWordsFromNode(pair.second, currentWord);
        
        // Remove what we added
        size_t removeLength = separator.length() + pair.first.length();
        currentWord.erase(currentWord.length() - removeLength);
    }
}

// Helper function to collect words from a node
void Trie::collectWordsFromNode(Node* node, std::string currentWord, std::vector<std::string>& result) {
    if (node == nullptr) return;
    if (node->isLeaf) {
        result.push_back(currentWord);
    }
    for (const auto& pair : node->children) {
        std::string nextWord = currentWord.empty() ? pair.first : currentWord + "/" + pair.first;
        collectWordsFromNode(pair.second, nextWord, result);
    }
}

// Helper function to check if any words exist
bool Trie::hasAnyWords(Node* node) {
    if (node == nullptr) return false;
    if (node->isLeaf && node != this->root) return true;
    for (const auto& pair : node->children) {
        if (hasAnyWords(pair.second)) {
            return true;
        }
    }
    return false;
}

// Helper function to count words
int Trie::countWordsFromNode(Node* node) {
    if (node == nullptr) return 0;
    int count = (node->isLeaf && node != this->root) ? 1 : 0;
    for (const auto& pair : node->children) {
        count += countWordsFromNode(pair.second);
    }
    return count;
}

// OLD: Basic search without parameters
Node* Trie::searchNode(const std::string& word) const{
    Node* result = this->root->find(this->root, word);
    if(!result) return nullptr;
    return result;
}

// NEW: Enhanced search that extracts parameters
Node* Trie::searchNode(const std::string& word, std::map<std::string, std::string>& params) const {
    Node* result = this->root->find(this->root, word, params);
    if(!result) return nullptr;
    return result;
}