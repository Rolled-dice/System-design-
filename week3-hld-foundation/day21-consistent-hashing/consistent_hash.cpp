#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class ConsistentHash {
    int virtualReplicas_;
    std::map<size_t, std::string> ring_;
    std::set<std::string> nodes_;

    size_t hash(const std::string& s) const {
        return std::hash<std::string>{}(s);
    }
public:
    explicit ConsistentHash(int v = 100) : virtualReplicas_(v) {}

    void addNode(const std::string& node) {
        nodes_.insert(node);
        for (int i = 0; i < virtualReplicas_; ++i) {
            size_t h = hash(node + "#" + std::to_string(i));
            ring_[h] = node;
        }
    }

    void removeNode(const std::string& node) {
        nodes_.erase(node);
        for (int i = 0; i < virtualReplicas_; ++i) {
            size_t h = hash(node + "#" + std::to_string(i));
            ring_.erase(h);
        }
    }

    std::string getNode(const std::string& key) const {
        if (ring_.empty()) return "";
        size_t h = hash(key);
        auto it = ring_.lower_bound(h);
        if (it == ring_.end()) it = ring_.begin();
        return it->second;
    }
};

int main() {
    ConsistentHash ch(50);
    for (const auto& n : {"node-A", "node-B", "node-C", "node-D"}) ch.addNode(n);

    std::vector<std::string> keys;
    for (int i = 0; i < 10000; ++i) keys.push_back("key-" + std::to_string(i));

    std::unordered_map<std::string, std::string> before;
    for (const auto& k : keys) before[k] = ch.getNode(k);

    ch.removeNode("node-B");

    int migrated = 0;
    for (const auto& k : keys) if (ch.getNode(k) != before[k]) ++migrated;
    std::cout << "After removing node-B: " << migrated << "/" << keys.size()
              << " keys migrated (~25% expected)\n";
    return 0;
}
