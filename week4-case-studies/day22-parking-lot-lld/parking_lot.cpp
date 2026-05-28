#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

enum class SlotType { SMALL, MEDIUM, LARGE };

class Vehicle {
public:
    std::string plate;
    SlotType minSlot;
    Vehicle(std::string p, SlotType s) : plate(std::move(p)), minSlot(s) {}
    virtual ~Vehicle() = default;
};

class Bike  : public Vehicle { public: explicit Bike(std::string p)  : Vehicle(std::move(p), SlotType::SMALL)  {} };
class Car   : public Vehicle { public: explicit Car(std::string p)   : Vehicle(std::move(p), SlotType::MEDIUM) {} };
class Truck : public Vehicle { public: explicit Truck(std::string p) : Vehicle(std::move(p), SlotType::LARGE)  {} };

struct ParkingSlot {
    int id;
    SlotType type;
    bool isFree = true;
    std::shared_ptr<Vehicle> vehicle;
};

class PricingStrategy {
public:
    virtual ~PricingStrategy() = default;
    virtual double price(SlotType t, int hours) const = 0;
};

class HourlyPricing : public PricingStrategy {
public:
    double price(SlotType t, int hours) const override {
        double rate = (t == SlotType::SMALL) ? 10 : (t == SlotType::MEDIUM ? 20 : 40);
        return rate * std::max(1, hours);
    }
};

struct Ticket {
    int id;
    std::string plate;
    int slotId;
    SlotType type;
    std::chrono::steady_clock::time_point entry;
};

class ParkingFloor {
    int floorNum_;
    std::vector<ParkingSlot> slots_;
    std::mutex mu_;
public:
    ParkingFloor(int n, int small, int medium, int large) : floorNum_(n) {
        int id = 0;
        for (int i = 0; i < small;  ++i) slots_.push_back({id++, SlotType::SMALL});
        for (int i = 0; i < medium; ++i) slots_.push_back({id++, SlotType::MEDIUM});
        for (int i = 0; i < large;  ++i) slots_.push_back({id++, SlotType::LARGE});
    }
    int floorNum() const { return floorNum_; }

    std::optional<int> allocate(std::shared_ptr<Vehicle> v) {
        std::lock_guard lk(mu_);
        for (auto& s : slots_) {
            if (s.isFree && static_cast<int>(s.type) >= static_cast<int>(v->minSlot)) {
                s.isFree = false;
                s.vehicle = v;
                return s.id;
            }
        }
        return std::nullopt;
    }
    SlotType release(int slotId) {
        std::lock_guard lk(mu_);
        slots_[slotId].isFree = true;
        SlotType t = slots_[slotId].type;
        slots_[slotId].vehicle.reset();
        return t;
    }
    int freeCount() {
        std::lock_guard lk(mu_);
        int c = 0;
        for (const auto& s : slots_) if (s.isFree) ++c;
        return c;
    }
};

class ParkingLot {
    std::vector<std::unique_ptr<ParkingFloor>> floors_;
    std::unique_ptr<PricingStrategy> pricing_;
    std::unordered_map<int, Ticket> active_;
    std::mutex mu_;
    int nextTicket_ = 1;
public:
    explicit ParkingLot(std::unique_ptr<PricingStrategy> p) : pricing_(std::move(p)) {}
    void addFloor(int small, int medium, int large) {
        floors_.push_back(std::make_unique<ParkingFloor>(static_cast<int>(floors_.size()), small, medium, large));
    }

    std::optional<Ticket> park(std::shared_ptr<Vehicle> v) {
        for (auto& f : floors_) {
            if (auto sid = f->allocate(v)) {
                std::lock_guard lk(mu_);
                Ticket t{nextTicket_++, v->plate, *sid, v->minSlot, std::chrono::steady_clock::now()};
                active_[t.id] = t;
                std::cout << "Parked " << v->plate << " floor=" << f->floorNum() << " slot=" << *sid << " ticket=" << t.id << "\n";
                return t;
            }
        }
        std::cout << "No slot for " << v->plate << "\n";
        return std::nullopt;
    }

    double unpark(int ticketId) {
        std::unique_lock lk(mu_);
        auto it = active_.find(ticketId);
        if (it == active_.end()) return -1;
        Ticket t = it->second;
        active_.erase(it);
        lk.unlock();

        floors_[0]->release(t.slotId);
        auto now = std::chrono::steady_clock::now();
        int hours = std::chrono::duration_cast<std::chrono::hours>(now - t.entry).count();
        double bill = pricing_->price(t.type, hours);
        std::cout << "Unparked ticket=" << t.id << " plate=" << t.plate << " bill=" << bill << "\n";
        return bill;
    }
};

int main() {
    ParkingLot lot(std::make_unique<HourlyPricing>());
    lot.addFloor(2, 2, 1);

    auto bike = std::make_shared<Bike>("KA-01-1");
    auto car  = std::make_shared<Car>("KA-01-2");
    auto truck = std::make_shared<Truck>("KA-01-3");

    auto t1 = lot.park(bike);
    auto t2 = lot.park(car);
    auto t3 = lot.park(truck);

    if (t1) lot.unpark(t1->id);
    if (t2) lot.unpark(t2->id);
    if (t3) lot.unpark(t3->id);
    return 0;
}
