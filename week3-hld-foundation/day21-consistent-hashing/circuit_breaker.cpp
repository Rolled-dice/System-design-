#include <chrono>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>

enum class CBState { CLOSED, OPEN, HALF_OPEN };

class CircuitBreaker {
    using Clock = std::chrono::steady_clock;
    int failureThreshold_;
    std::chrono::milliseconds openDuration_;
    int consecutiveFailures_ = 0;
    CBState state_ = CBState::CLOSED;
    Clock::time_point openedAt_;

    static const char* name(CBState s) {
        switch (s) { case CBState::CLOSED: return "CLOSED"; case CBState::OPEN: return "OPEN"; case CBState::HALF_OPEN: return "HALF_OPEN"; }
        return "?";
    }
    void transition(CBState s) {
        if (state_ != s) std::cout << "  [CB] " << name(state_) << " -> " << name(s) << "\n";
        state_ = s;
        if (s == CBState::OPEN) openedAt_ = Clock::now();
    }
public:
    CircuitBreaker(int t, std::chrono::milliseconds d)
        : failureThreshold_(t), openDuration_(d) {}

    bool execute(const std::function<bool()>& fn) {
        if (state_ == CBState::OPEN) {
            if (Clock::now() - openedAt_ < openDuration_) {
                std::cout << "  [CB] short-circuited (OPEN)\n";
                return false;
            }
            transition(CBState::HALF_OPEN);
        }
        try {
            bool ok = fn();
            if (ok) {
                consecutiveFailures_ = 0;
                if (state_ == CBState::HALF_OPEN) transition(CBState::CLOSED);
                return true;
            }
            consecutiveFailures_++;
        } catch (...) {
            consecutiveFailures_++;
        }
        if (state_ == CBState::HALF_OPEN || consecutiveFailures_ >= failureThreshold_)
            transition(CBState::OPEN);
        return false;
    }
};

int main() {
    int callIdx = 0;
    auto flaky = [&]() {
        ++callIdx;
        bool ok = (callIdx > 8);
        std::cout << "  call#" << callIdx << " -> " << (ok ? "OK" : "FAIL") << "\n";
        return ok;
    };

    CircuitBreaker cb(3, std::chrono::milliseconds(300));
    for (int i = 0; i < 5; ++i) cb.execute(flaky);
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    for (int i = 0; i < 5; ++i) cb.execute(flaky);
    return 0;
}
