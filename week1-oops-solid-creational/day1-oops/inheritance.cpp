#include <iostream>
#include <string>

class Vehicle {
protected:
    std::string brand_;
public:
    explicit Vehicle(std::string b) : brand_(std::move(b)) {}
    virtual ~Vehicle() = default;
    virtual void start() const { std::cout << brand_ << " vehicle starts\n"; }
};

class Car : public Vehicle {
    int wheels_ = 4;
public:
    explicit Car(std::string b) : Vehicle(std::move(b)) {}
    void start() const override { std::cout << brand_ << " car starts on " << wheels_ << " wheels\n"; }
};

class ElectricCar : public Car {
    int batteryKWh_;
public:
    ElectricCar(std::string b, int kwh) : Car(std::move(b)), batteryKWh_(kwh) {}
    void start() const override {
        std::cout << brand_ << " EV silently starts. Battery: " << batteryKWh_ << " kWh\n";
    }
};

int main() {
    Vehicle* v = new ElectricCar("Tesla", 75);
    v->start();
    delete v;
    return 0;
}
