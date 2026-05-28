#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

struct WriteOp { std::string key; std::string val; long lsn; };

class Replica {
    std::unordered_map<std::string, std::string> store_;
    long lastLsn_ = 0;
public:
    long lastLsn() const { return lastLsn_; }
    void apply(const WriteOp& op) {
        store_[op.key] = op.val;
        lastLsn_ = op.lsn;
    }
    std::string get(const std::string& k) const {
        auto it = store_.find(k);
        return it == store_.end() ? "" : it->second;
    }
};

class Cluster {
    Replica leader_;
    std::vector<Replica> followers_;
    std::queue<WriteOp> log_;
    long nextLsn_ = 1;
public:
    explicit Cluster(int nFollowers) : followers_(nFollowers) {}

    void write(const std::string& k, const std::string& v) {
        WriteOp op{k, v, nextLsn_++};
        leader_.apply(op);
        log_.push(op);
        std::cout << "[leader] write " << k << "=" << v << " lsn=" << op.lsn << "\n";
    }

    void replicateOne() {
        if (log_.empty()) return;
        auto op = log_.front(); log_.pop();
        for (size_t i = 0; i < followers_.size(); ++i) {
            followers_[i].apply(op);
            std::cout << "  [replica " << i << "] applied lsn=" << op.lsn << "\n";
        }
    }

    std::string readFromLeader(const std::string& k) { return leader_.get(k); }
    std::string readFromReplica(size_t i, const std::string& k) { return followers_[i].get(k); }
};

int main() {
    Cluster c(2);
    c.write("user:1", "Alice");
    c.write("user:2", "Bob");

    std::cout << "leader user:1 = " << c.readFromLeader("user:1") << "\n";
    std::cout << "replica0 user:1 (before replicate) = '" << c.readFromReplica(0, "user:1") << "'\n";

    c.replicateOne();
    c.replicateOne();

    std::cout << "replica0 user:1 (after) = " << c.readFromReplica(0, "user:1") << "\n";
    return 0;
}
