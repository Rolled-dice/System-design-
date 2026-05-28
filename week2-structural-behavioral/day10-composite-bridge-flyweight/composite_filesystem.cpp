#include <iostream>
#include <memory>
#include <string>
#include <vector>

class FsNode {
public:
    virtual ~FsNode() = default;
    virtual int size() const = 0;
    virtual void print(int depth) const = 0;
};

class File : public FsNode {
    std::string name_;
    int size_;
public:
    File(std::string n, int s) : name_(std::move(n)), size_(s) {}
    int size() const override { return size_; }
    void print(int depth) const override {
        std::cout << std::string(depth * 2, ' ') << "- " << name_ << " (" << size_ << "b)\n";
    }
};

class Directory : public FsNode {
    std::string name_;
    std::vector<std::unique_ptr<FsNode>> children_;
public:
    explicit Directory(std::string n) : name_(std::move(n)) {}
    void add(std::unique_ptr<FsNode> n) { children_.push_back(std::move(n)); }
    int size() const override {
        int total = 0;
        for (const auto& c : children_) total += c->size();
        return total;
    }
    void print(int depth) const override {
        std::cout << std::string(depth * 2, ' ') << "+ " << name_ << "/ (" << size() << "b)\n";
        for (const auto& c : children_) c->print(depth + 1);
    }
};

int main() {
    auto root = std::make_unique<Directory>("root");
    root->add(std::make_unique<File>("a.txt", 100));

    auto sub = std::make_unique<Directory>("docs");
    sub->add(std::make_unique<File>("readme.md", 500));
    sub->add(std::make_unique<File>("note.txt", 50));
    root->add(std::move(sub));

    root->print(0);
    std::cout << "Total: " << root->size() << "b\n";
    return 0;
}
