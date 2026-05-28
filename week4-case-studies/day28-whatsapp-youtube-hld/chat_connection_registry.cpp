#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class ConnectionRegistry {
    std::unordered_map<std::string, std::string> userToServer_;
    std::unordered_map<std::string, int> serverLoad_;
    std::mutex mu_;
public:
    void connect(const std::string& userId, const std::string& serverId) {
        std::lock_guard lk(mu_);
        if (auto it = userToServer_.find(userId); it != userToServer_.end()) {
            serverLoad_[it->second]--;
        }
        userToServer_[userId] = serverId;
        serverLoad_[serverId]++;
    }

    void disconnect(const std::string& userId) {
        std::lock_guard lk(mu_);
        auto it = userToServer_.find(userId);
        if (it == userToServer_.end()) return;
        serverLoad_[it->second]--;
        userToServer_.erase(it);
    }

    std::optional<std::string> serverFor(const std::string& userId) {
        std::lock_guard lk(mu_);
        auto it = userToServer_.find(userId);
        if (it == userToServer_.end()) return std::nullopt;
        return it->second;
    }

    void status() {
        std::lock_guard lk(mu_);
        std::cout << "online users: " << userToServer_.size() << "\n";
        for (const auto& [s, n] : serverLoad_)
            std::cout << "  " << s << " -> " << n << " conns\n";
    }
};

class MessageRouter {
    ConnectionRegistry& reg_;
public:
    explicit MessageRouter(ConnectionRegistry& r) : reg_(r) {}
    void route(const std::string& from, const std::string& to, const std::string& msg) {
        auto srv = reg_.serverFor(to);
        if (!srv) {
            std::cout << "[router] " << to << " offline -> queue + push notification\n";
            return;
        }
        std::cout << "[router] " << from << " -> " << to << " via " << *srv << ": " << msg << "\n";
    }
};

int main() {
    ConnectionRegistry reg;
    reg.connect("alice", "edge-1");
    reg.connect("bob",   "edge-2");
    reg.connect("carol", "edge-1");
    reg.status();

    MessageRouter router(reg);
    router.route("alice", "bob", "hi");
    router.route("alice", "dave", "you there?");

    reg.disconnect("bob");
    router.route("alice", "bob", "are you back?");
    reg.status();
    return 0;
}
