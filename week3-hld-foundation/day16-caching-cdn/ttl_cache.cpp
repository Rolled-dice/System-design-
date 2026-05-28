#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

template <typename K, typename V>
class TTLCache {
    using Clock = std::chrono::steady_clock;
    struct Entry { V value; Clock::time_point expiry; };
    std::unordered_map<K, Entry> store_;
public:
    void put(const K& k, V v, std::chrono::milliseconds ttl) {
        store_[k] = {std::move(v), Clock::now() + ttl};
    }
    std::optional<V> get(const K& k) {
        auto it = store_.find(k);
        if (it == store_.end()) return std::nullopt;
        if (Clock::now() > it->second.expiry) {
            store_.erase(it);
            return std::nullopt;
        }
        return it->second.value;
    }
    void sweep() {
        auto now = Clock::now();
        for (auto it = store_.begin(); it != store_.end(); ) {
            if (now > it->second.expiry) it = store_.erase(it);
            else ++it;
        }
    }
    size_t size() const { return store_.size(); }
};

int main() {
    TTLCache<std::string, int> c;
    c.put("a", 1, std::chrono::milliseconds(200));
    c.put("b", 2, std::chrono::milliseconds(800));

    std::cout << "a=" << c.get("a").value_or(-1) << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "after 300ms, a=" << c.get("a").value_or(-1) << " b=" << c.get("b").value_or(-1) << "\n";
    c.sweep();
    std::cout << "size after sweep: " << c.size() << "\n";
    return 0;
}
