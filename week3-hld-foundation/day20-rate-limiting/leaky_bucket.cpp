#include <chrono>
#include <iostream>
#include <queue>
#include <thread>

class LeakyBucket {
    using Clock = std::chrono::steady_clock;
    size_t capacity_;
    double leakPerSec_;
    double waterLevel_ = 0;
    Clock::time_point last_;
public:
    LeakyBucket(size_t cap, double leak)
        : capacity_(cap), leakPerSec_(leak), last_(Clock::now()) {}

    bool offer() {
        auto now = Clock::now();
        double elapsed = std::chrono::duration<double>(now - last_).count();
        waterLevel_ = std::max(0.0, waterLevel_ - elapsed * leakPerSec_);
        last_ = now;
        if (waterLevel_ + 1.0 > capacity_) return false;
        waterLevel_ += 1.0;
        return true;
    }
};

int main() {
    LeakyBucket lb(5, 2.0);
    int allowed = 0, denied = 0;
    for (int i = 0; i < 12; ++i) {
        if (lb.offer()) ++allowed; else ++denied;
    }
    std::cout << "burst: allowed=" << allowed << " denied=" << denied << "\n";

    std::this_thread::sleep_for(std::chrono::seconds(1));
    allowed = denied = 0;
    for (int i = 0; i < 5; ++i) {
        if (lb.offer()) ++allowed; else ++denied;
    }
    std::cout << "after 1s leak: allowed=" << allowed << " denied=" << denied << "\n";
    return 0;
}
