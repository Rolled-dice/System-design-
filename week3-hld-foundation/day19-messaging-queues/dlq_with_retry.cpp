#include <chrono>
#include <functional>
#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <vector>

struct Message {
    std::string id;
    std::string body;
    int attempts = 0;
};

class RetryQueue {
    std::queue<Message> main_;
    std::vector<Message> dlq_;
    int maxAttempts_;
    std::function<bool(const Message&)> handler_;
public:
    RetryQueue(int max, std::function<bool(const Message&)> h)
        : maxAttempts_(max), handler_(std::move(h)) {}

    void publish(Message m) { main_.push(std::move(m)); }

    void processAll() {
        while (!main_.empty()) {
            Message m = main_.front(); main_.pop();
            m.attempts++;
            if (handler_(m)) {
                std::cout << "  ack " << m.id << " (attempt " << m.attempts << ")\n";
            } else if (m.attempts >= maxAttempts_) {
                std::cout << "  -> DLQ " << m.id << " after " << m.attempts << " attempts\n";
                dlq_.push_back(m);
            } else {
                int backoffMs = (1 << m.attempts) * 100;
                std::cout << "  retry " << m.id << " backoff=" << backoffMs << "ms\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs));
                main_.push(m);
            }
        }
    }

    const std::vector<Message>& deadLetters() const { return dlq_; }
};

int main() {
    int callCount = 0;
    RetryQueue q(3, [&](const Message& m) {
        ++callCount;
        if (m.id == "evil") return false;
        return m.id != "flaky" || m.attempts >= 2;
    });

    q.publish({"good", "ok"});
    q.publish({"flaky", "succeeds on attempt 2"});
    q.publish({"evil", "always fails"});

    q.processAll();
    std::cout << "DLQ size: " << q.deadLetters().size() << "\n";
    return 0;
}
