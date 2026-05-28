#include <chrono>
#include <deque>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>

class SlidingWindowLimiter {
    using Clock = std::chrono::steady_clock;
    int limit_;
    std::chrono::milliseconds window_;
    std::unordered_map<std::string, std::deque<Clock::time_point>> log_;
public:
    SlidingWindowLimiter(int limit, std::chrono::milliseconds window)
        : limit_(limit), window_(window) {}

    bool allow(const std::string& userId) {
        auto now = Clock::now();
        auto& dq = log_[userId];
        while (!dq.empty() && now - dq.front() >= window_) dq.pop_front();
        if ((int)dq.size() < limit_) { dq.push_back(now); return true; }
        return false;
    }
};

int main() {
    SlidingWindowLimiter lim(3, std::chrono::milliseconds(500));
    for (int i = 0; i < 5; ++i)
        std::cout << "req" << i << " -> " << (lim.allow("u1") ? "OK" : "429") << "\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    std::cout << "after 600ms\n";
    for (int i = 0; i < 4; ++i)
        std::cout << "req" << i << " -> " << (lim.allow("u1") ? "OK" : "429") << "\n";
    return 0;
}
