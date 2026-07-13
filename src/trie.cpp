#include <PlusWeb/trie.h>
#include <PlusWeb/utils.h>

namespace {

// Rejoins segments[1..] into the remaining path, e.g. {"users",":id","posts"}
// -> ":id/posts".
std::string joinRemaining(const std::vector<std::string>& segments) {
    std::string remaining;
    for (size_t i = 1; i < segments.size(); ++i) {
        if (i > 1) {
            remaining += "/";
        }
        remaining += segments[i];
    }
    return remaining;
}

}  // namespace

Node::Node(std::string v) : value(std::move(v)), isLeaf(false) {
    if (!value.empty() && value[0] == ':') {
        isParameter = true;
        parameterName = value.substr(1);
    }
}

Node::~Node() {
    for (auto& pair : children) {
        delete pair.second;
    }
}

Node* Node::insertChild(Node* node, const std::string& segment) {
    if (segment.empty()) {
        return node;
    }

    auto existing = node->children.find(segment);
    if (existing != node->children.end()) {
        return existing->second;
    }

    Node* child = new Node(segment);
    node->children[segment] = child;
    node->isLeaf = false;
    return child;
}

Node* Node::insert(Node* curr, const std::string& path, RouteHandler func) {
    std::vector<std::string> segments = Utils::split(path.c_str(), "/");
    if (path.empty() || segments.empty()) {
        curr->isLeaf = true;
        curr->handler = std::move(func);
        return curr;
    }

    Node* next = insertChild(curr, segments[0]);
    return insert(next, joinRemaining(segments), std::move(func));
}

Node* Node::find(Node* node, const std::string& path,
                 std::map<std::string, std::string>& params) {
    if (path.empty()) {
        return node;
    }

    std::vector<std::string> segments = Utils::split(path.c_str(), "/");
    if (segments.empty()) {
        return node;
    }

    const std::string& segment = segments[0];
    const std::string remaining = joinRemaining(segments);

    // A literal match always wins over a parameter match, so that /users/new
    // beats /users/:id when both are registered.
    auto literal = node->children.find(segment);
    if (literal != node->children.end()) {
        return find(literal->second, remaining, params);
    }

    for (const auto& child : node->children) {
        if (child.second->isParameter) {
            params[child.second->parameterName] = segment;
            return find(child.second, remaining, params);
        }
    }

    return nullptr;
}

Trie::Trie() : root(new Node()) {}

Trie::~Trie() {
    delete root;  // Node's destructor recurses into its children.
}

void Trie::insert(const std::string& path, RouteHandler handler) {
    root->insert(root, path, std::move(handler));
}

Node* Trie::searchNode(const std::string& path,
                       std::map<std::string, std::string>& params) const {
    return root->find(root, path, params);
}
