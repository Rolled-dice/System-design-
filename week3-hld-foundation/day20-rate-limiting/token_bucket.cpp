#include <chrono>
#include <iostream>
#include <thread>

class TokenBucket {
    using Clock = std::chrono::steady_clock;
    double capacity_;
    double tokens_;
    double refillPerSec_;
    Clock::time_point last_;
public:
    TokenBucket(double cap, double rate)
        : capacity_(cap), tokens_(cap), refillPerSec_(rate), last_(Clock::now()) {}

    bool tryAcquire(double n = 1.0) {
        auto now = Clock::now();
        double elapsed = std::chrono::duration<double>(now - last_).count();
        tokens_ = std::min(capacity_, tokens_ + elapsed * refillPerSec_);
        last_ = now;
        if (tokens_ >= n) { tokens_ -= n; return true; }
        return false;
    }
};

int main() {
    TokenBucket tb(5, 2.0);
    int allowed = 0, denied = 0;
    for (int i = 0; i < 10; ++i) {
        if (tb.tryAcquire()) ++allowed; else ++denied;
    }
    std::cout << "first burst: allowed=" << allowed << " denied=" << denied << "\n";

    std::this_thread::sleep_for(std::chrono::seconds(2));
    allowed = denied = 0;
    for (int i = 0; i < 10; ++i) {
        if (tb.tryAcquire()) ++allowed; else ++denied;
    }
    std::cout << "after 2s refill: allowed=" << allowed << " denied=" << denied << "\n";
    return 0;
}
