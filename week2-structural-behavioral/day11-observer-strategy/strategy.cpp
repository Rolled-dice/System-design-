#include <iostream>
#include <memory>

class PaymentStrategy {
public:
    virtual ~PaymentStrategy() = default;
    virtual void pay(double amt) = 0;
};

class CreditCard : public PaymentStrategy {
public:
    void pay(double a) override { std::cout << "[CC] charged " << a << "\n"; }
};

class UPI : public PaymentStrategy {
public:
    void pay(double a) override { std::cout << "[UPI] paid " << a << "\n"; }
};

class Wallet : public PaymentStrategy {
public:
    void pay(double a) override { std::cout << "[Wallet] deducted " << a << "\n"; }
};

class Cart {
    std::unique_ptr<PaymentStrategy> strat_;
public:
    void setStrategy(std::unique_ptr<PaymentStrategy> s) { strat_ = std::move(s); }
    void checkout(double amt) {
        if (strat_) strat_->pay(amt);
    }
};

int main() {
    Cart c;
    c.setStrategy(std::make_unique<CreditCard>()); c.checkout(599);
    c.setStrategy(std::make_unique<UPI>());        c.checkout(150);
    c.setStrategy(std::make_unique<Wallet>());     c.checkout(99);
    return 0;
}
