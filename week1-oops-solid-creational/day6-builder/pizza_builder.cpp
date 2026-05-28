#include <iostream>
#include <string>
#include <vector>

class Pizza {
public:
    std::string size;
    std::string crust;
    std::vector<std::string> toppings;
    bool extraCheese = false;

    void show() const {
        std::cout << size << " " << crust << " pizza";
        if (extraCheese) std::cout << " w/ extra cheese";
        std::cout << ", toppings: ";
        for (const auto& t : toppings) std::cout << t << " ";
        std::cout << "\n";
    }
};

class PizzaBuilder {
    Pizza p_;
public:
    PizzaBuilder& size(const std::string& s) { p_.size = s; return *this; }
    PizzaBuilder& crust(const std::string& c) { p_.crust = c; return *this; }
    PizzaBuilder& addTopping(const std::string& t) { p_.toppings.push_back(t); return *this; }
    PizzaBuilder& cheese(bool b) { p_.extraCheese = b; return *this; }
    Pizza build() { return std::move(p_); }
};

int main() {
    Pizza p = PizzaBuilder()
                .size("Large")
                .crust("Thin")
                .addTopping("Mushroom")
                .addTopping("Olives")
                .cheese(true)
                .build();
    p.show();
    return 0;
}
