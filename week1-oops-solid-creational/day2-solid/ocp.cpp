#include <iostream>
#include <memory>
#include <vector>

class DiscountStrategy {
public:
    virtual ~DiscountStrategy() = default;
    virtual double apply(double price) const = 0;
};

class NoDiscount : public DiscountStrategy {
public:
    double apply(double price) const override { return price; }
};

class PercentageDiscount : public DiscountStrategy {
    double pct_;
public:
    explicit PercentageDiscount(double pct) : pct_(pct) {}
    double apply(double price) const override { return price * (1 - pct_ / 100); }
};

class FlatDiscount : public DiscountStrategy {
    double flat_;
public:
    explicit FlatDiscount(double f) : flat_(f) {}
    double apply(double price) const override { return std::max(0.0, price - flat_); }
};

class Checkout {
    std::unique_ptr<DiscountStrategy> strategy_;
public:
    explicit Checkout(std::unique_ptr<DiscountStrategy> s) : strategy_(std::move(s)) {}
    double total(double price) const { return strategy_->apply(price); }
};

int main() {
    Checkout c1(std::make_unique<PercentageDiscount>(20));
    std::cout << "After 20% off: " << c1.total(1000) << "\n";

    Checkout c2(std::make_unique<FlatDiscount>(150));
    std::cout << "After flat 150: " << c2.total(1000) << "\n";
    return 0;
}
