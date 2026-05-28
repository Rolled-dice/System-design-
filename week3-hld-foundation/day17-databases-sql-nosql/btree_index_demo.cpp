#include <iostream>
#include <map>
#include <string>

struct User { int id; std::string name; int age; };

class UserTable {
    std::map<int, User> primary_;
public:
    void insert(const User& u) { primary_[u.id] = u; }

    User* findById(int id) {
        auto it = primary_.find(id);
        return it == primary_.end() ? nullptr : &it->second;
    }

    void rangeQuery(int lo, int hi) {
        auto it = primary_.lower_bound(lo);
        auto end = primary_.upper_bound(hi);
        for (; it != end; ++it)
            std::cout << "  id=" << it->first << " " << it->second.name << "\n";
    }
};

int main() {
    UserTable t;
    t.insert({1, "A", 22});
    t.insert({5, "E", 30});
    t.insert({3, "C", 28});
    t.insert({7, "G", 19});
    t.insert({4, "D", 25});

    std::cout << "id=3 -> " << t.findById(3)->name << "\n";
    std::cout << "range [3..5]:\n"; t.rangeQuery(3, 5);
    return 0;
}
