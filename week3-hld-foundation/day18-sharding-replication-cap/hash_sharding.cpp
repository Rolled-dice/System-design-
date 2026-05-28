#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class HashShardedStore {
    std::vector<std::unordered_map<std::string, std::string>> shards_;
    size_t numShards_;
    size_t shardOf(const std::string& key) const {
        return std::hash<std::string>{}(key) % numShards_;
    }
public:
    explicit HashShardedStore(size_t n) : shards_(n), numShards_(n) {}
    void put(const std::string& k, const std::string& v) { shards_[shardOf(k)][k] = v; }
    std::string get(const std::string& k) const {
        const auto& s = shards_[shardOf(k)];
        auto it = s.find(k);
        return it == s.end() ? "" : it->second;
    }
    void distribution() const {
        for (size_t i = 0; i < shards_.size(); ++i)
            std::cout << "shard " << i << ": " << shards_[i].size() << " keys\n";
    }
};

int main() {
    HashShardedStore db(4);
    for (int i = 0; i < 100000; ++i) db.put("user:" + std::to_string(i), "x");
    db.distribution();
    std::cout << "user:42 -> " << db.get("user:42") << "\n";
    return 0;
}
