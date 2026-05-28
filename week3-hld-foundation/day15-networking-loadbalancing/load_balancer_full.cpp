#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct Backend {
    std::string id;
    int weight = 1;
    bool healthy = true;
    int activeConns = 0;
};

class LbStrategy {
public:
    virtual ~LbStrategy() = default;
    virtual Backend* pick(std::vector<Backend>& pool) = 0;
};

class RoundRobinStrategy : public LbStrategy {
    size_t idx_ = 0;
public:
    Backend* pick(std::vector<Backend>& pool) override {
        for (size_t i = 0; i < pool.size(); ++i) {
            Backend& b = pool[idx_ % pool.size()];
            ++idx_;
            if (b.healthy) return &b;
        }
        return nullptr;
    }
};

class LeastConnStrategy : public LbStrategy {
public:
    Backend* pick(std::vector<Backend>& pool) override {
        Backend* best = nullptr;
        for (auto& b : pool) {
            if (!b.healthy) continue;
            if (!best || b.activeConns < best->activeConns) best = &b;
        }
        return best;
    }
};

class LoadBalancer {
    std::vector<Backend> pool_;
    std::unique_ptr<LbStrategy> strategy_;
public:
    LoadBalancer(std::vector<Backend> p, std::unique_ptr<LbStrategy> s)
        : pool_(std::move(p)), strategy_(std::move(s)) {}

    std::string route() {
        Backend* b = strategy_->pick(pool_);
        if (!b) return "(no backend)";
        b->activeConns++;
        return b->id;
    }
    void releaseConn(const std::string& id) {
        for (auto& b : pool_) if (b.id == id) { b.activeConns--; break; }
    }
    void markUnhealthy(const std::string& id) {
        for (auto& b : pool_) if (b.id == id) { b.healthy = false; break; }
    }
    void distribution(int n) {
        std::unordered_map<std::string, int> hits;
        for (int i = 0; i < n; ++i) hits[route()]++;
        for (const auto& [k, v] : hits) std::cout << k << " -> " << v << "\n";
    }
};

int main() {
    std::vector<Backend> pool = {{"s1"}, {"s2"}, {"s3"}};
    LoadBalancer lb(pool, std::make_unique<RoundRobinStrategy>());
    lb.distribution(9);

    std::cout << "--- after marking s2 unhealthy ---\n";
    LoadBalancer lb2({{"s1"}, {"s2"}, {"s3"}}, std::make_unique<RoundRobinStrategy>());
    lb2.markUnhealthy("s2");
    lb2.distribution(9);
    return 0;
}
