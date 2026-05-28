#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class PubSub {
public:
    using Handler = std::function<void(const std::string&)>;
    using SubId = int;

    SubId subscribe(const std::string& topic, Handler h) {
        SubId id = nextId_++;
        topics_[topic].push_back({id, std::move(h)});
        return id;
    }

    void unsubscribe(const std::string& topic, SubId id) {
        auto& subs = topics_[topic];
        for (auto it = subs.begin(); it != subs.end(); ++it)
            if (it->id == id) { subs.erase(it); return; }
    }

    void publish(const std::string& topic, const std::string& msg) {
        auto it = topics_.find(topic);
        if (it == topics_.end()) return;
        for (auto& s : it->second) s.handler(msg);
    }

private:
    struct Sub { SubId id; Handler handler; };
    std::unordered_map<std::string, std::vector<Sub>> topics_;
    SubId nextId_ = 1;
};

int main() {
    PubSub bus;
    bus.subscribe("orders", [](const std::string& m) { std::cout << "[email] " << m << "\n"; });
    bus.subscribe("orders", [](const std::string& m) { std::cout << "[ship]  " << m << "\n"; });
    bus.subscribe("audit",  [](const std::string& m) { std::cout << "[audit] " << m << "\n"; });

    bus.publish("orders", "order#42 placed");
    bus.publish("audit",  "user#5 login");
    return 0;
}
