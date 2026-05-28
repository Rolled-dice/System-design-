#include <iostream>
#include <memory>
#include <string>

class IImage {
public:
    virtual ~IImage() = default;
    virtual void display() = 0;
};

class RealImage : public IImage {
    std::string filename_;
    void load() { std::cout << "Loading heavy image " << filename_ << " from disk...\n"; }
public:
    explicit RealImage(std::string f) : filename_(std::move(f)) { load(); }
    void display() override { std::cout << "Display " << filename_ << "\n"; }
};

class ImageProxy : public IImage {
    std::string filename_;
    std::unique_ptr<RealImage> real_;
public:
    explicit ImageProxy(std::string f) : filename_(std::move(f)) {}
    void display() override {
        if (!real_) real_ = std::make_unique<RealImage>(filename_);
        real_->display();
    }
};

int main() {
    ImageProxy img("photo_4k.png");
    std::cout << "Proxy created (no load yet)\n";
    img.display();
    img.display();
    return 0;
}
