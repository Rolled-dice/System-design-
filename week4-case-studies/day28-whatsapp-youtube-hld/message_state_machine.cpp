#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

enum class MsgState { SENT, DELIVERED, READ, FAILED };

class Message {
    std::string id_;
    std::string from_, to_, body_;
    MsgState state_ = MsgState::SENT;

    static const char* name(MsgState s) {
        switch (s) {
            case MsgState::SENT: return "SENT";
            case MsgState::DELIVERED: return "DELIVERED";
            case MsgState::READ: return "READ";
            case MsgState::FAILED: return "FAILED";
        }
        return "?";
    }

    bool canTransition(MsgState to) const {
        if (state_ == MsgState::SENT)      return to == MsgState::DELIVERED || to == MsgState::FAILED;
        if (state_ == MsgState::DELIVERED) return to == MsgState::READ;
        return false;
    }
public:
    Message(std::string id, std::string from, std::string to, std::string body)
        : id_(std::move(id)), from_(std::move(from)), to_(std::move(to)), body_(std::move(body)) {}

    void transition(MsgState to) {
        if (!canTransition(to))
            throw std::runtime_error("Invalid transition " + std::string(name(state_)) + "->" + name(to));
        std::cout << "msg " << id_ << ": " << name(state_) << " -> " << name(to) << "\n";
        state_ = to;
    }

    MsgState state() const { return state_; }
    const std::string& id() const { return id_; }
};

int main() {
    Message m("m1", "alice", "bob", "hi");
    m.transition(MsgState::DELIVERED);
    m.transition(MsgState::READ);

    Message m2("m2", "alice", "carol", "yo");
    try { m2.transition(MsgState::READ); }
    catch (const std::exception& e) { std::cout << "expected: " << e.what() << "\n"; }

    m2.transition(MsgState::FAILED);
    return 0;
}
