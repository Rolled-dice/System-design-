#include <iostream>
#include <map>
#include <string>
#include <vector>

class RangeShard {
    std::map<std::string, std::string> data_;
public:
    void put(const std::string& k, const std::string& v) { data_[k] = v; }
    std::string get(const std::string& k) const {
        auto it = data_.find(k);
        return it == data_.end() ? "" : it->second;
    }
    void scan(const std::string& lo, const std::string& hi) const {
        for (auto it = data_.lower_bound(lo); it != data_.end() && it->first <= hi; ++it)
            std::cout << "  " << it->first << "=" << it->second << "\n";
    }
    size_t size() const { return data_.size(); }
};

class RangeShardedDB {
    std::vector<std::pair<char, RangeShard>> shards_;
public:
    RangeShardedDB() {
        shards_.push_back({'h', {}});
        shards_.push_back({'p', {}});
        shards_.push_back({'z', {}});
    }
    RangeShard& shardFor(const std::string& key) {
        for (auto& [boundary, s] : shards_)
            if (key[0] <= boundary) return s;
        return shards_.back().second;
    }
    void put(const std::string& k, const std::string& v) { shardFor(k).put(k, v); }
    void status() {
        for (size_t i = 0; i < shards_.size(); ++i)
            std::cout << "shard " << i << " (<= '" << shards_[i].first << "') size=" << shards_[i].second.size() << "\n";
    }
};

int main() {
    RangeShardedDB db;
    for (const auto& s : {"alice", "bob", "carol", "dave", "isaac", "joe", "mary", "noah", "paul", "rita", "sara", "yara", "zane"})
        db.put(s, s);
    db.status();
    return 0;
}
