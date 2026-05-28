#include <iostream>
#include <list>
#include <stdexcept>
#include <unordered_map>

template <typename K, typename V>
class LRUCache {
    size_t cap_;
    std::list<std::pair<K, V>> items_;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> index_;
public:
    explicit LRUCache(size_t cap) : cap_(cap) {}

    bool get(const K& key, V& out) {
        auto it = index_.find(key);
        if (it == index_.end()) return false;
        items_.splice(items_.begin(), items_, it->second);
        out = it->second->second;
        return true;
    }

    void put(const K& key, V value) {
        auto it = index_.find(key);
        if (it != index_.end()) {
            it->second->second = std::move(value);
            items_.splice(items_.begin(), items_, it->second);
            return;
        }
        if (items_.size() == cap_) {
            index_.erase(items_.back().first);
            items_.pop_back();
        }
        items_.emplace_front(key, std::move(value));
        index_[key] = items_.begin();
    }
};

int main() {
    LRUCache<int, std::string> c(3);
    c.put(1, "a"); c.put(2, "b"); c.put(3, "c");
    std::string v;
    c.get(1, v); std::cout << "get(1)=" << v << "\n";
    c.put(4, "d");
    if (!c.get(2, v)) std::cout << "2 evicted\n";
    c.get(3, v); std::cout << "get(3)=" << v << "\n";
    return 0;
}
