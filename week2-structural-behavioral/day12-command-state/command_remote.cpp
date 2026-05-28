#include <iostream>
#include <memory>
#include <unordered_map>

class Light {
public: void on() { std::cout << "Light ON\n"; } void off() { std::cout << "Light OFF\n"; }
};
class Fan {
public: void on() { std::cout << "Fan ON\n"; } void off() { std::cout << "Fan OFF\n"; }
};

class Cmd {
public: virtual ~Cmd() = default; virtual void execute() = 0;
};

class LightOn : public Cmd { Light& l_; public: explicit LightOn(Light& l) : l_(l) {} void execute() override { l_.on(); } };
class LightOff: public Cmd { Light& l_; public: explicit LightOff(Light& l): l_(l) {} void execute() override { l_.off(); } };
class FanOn   : public Cmd { Fan&   f_; public: explicit FanOn(Fan& f)    : f_(f) {} void execute() override { f_.on(); } };
class FanOff  : public Cmd { Fan&   f_; public: explicit FanOff(Fan& f)   : f_(f) {} void execute() override { f_.off(); } };

class Remote {
    std::unordered_map<int, std::unique_ptr<Cmd>> slots_;
public:
    void bind(int slot, std::unique_ptr<Cmd> c) { slots_[slot] = std::move(c); }
    void press(int slot) { if (slots_.count(slot)) slots_[slot]->execute(); }
};

int main() {
    Light l; Fan f;
    Remote r;
    r.bind(1, std::make_unique<LightOn>(l));
    r.bind(2, std::make_unique<LightOff>(l));
    r.bind(3, std::make_unique<FanOn>(f));
    r.bind(4, std::make_unique<FanOff>(f));

    r.press(1); r.press(3); r.press(2); r.press(4);
    return 0;
}
