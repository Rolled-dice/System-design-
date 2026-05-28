#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

struct Product { int id; std::string sku; std::string name; };

class ProductTable {
    std::unordered_map<int, Product> primary_;
    std::unordered_map<std::string, int> bySku_;
public:
    void insert(const Product& p) {
        primary_[p.id] = p;
        bySku_[p.sku] = p.id;
    }
    Product* findById(int id) {
        auto it = primary_.find(id);
        return it == primary_.end() ? nullptr : &it->second;
    }
    Product* findBySku(const std::string& sku) {
        auto it = bySku_.find(sku);
        if (it == bySku_.end()) return nullptr;
        return findById(it->second);
    }
};

int main() {
    ProductTable t;
    t.insert({1, "SKU-A1", "Pen"});
    t.insert({2, "SKU-B2", "Pencil"});
    t.insert({3, "SKU-C3", "Notebook"});

    auto* p = t.findBySku("SKU-B2");
    std::cout << "SKU-B2 -> " << (p ? p->name : "(none)") << "\n";
    return 0;
}
