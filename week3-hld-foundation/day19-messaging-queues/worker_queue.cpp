#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

template <typename T>
class BlockingQueue {
    std::queue<T> q_;
    std::mutex mu_;
    std::condition_variable notEmpty_, notFull_;
    size_t cap_;
    bool closed_ = false;
public:
    explicit BlockingQueue(size_t cap) : cap_(cap) {}

    void push(T v) {
        std::unique_lock lk(mu_);
        notFull_.wait(lk, [&]{ return q_.size() < cap_ || closed_; });
        if (closed_) return;
        q_.push(std::move(v));
        notEmpty_.notify_one();
    }

    bool pop(T& out) {
        std::unique_lock lk(mu_);
        notEmpty_.wait(lk, [&]{ return !q_.empty() || closed_; });
        if (q_.empty()) return false;
        out = std::move(q_.front()); q_.pop();
        notFull_.notify_one();
        return true;
    }

    void close() {
        std::lock_guard lk(mu_);
        closed_ = true;
        notEmpty_.notify_all();
        notFull_.notify_all();
    }
};

int main() {
    BlockingQueue<std::string> q(5);
    std::atomic<int> processed{0};

    std::vector<std::thread> workers;
    for (int i = 0; i < 3; ++i) {
        workers.emplace_back([&, id = i] {
            std::string job;
            while (q.pop(job)) {
                std::cout << "[w" << id << "] processing " << job << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                processed++;
            }
        });
    }

    for (int i = 0; i < 12; ++i) q.push("job#" + std::to_string(i));
    q.close();

    for (auto& t : workers) t.join();
    std::cout << "processed: " << processed << "\n";
    return 0;
}
