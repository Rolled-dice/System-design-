#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

class SortStrategy {
public:
    virtual ~SortStrategy() = default;
    virtual void sort(std::vector<int>& v) = 0;
};

class AscSort : public SortStrategy {
public:
    void sort(std::vector<int>& v) override { std::sort(v.begin(), v.end()); }
};

class DescSort : public SortStrategy {
public:
    void sort(std::vector<int>& v) override {
        std::sort(v.begin(), v.end(), std::greater<int>());
    }
};

int main() {
    std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};

    AscSort asc; asc.sort(v);
    for (int x : v) std::cout << x << " ";
    std::cout << "\n";

    DescSort desc; desc.sort(v);
    for (int x : v) std::cout << x << " ";
    std::cout << "\n";
    return 0;
}
