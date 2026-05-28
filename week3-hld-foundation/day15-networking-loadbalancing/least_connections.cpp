#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class LeastConnLB {
    std::vector<std::string> servers_;
    std::unordered_map<std::string, int> conns_;
public:
    explicit LeastConnLB(std::vector<std::string> s) : servers_(std::move(s)) {
        for (const auto& x : servers_) conns_[x] = 0;
    }
    std::string acquire() {
        std::string best = servers_.front();
        for (const auto& s : servers_)
            if (conns_[s] < conns_[best]) best = s;
        conns_[best]++;
        return best;
    }
    void release(const std::string& s) { conns_[s]--; }
    void status() const {
        for (const auto& [s, c] : conns_) std::cout << s << "=" << c << " ";
        std::cout << "\n";
    }
};

int main() {
    LeastConnLB lb({"s1", "s2", "s3"});
    auto a = lb.acquire();
    auto b = lb.acquire();
    auto c = lb.acquire();
    auto d = lb.acquire();
    lb.status();
    lb.release(a);
    lb.release(c);
    lb.status();
    return 0;
}
