#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

class InMemoryKV {
    using Clock = std::chrono::steady_clock;
    struct Entry { std::string val; std::optional<Clock::time_point> expiry; };
    std::unordered_map<std::string, Entry> store_;

    bool expired(const Entry& e) const {
        return e.expiry && Clock::now() > *e.expiry;
    }
public:
    void set(const std::string& k, const std::string& v) { store_[k] = {v, std::nullopt}; }
    void setex(const std::string& k, const std::string& v, std::chrono::milliseconds ttl) {
        store_[k] = {v, Clock::now() + ttl};
    }
    std::optional<std::string> get(const std::string& k) {
        auto it = store_.find(k);
        if (it == store_.end()) return std::nullopt;
        if (expired(it->second)) { store_.erase(it); return std::nullopt; }
        return it->second.val;
    }
    bool del(const std::string& k) { return store_.erase(k) > 0; }
    bool expire(const std::string& k, std::chrono::milliseconds ttl) {
        auto it = store_.find(k);
        if (it == store_.end()) return false;
        it->second.expiry = Clock::now() + ttl;
        return true;
    }
};

int main() {
    InMemoryKV db;
    db.set("user:1", "Alice");
    db.setex("session:1", "abc", std::chrono::milliseconds(150));
    std::cout << "session:1 = " << db.get("session:1").value_or("(nil)") << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "session:1 after 200ms = " << db.get("session:1").value_or("(nil)") << "\n";
    std::cout << "user:1 = " << db.get("user:1").value_or("(nil)") << "\n";
    return 0;
}
