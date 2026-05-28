#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

class Unit {
public:
    virtual ~Unit() = default;
    virtual std::unique_ptr<Unit> clone() const = 0;
    virtual void info() const = 0;
};

class Warrior : public Unit {
    int hp_, dmg_;
public:
    Warrior(int hp, int dmg) : hp_(hp), dmg_(dmg) {}
    std::unique_ptr<Unit> clone() const override { return std::make_unique<Warrior>(*this); }
    void info() const override { std::cout << "Warrior HP=" << hp_ << " DMG=" << dmg_ << "\n"; }
};

class Mage : public Unit {
    int hp_, mana_;
public:
    Mage(int hp, int mana) : hp_(hp), mana_(mana) {}
    std::unique_ptr<Unit> clone() const override { return std::make_unique<Mage>(*this); }
    void info() const override { std::cout << "Mage HP=" << hp_ << " Mana=" << mana_ << "\n"; }
};

class UnitRegistry {
    std::unordered_map<std::string, std::unique_ptr<Unit>> protos_;
public:
    void registerProto(const std::string& name, std::unique_ptr<Unit> u) {
        protos_[name] = std::move(u);
    }
    std::unique_ptr<Unit> spawn(const std::string& name) {
        auto it = protos_.find(name);
        return it != protos_.end() ? it->second->clone() : nullptr;
    }
};

int main() {
    UnitRegistry reg;
    reg.registerProto("eliteWarrior", std::make_unique<Warrior>(200, 50));
    reg.registerProto("frostMage",    std::make_unique<Mage>(120, 300));

    auto u1 = reg.spawn("eliteWarrior");
    auto u2 = reg.spawn("frostMage");
    auto u3 = reg.spawn("eliteWarrior");
    u1->info(); u2->info(); u3->info();
    return 0;
}
