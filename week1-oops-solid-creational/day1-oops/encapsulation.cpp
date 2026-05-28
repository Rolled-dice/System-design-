#include <iostream>
#include <stdexcept>
#include <string>

class BankAccount {
private:
    std::string owner_;
    double balance_;

public:
    BankAccount(std::string owner, double opening)
        : owner_(std::move(owner)), balance_(opening) {
        if (opening < 0) throw std::invalid_argument("Opening balance must be non-negative");
    }

    void deposit(double amount) {
        if (amount <= 0) throw std::invalid_argument("Deposit must be positive");
        balance_ += amount;
    }

    void withdraw(double amount) {
        if (amount <= 0) throw std::invalid_argument("Withdraw must be positive");
        if (amount > balance_) throw std::runtime_error("Insufficient funds");
        balance_ -= amount;
    }

    double balance() const { return balance_; }
    const std::string& owner() const { return owner_; }
};

int main() {
    BankAccount acc("Alice", 1000);
    acc.deposit(500);
    acc.withdraw(300);
    std::cout << acc.owner() << " balance: " << acc.balance() << "\n";

    try {
        acc.withdraw(5000);
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
    return 0;
}
