#include <iostream>
#include <memory>
#include <vector>

class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void update(double price) = 0;
};

class StockTicker {
    std::vector<std::weak_ptr<IObserver>> obs_;
    double price_ = 0;
public:
    void subscribe(std::shared_ptr<IObserver> o) { obs_.push_back(o); }
    void setPrice(double p) {
        price_ = p;
        notify();
    }
    void notify() {
        for (auto it = obs_.begin(); it != obs_.end(); ) {
            if (auto sp = it->lock()) {
                sp->update(price_);
                ++it;
            } else {
                it = obs_.erase(it);
            }
        }
    }
};

class PhoneApp : public IObserver {
public:
    void update(double p) override { std::cout << "[Phone] price: " << p << "\n"; }
};

class WebDashboard : public IObserver {
public:
    void update(double p) override { std::cout << "[Web]   price: " << p << "\n"; }
};

int main() {
    StockTicker t;
    auto a = std::make_shared<PhoneApp>();
    auto b = std::make_shared<WebDashboard>();
    t.subscribe(a);
    t.subscribe(b);

    t.setPrice(101.5);
    t.setPrice(99.2);
    return 0;
}
