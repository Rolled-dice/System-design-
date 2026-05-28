#include <iostream>
#include <memory>
#include <string>

class PaymentGateway {
public:
    virtual ~PaymentGateway() = default;
    virtual bool pay(double amount) = 0;
    virtual std::string name() const = 0;
};

class StripeGateway : public PaymentGateway {
public:
    bool pay(double amount) override {
        std::cout << "[Stripe] charging $" << amount << "\n";
        return true;
    }
    std::string name() const override { return "Stripe"; }
};

class RazorpayGateway : public PaymentGateway {
public:
    bool pay(double amount) override {
        std::cout << "[Razorpay] charging Rs." << amount << "\n";
        return true;
    }
    std::string name() const override { return "Razorpay"; }
};

class CheckoutService {
    std::unique_ptr<PaymentGateway> gateway_;
public:
    explicit CheckoutService(std::unique_ptr<PaymentGateway> g) : gateway_(std::move(g)) {}
    void checkout(double amount) {
        std::cout << "Using " << gateway_->name() << "\n";
        gateway_->pay(amount);
    }
};

int main() {
    CheckoutService svc(std::make_unique<StripeGateway>());
    svc.checkout(99.99);

    CheckoutService svc2(std::make_unique<RazorpayGateway>());
    svc2.checkout(2500);
    return 0;
}
