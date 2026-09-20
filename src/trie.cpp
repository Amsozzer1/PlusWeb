#include <PlusWeb/trie.h>
#include <PlusWeb/utils.h>

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
    delete paramChild;
}

Node* Node::insertChild(Node* node, const std::string& segment) {
    if (segment.empty()) {
        return node;
    }

    if (segment[0] == ':') {
        // One parameter child per node. Re-registering a different name under
        // the same node reuses the existing slot, matching the old behaviour of
        // the first parameter encountered winning.
        if (!node->paramChild) {
            node->paramChild = new Node(segment);
            node->isLeaf = false;
        }
        return node->paramChild;
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
    const std::vector<std::string> segments = Utils::split(path.c_str(), "/");

    Node* node = curr;
    for (const std::string& segment : segments) {
        node = insertChild(node, segment);
    }
    node->isLeaf = true;
    node->handler = std::move(func);
    return node;
}

// Walks the pre-split segments by index, so no substring of the path is rebuilt
// at any level. Returns a node that actually carries a handler, which is what
// lets the caller above backtrack on failure.
Node* Node::findFrom(Node* node, const std::vector<std::string>& segments, size_t index,
                     std::map<std::string, std::string>& params) {
    if (index == segments.size()) {
        return node->handler ? node : nullptr;
    }

    const std::string& segment = segments[index];

    // A literal match wins over a parameter, so that /users/new beats
    // /users/:id when both are registered.
    auto literal = node->children.find(segment);
    if (literal != node->children.end()) {
        if (Node* hit = findFrom(literal->second, segments, index + 1, params)) {
            return hit;
        }
    }

    // The literal branch led nowhere, so try the parameter branch. Binding is
    // undone on failure so a dead end cannot leave a stale param behind.
    if (node->paramChild) {
        const std::string& name = node->paramChild->parameterName;
        auto previous = params.find(name);
        const bool had = previous != params.end();
        const std::string saved = had ? previous->second : std::string();

        params[name] = segment;
        if (Node* hit = findFrom(node->paramChild, segments, index + 1, params)) {
            return hit;
        }
        if (had) {
            params[name] = saved;
        } else {
            params.erase(name);
        }
    }

    return nullptr;
}

Node* Node::find(Node* node, const std::string& path,
                 std::map<std::string, std::string>& params) {
    return findFrom(node, Utils::split(path.c_str(), "/"), 0, params);
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
