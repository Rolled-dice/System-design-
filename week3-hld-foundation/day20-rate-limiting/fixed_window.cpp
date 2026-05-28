#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>

class FixedWindowLimiter {
    using Clock = std::chrono::steady_clock;
    int limit_;
    std::chrono::milliseconds window_;
    struct Bucket { int count = 0; Clock::time_point start; };
    std::unordered_map<std::string, Bucket> buckets_;
public:
    FixedWindowLimiter(int limit, std::chrono::milliseconds window)
        : limit_(limit), window_(window) {}

    bool allow(const std::string& userId) {
        auto& b = buckets_[userId];
        auto now = Clock::now();
        if (b.count == 0 || now - b.start >= window_) {
            b.start = now; b.count = 0;
        }
        if (b.count < limit_) { b.count++; return true; }
        return false;
    }
};

int main() {
    FixedWindowLimiter lim(3, std::chrono::milliseconds(500));
    for (int i = 0; i < 6; ++i)
        std::cout << "req" << i << " -> " << (lim.allow("u1") ? "OK" : "429") << "\n";
    return 0;
}
