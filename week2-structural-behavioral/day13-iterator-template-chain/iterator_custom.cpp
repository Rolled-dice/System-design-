#include <iostream>
#include <vector>

template <typename T>
class RingBuffer {
    std::vector<T> data_;
    size_t cap_;
    size_t head_ = 0, size_ = 0;
public:
    explicit RingBuffer(size_t cap) : data_(cap), cap_(cap) {}
    void push(T v) {
        size_t pos = (head_ + size_) % cap_;
        if (size_ == cap_) { head_ = (head_ + 1) % cap_; data_[pos] = v; }
        else { data_[pos] = v; ++size_; }
    }

    class Iterator {
        const RingBuffer& rb_;
        size_t idx_;
    public:
        Iterator(const RingBuffer& rb, size_t i) : rb_(rb), idx_(i) {}
        T operator*() const { return rb_.data_[(rb_.head_ + idx_) % rb_.cap_]; }
        Iterator& operator++() { ++idx_; return *this; }
        bool operator!=(const Iterator& o) const { return idx_ != o.idx_; }
    };

    Iterator begin() const { return Iterator(*this, 0); }
    Iterator end()   const { return Iterator(*this, size_); }
};

int main() {
    RingBuffer<int> rb(3);
    rb.push(1); rb.push(2); rb.push(3); rb.push(4); rb.push(5);
    for (int v : rb) std::cout << v << " ";
    std::cout << "\n";
    return 0;
}
