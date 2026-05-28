#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class ShardedCounter {
    static constexpr int SHARDS = 16;
    std::vector<std::atomic<long>> shards_;
public:
    ShardedCounter() : shards_(SHARDS) {
        for (auto& s : shards_) s.store(0);
    }

    void incr(const std::string& key) {
        size_t i = std::hash<std::string>{}(key) % SHARDS;
        shards_[i].fetch_add(1, std::memory_order_relaxed);
    }

    long flush() {
        long total = 0;
        for (auto& s : shards_)
            total += s.exchange(0, std::memory_order_relaxed);
        return total;
    }
};

class ViewService {
    std::unordered_map<std::string, ShardedCounter> perVideo_;
    std::unordered_map<std::string, long> persisted_;
public:
    void recordView(const std::string& videoId, const std::string& userId) {
        perVideo_[videoId].incr(userId);
    }

    void flushAll() {
        for (auto& [vid, ctr] : perVideo_) {
            long delta = ctr.flush();
            if (delta > 0) {
                persisted_[vid] += delta;
                std::cout << "[flush] " << vid << " += " << delta << " (total=" << persisted_[vid] << ")\n";
            }
        }
    }

    long get(const std::string& videoId) const {
        auto it = persisted_.find(videoId);
        return it == persisted_.end() ? 0 : it->second;
    }
};

int main() {
    ViewService svc;

    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < 1000; ++i) {
                svc.recordView("v1", "user" + std::to_string(t * 1000 + i));
                if (i % 3 == 0) svc.recordView("v2", "user" + std::to_string(i));
            }
        });
    }
    for (auto& th : threads) th.join();

    svc.flushAll();
    std::cout << "v1 total: " << svc.get("v1") << " (expected 8000)\n";
    std::cout << "v2 total: " << svc.get("v2") << "\n";
    return 0;
}
