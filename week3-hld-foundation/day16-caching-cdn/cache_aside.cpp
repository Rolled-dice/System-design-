#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

class UserDb {
    std::unordered_map<int, std::string> data_ = {
        {1, "Alice"}, {2, "Bob"}, {3, "Carol"}};
    int hits_ = 0;
public:
    std::optional<std::string> findById(int id) {
        ++hits_;
        std::cout << "  [DB hit #" << hits_ << "] id=" << id << "\n";
        auto it = data_.find(id);
        return it != data_.end() ? std::optional<std::string>(it->second) : std::nullopt;
    }
};

class UserCache {
    std::unordered_map<int, std::string> store_;
public:
    std::optional<std::string> get(int id) const {
        auto it = store_.find(id);
        return it != store_.end() ? std::optional<std::string>(it->second) : std::nullopt;
    }
    void put(int id, std::string v) { store_[id] = std::move(v); }
    void invalidate(int id) { store_.erase(id); }
};

class UserService {
    UserDb& db_;
    UserCache& cache_;
public:
    UserService(UserDb& d, UserCache& c) : db_(d), cache_(c) {}
    std::optional<std::string> getUser(int id) {
        if (auto v = cache_.get(id)) {
            std::cout << "[cache] id=" << id << " -> " << *v << "\n";
            return v;
        }
        auto v = db_.findById(id);
        if (v) cache_.put(id, *v);
        return v;
    }
};

int main() {
    UserDb db;
    UserCache cache;
    UserService svc(db, cache);
    svc.getUser(1);
    svc.getUser(1);
    svc.getUser(2);
    svc.getUser(1);
    return 0;
}
