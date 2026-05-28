#include <iostream>
#include <memory>
#include <string>

class ICoffee {
public:
    virtual ~ICoffee() = default;
    virtual double cost() const = 0;
    virtual std::string desc() const = 0;
};

class Espresso : public ICoffee {
public:
    double cost() const override { return 50; }
    std::string desc() const override { return "Espresso"; }
};

class CoffeeDecorator : public ICoffee {
protected:
    std::unique_ptr<ICoffee> inner_;
public:
    explicit CoffeeDecorator(std::unique_ptr<ICoffee> c) : inner_(std::move(c)) {}
};

class Milk : public CoffeeDecorator {
public:
    using CoffeeDecorator::CoffeeDecorator;
    double cost() const override { return inner_->cost() + 15; }
    std::string desc() const override { return inner_->desc() + " + Milk"; }
};

class Sugar : public CoffeeDecorator {
public:
    using CoffeeDecorator::CoffeeDecorator;
    double cost() const override { return inner_->cost() + 5; }
    std::string desc() const override { return inner_->desc() + " + Sugar"; }
};

class Whip : public CoffeeDecorator {
public:
    using CoffeeDecorator::CoffeeDecorator;
    double cost() const override { return inner_->cost() + 20; }
    std::string desc() const override { return inner_->desc() + " + Whip"; }
};

int main() {
    std::unique_ptr<ICoffee> c = std::make_unique<Espresso>();
    c = std::make_unique<Milk>(std::move(c));
    c = std::make_unique<Sugar>(std::move(c));
    c = std::make_unique<Whip>(std::move(c));

    std::cout << c->desc() << " = Rs." << c->cost() << "\n";
    return 0;
}
