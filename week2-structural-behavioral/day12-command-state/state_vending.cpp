#include <iostream>
#include <memory>

class VendingMachine;

class State {
public:
    virtual ~State() = default;
    virtual void insertCoin(VendingMachine&) = 0;
    virtual void selectItem(VendingMachine&) = 0;
    virtual void dispense(VendingMachine&)   = 0;
    virtual const char* name() const = 0;
};

class IdleState;
class HasCoinState;
class DispensingState;

class VendingMachine {
    std::unique_ptr<State> state_;
public:
    VendingMachine();
    void setState(std::unique_ptr<State> s) {
        state_ = std::move(s);
        std::cout << "[State -> " << state_->name() << "]\n";
    }
    void insertCoin() { state_->insertCoin(*this); }
    void selectItem() { state_->selectItem(*this); }
    void dispense()   { state_->dispense(*this); }
};

class IdleState : public State {
public:
    void insertCoin(VendingMachine& vm) override;
    void selectItem(VendingMachine&) override { std::cout << "Insert coin first\n"; }
    void dispense(VendingMachine&) override   { std::cout << "Insert coin first\n"; }
    const char* name() const override { return "Idle"; }
};

class HasCoinState : public State {
public:
    void insertCoin(VendingMachine&) override { std::cout << "Already has coin\n"; }
    void selectItem(VendingMachine& vm) override;
    void dispense(VendingMachine&) override   { std::cout << "Select item first\n"; }
    const char* name() const override { return "HasCoin"; }
};

class DispensingState : public State {
public:
    void insertCoin(VendingMachine&) override { std::cout << "Wait, dispensing\n"; }
    void selectItem(VendingMachine&) override { std::cout << "Wait, dispensing\n"; }
    void dispense(VendingMachine& vm) override;
    const char* name() const override { return "Dispensing"; }
};

void IdleState::insertCoin(VendingMachine& vm) {
    std::cout << "Coin accepted\n";
    vm.setState(std::make_unique<HasCoinState>());
}
void HasCoinState::selectItem(VendingMachine& vm) {
    std::cout << "Item selected\n";
    vm.setState(std::make_unique<DispensingState>());
}
void DispensingState::dispense(VendingMachine& vm) {
    std::cout << "Item dispensed\n";
    vm.setState(std::make_unique<IdleState>());
}

VendingMachine::VendingMachine() {
    setState(std::make_unique<IdleState>());
}

int main() {
    VendingMachine vm;
    vm.selectItem();
    vm.insertCoin();
    vm.selectItem();
    vm.dispense();
    return 0;
}
