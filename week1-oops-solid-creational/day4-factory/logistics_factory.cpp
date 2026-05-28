#include <iostream>
#include <memory>

class Transport {
public:
    virtual ~Transport() = default;
    virtual void deliver() = 0;
};

class Truck : public Transport {
public:
    void deliver() override { std::cout << "Deliver by land in a Truck\n"; }
};

class Ship : public Transport {
public:
    void deliver() override { std::cout << "Deliver by sea in a Ship\n"; }
};

class Drone : public Transport {
public:
    void deliver() override { std::cout << "Deliver by air via Drone\n"; }
};

class Logistics {
public:
    virtual ~Logistics() = default;
    virtual std::unique_ptr<Transport> createTransport() = 0;

    void planDelivery() {
        auto t = createTransport();
        t->deliver();
    }
};

class RoadLogistics : public Logistics {
public:
    std::unique_ptr<Transport> createTransport() override { return std::make_unique<Truck>(); }
};

class SeaLogistics : public Logistics {
public:
    std::unique_ptr<Transport> createTransport() override { return std::make_unique<Ship>(); }
};

class AirLogistics : public Logistics {
public:
    std::unique_ptr<Transport> createTransport() override { return std::make_unique<Drone>(); }
};

int main() {
    std::unique_ptr<Logistics> l = std::make_unique<RoadLogistics>();
    l->planDelivery();
    l = std::make_unique<SeaLogistics>();
    l->planDelivery();
    l = std::make_unique<AirLogistics>();
    l->planDelivery();
    return 0;
}
