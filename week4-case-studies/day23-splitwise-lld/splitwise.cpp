#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct User { std::string id; std::string name; };

struct Split { std::string userId; double amount; };

class SplitStrategy {
public:
    virtual ~SplitStrategy() = default;
    virtual std::vector<Split> split(double total, const std::vector<std::string>& userIds,
                                     const std::vector<double>& params) = 0;
};

class EqualSplit : public SplitStrategy {
public:
    std::vector<Split> split(double total, const std::vector<std::string>& userIds,
                             const std::vector<double>&) override {
        std::vector<Split> out;
        double share = total / userIds.size();
        for (const auto& u : userIds) out.push_back({u, share});
        return out;
    }
};

class ExactSplit : public SplitStrategy {
public:
    std::vector<Split> split(double total, const std::vector<std::string>& userIds,
                             const std::vector<double>& amounts) override {
        std::vector<Split> out;
        double sum = 0;
        for (size_t i = 0; i < userIds.size(); ++i) {
            out.push_back({userIds[i], amounts[i]});
            sum += amounts[i];
        }
        if (std::abs(sum - total) > 0.01) throw std::runtime_error("ExactSplit: sums mismatch");
        return out;
    }
};

class PercentSplit : public SplitStrategy {
public:
    std::vector<Split> split(double total, const std::vector<std::string>& userIds,
                             const std::vector<double>& pcts) override {
        std::vector<Split> out;
        double sum = 0;
        for (double p : pcts) sum += p;
        if (std::abs(sum - 100.0) > 0.01) throw std::runtime_error("Percent must sum to 100");
        for (size_t i = 0; i < userIds.size(); ++i)
            out.push_back({userIds[i], total * pcts[i] / 100.0});
        return out;
    }
};

class ExpenseManager {
    std::unordered_map<std::string, User> users_;
    std::unordered_map<std::string, std::unordered_map<std::string, double>> balance_;
public:
    void addUser(const User& u) { users_[u.id] = u; }

    void addExpense(const std::string& payer, double total,
                    const std::vector<std::string>& participants,
                    SplitStrategy& strat,
                    const std::vector<double>& params = {}) {
        auto splits = strat.split(total, participants, params);
        for (const auto& s : splits) {
            if (s.userId == payer) continue;
            balance_[s.userId][payer] += s.amount;
            balance_[payer][s.userId] -= s.amount;
        }
    }

    void showBalance(const std::string& uid) {
        std::cout << "Balance of " << users_[uid].name << ":\n";
        auto it = balance_.find(uid);
        if (it == balance_.end()) { std::cout << "  (clear)\n"; return; }
        for (const auto& [other, amt] : it->second) {
            if (amt > 0.01) std::cout << "  owes " << users_[other].name << ": " << amt << "\n";
            else if (amt < -0.01) std::cout << "  gets from " << users_[other].name << ": " << -amt << "\n";
        }
    }
};

int main() {
    ExpenseManager mgr;
    mgr.addUser({"u1", "Alice"});
    mgr.addUser({"u2", "Bob"});
    mgr.addUser({"u3", "Carol"});

    EqualSplit eq;
    mgr.addExpense("u1", 900, {"u1", "u2", "u3"}, eq);

    PercentSplit pct;
    mgr.addExpense("u2", 1000, {"u1", "u2", "u3"}, pct, {30, 30, 40});

    ExactSplit exact;
    mgr.addExpense("u3", 600, {"u1", "u2", "u3"}, exact, {200, 100, 300});

    mgr.showBalance("u1");
    mgr.showBalance("u2");
    mgr.showBalance("u3");
    return 0;
}
