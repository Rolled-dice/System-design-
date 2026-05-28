#include <iostream>

class Basic {
public:
    static Basic* getInstance() {
        if (!inst_) inst_ = new Basic();
        return inst_;
    }
    void hello() { std::cout << "hi from singleton\n"; }

    Basic(const Basic&) = delete;
    Basic& operator=(const Basic&) = delete;

private:
    Basic() = default;
    static Basic* inst_;
};

Basic* Basic::inst_ = nullptr;

int main() {
    Basic::getInstance()->hello();
    return 0;
}
