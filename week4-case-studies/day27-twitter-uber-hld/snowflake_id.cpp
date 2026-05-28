#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>

class Snowflake {
    static constexpr int MACHINE_BITS = 10;
    static constexpr int SEQ_BITS     = 12;
    static constexpr int MAX_MACHINE  = (1 << MACHINE_BITS) - 1;
    static constexpr int MAX_SEQ      = (1 << SEQ_BITS) - 1;
    static constexpr long EPOCH_MS    = 1577836800000L;

    int machineId_;
    long lastMs_ = -1;
    int seq_ = 0;
    std::mutex mu_;

    static long nowMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
public:
    explicit Snowflake(int machineId) : machineId_(machineId) {
        if (machineId < 0 || machineId > MAX_MACHINE) throw std::invalid_argument("machineId");
    }

    uint64_t next() {
        std::lock_guard lk(mu_);
        long ms = nowMs();
        if (ms < lastMs_) throw std::runtime_error("clock moved backwards");
        if (ms == lastMs_) {
            seq_ = (seq_ + 1) & MAX_SEQ;
            if (seq_ == 0) {
                while ((ms = nowMs()) <= lastMs_) {}
            }
        } else {
            seq_ = 0;
        }
        lastMs_ = ms;
        uint64_t id = ((uint64_t)(ms - EPOCH_MS) << (MACHINE_BITS + SEQ_BITS)) |
                      ((uint64_t)machineId_       << SEQ_BITS) |
                      (uint64_t)seq_;
        return id;
    }
};

int main() {
    Snowflake gen(42);
    uint64_t prev = 0;
    for (int i = 0; i < 5; ++i) {
        uint64_t id = gen.next();
        std::cout << "id=" << id << " sortable=" << (id > prev) << "\n";
        prev = id;
    }
    return 0;
}
