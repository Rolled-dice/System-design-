#include <iostream>
#include <string>
#include <vector>

class RoundRobinLB {
    std::vector<std::string> servers_;
    size_t idx_ = 0;
public:
    explicit RoundRobinLB(std::vector<std::string> s) : servers_(std::move(s)) {}
    std::string next() {
        if (servers_.empty()) return "";
        std::string s = servers_[idx_];
        idx_ = (idx_ + 1) % servers_.size();
        return s;
    }
};

int main() {
    RoundRobinLB lb({"s1", "s2", "s3"});
    for (int i = 0; i < 7; ++i) std::cout << "req " << i << " -> " << lb.next() << "\n";
    return 0;
}
