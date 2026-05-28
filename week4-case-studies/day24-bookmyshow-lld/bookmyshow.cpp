#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class SeatState { AVAILABLE, LOCKED, BOOKED };
enum class SeatType  { STANDARD, PREMIUM, RECLINER };

struct Seat {
    std::string id;
    SeatType type;
    SeatState state = SeatState::AVAILABLE;
    std::string lockedBy;
    std::chrono::steady_clock::time_point lockExpiry;
};

class Show {
    std::string id_;
    std::string movie_;
    std::unordered_map<std::string, Seat> seats_;
    std::mutex mu_;
public:
    Show(std::string id, std::string movie, int rows, int cols) : id_(std::move(id)), movie_(std::move(movie)) {
        for (int r = 1; r <= rows; ++r)
            for (int c = 1; c <= cols; ++c) {
                std::string sid = std::string(1, char('A' + r - 1)) + std::to_string(c);
                SeatType t = (r <= 1) ? SeatType::RECLINER : (r <= 3 ? SeatType::PREMIUM : SeatType::STANDARD);
                seats_[sid] = {sid, t};
            }
    }
    const std::string& id() const { return id_; }
    const std::string& movie() const { return movie_; }

    bool lockSeats(const std::vector<std::string>& ids, const std::string& userId) {
        std::lock_guard lk(mu_);
        auto now = std::chrono::steady_clock::now();
        for (const auto& sid : ids) {
            auto it = seats_.find(sid);
            if (it == seats_.end()) return false;
            auto& s = it->second;
            if (s.state == SeatState::AVAILABLE) continue;
            if (s.state == SeatState::LOCKED && now > s.lockExpiry) continue;
            return false;
        }
        for (const auto& sid : ids) {
            auto& s = seats_[sid];
            s.state = SeatState::LOCKED;
            s.lockedBy = userId;
            s.lockExpiry = now + std::chrono::minutes(5);
        }
        return true;
    }

    bool confirm(const std::vector<std::string>& ids, const std::string& userId) {
        std::lock_guard lk(mu_);
        auto now = std::chrono::steady_clock::now();
        for (const auto& sid : ids) {
            auto it = seats_.find(sid);
            if (it == seats_.end()) return false;
            auto& s = it->second;
            if (s.state != SeatState::LOCKED || s.lockedBy != userId || now > s.lockExpiry) return false;
        }
        for (const auto& sid : ids) seats_[sid].state = SeatState::BOOKED;
        return true;
    }

    void cancel(const std::vector<std::string>& ids, const std::string& userId) {
        std::lock_guard lk(mu_);
        for (const auto& sid : ids) {
            auto& s = seats_[sid];
            if (s.lockedBy == userId) { s.state = SeatState::AVAILABLE; s.lockedBy.clear(); }
        }
    }

    void layout() {
        std::lock_guard lk(mu_);
        std::cout << "Show " << id_ << " (" << movie_ << "):\n";
        for (const auto& [k, v] : seats_) {
            char m = (v.state == SeatState::AVAILABLE ? '.' : v.state == SeatState::LOCKED ? 'L' : 'X');
            std::cout << k << "(" << m << ") ";
        }
        std::cout << "\n";
    }
};

int main() {
    Show s("show1", "Inception", 2, 3);
    s.layout();

    bool ok1 = s.lockSeats({"A1", "A2"}, "alice");
    bool ok2 = s.lockSeats({"A1"},        "bob");
    std::cout << "alice lock A1,A2: " << ok1 << "\n";
    std::cout << "bob   lock A1:    " << ok2 << " (should fail)\n";
    s.layout();

    bool conf = s.confirm({"A1", "A2"}, "alice");
    std::cout << "alice confirm: " << conf << "\n";
    s.layout();

    bool ok3 = s.lockSeats({"B1"}, "bob");
    s.cancel({"B1"}, "bob");
    std::cout << "bob lock B1: " << ok3 << " then cancel\n";
    s.layout();
    return 0;
}
