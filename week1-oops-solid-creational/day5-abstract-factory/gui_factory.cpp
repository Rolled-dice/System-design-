#include <iostream>
#include <memory>

class Button {
public:
    virtual ~Button() = default;
    virtual void render() = 0;
};
class Checkbox {
public:
    virtual ~Checkbox() = default;
    virtual void render() = 0;
};

class MacButton    : public Button   { public: void render() override { std::cout << "[Mac Button]\n"; } };
class WinButton    : public Button   { public: void render() override { std::cout << "[Win Button]\n"; } };
class MacCheckbox  : public Checkbox { public: void render() override { std::cout << "[Mac Checkbox]\n"; } };
class WinCheckbox  : public Checkbox { public: void render() override { std::cout << "[Win Checkbox]\n"; } };

class GUIFactory {
public:
    virtual ~GUIFactory() = default;
    virtual std::unique_ptr<Button>   createButton()   = 0;
    virtual std::unique_ptr<Checkbox> createCheckbox() = 0;
};

class MacFactory : public GUIFactory {
public:
    std::unique_ptr<Button>   createButton()   override { return std::make_unique<MacButton>(); }
    std::unique_ptr<Checkbox> createCheckbox() override { return std::make_unique<MacCheckbox>(); }
};

class WinFactory : public GUIFactory {
public:
    std::unique_ptr<Button>   createButton()   override { return std::make_unique<WinButton>(); }
    std::unique_ptr<Checkbox> createCheckbox() override { return std::make_unique<WinCheckbox>(); }
};

void renderForm(GUIFactory& f) {
    auto b = f.createButton();
    auto c = f.createCheckbox();
    b->render();
    c->render();
}

int main() {
    MacFactory mac; renderForm(mac);
    WinFactory win; renderForm(win);
    return 0;
}
