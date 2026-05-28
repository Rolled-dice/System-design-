#include <iostream>
#include <string>
#include <vector>

struct Server { std::string name; int weight; };

class WeightedRR {
    std::vector<Server> servers_;
    std::vector<int> currentWeights_;
public:
    explicit WeightedRR(std::vector<Server> s) : servers_(std::move(s)),
                                                 currentWeights_(servers_.size(), 0) {}
    std::string next() {
        int totalWeight = 0;
        int best = -1;
        for (size_t i = 0; i < servers_.size(); ++i) {
            currentWeights_[i] += servers_[i].weight;
            totalWeight += servers_[i].weight;
            if (best == -1 || currentWeights_[i] > currentWeights_[best]) best = static_cast<int>(i);
        }
        currentWeights_[best] -= totalWeight;
        return servers_[best].name;
    }
};

int main() {
    WeightedRR lb({{"s1", 5}, {"s2", 1}, {"s3", 1}});
    for (int i = 0; i < 14; ++i) std::cout << lb.next() << " ";
    std::cout << "\n";
    return 0;
}
