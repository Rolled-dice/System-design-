#include <iostream>
#include <list>
#include <unordered_map>

template <typename K, typename V>
class LFUCache {
    struct Node { K key; V value; int freq; };
    size_t cap_;
    int minFreq_ = 0;
    std::unordered_map<K, typename std::list<Node>::iterator> keyMap_;
    std::unordered_map<int, std::list<Node>> freqMap_;

    void touch(typename std::list<Node>::iterator it) {
        Node n = *it;
        freqMap_[n.freq].erase(it);
        if (freqMap_[n.freq].empty()) {
            freqMap_.erase(n.freq);
            if (minFreq_ == n.freq) ++minFreq_;
        }
        n.freq++;
        freqMap_[n.freq].push_front(n);
        keyMap_[n.key] = freqMap_[n.freq].begin();
    }

public:
    explicit LFUCache(size_t c) : cap_(c) {}

    bool get(const K& k, V& out) {
        auto it = keyMap_.find(k);
        if (it == keyMap_.end()) return false;
        out = it->second->value;
        touch(it->second);
        return true;
    }

    void put(const K& k, V v) {
        if (cap_ == 0) return;
        auto it = keyMap_.find(k);
        if (it != keyMap_.end()) {
            it->second->value = std::move(v);
            touch(it->second);
            return;
        }
        if (keyMap_.size() == cap_) {
            auto& lst = freqMap_[minFreq_];
            keyMap_.erase(lst.back().key);
            lst.pop_back();
            if (lst.empty()) freqMap_.erase(minFreq_);
        }
        minFreq_ = 1;
        freqMap_[1].push_front({k, std::move(v), 1});
        keyMap_[k] = freqMap_[1].begin();
    }
};

int main() {
    LFUCache<int, std::string> c(2);
    c.put(1, "a"); c.put(2, "b");
    std::string v;
    c.get(1, v); c.get(1, v);
    c.put(3, "c");
    std::cout << "2 evicted? " << (c.get(2, v) ? "no" : "yes") << "\n";
    std::cout << "1 still? "  << (c.get(1, v) ? "yes (" + v + ")" : "no") << "\n";
    return 0;
}
