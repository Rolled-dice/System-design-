#include <iostream>
#include <memory>
#include <string>

class INotifier {
public:
    virtual ~INotifier() = default;
    virtual void send(const std::string& to, const std::string& msg) = 0;
};

class EmailNotifier : public INotifier {
public:
    void send(const std::string& to, const std::string& msg) override {
        std::cout << "[Email -> " << to << "] " << msg << "\n";
    }
};

class SmsNotifier : public INotifier {
public:
    void send(const std::string& to, const std::string& msg) override {
        std::cout << "[SMS -> " << to << "] " << msg << "\n";
    }
};

class OrderService {
    std::unique_ptr<INotifier> notifier_;
public:
    explicit OrderService(std::unique_ptr<INotifier> n) : notifier_(std::move(n)) {}
    void placeOrder(const std::string& user) {
        std::cout << "Order placed for " << user << "\n";
        notifier_->send(user, "Your order is confirmed");
    }
};

int main() {
    OrderService svc(std::make_unique<EmailNotifier>());
    svc.placeOrder("alice@x.com");

    OrderService svc2(std::make_unique<SmsNotifier>());
    svc2.placeOrder("+91-9999");
    return 0;
}
