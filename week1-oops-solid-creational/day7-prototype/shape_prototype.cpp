#include <iostream>
#include <memory>
#include <string>

class Shape {
public:
    virtual ~Shape() = default;
    virtual std::unique_ptr<Shape> clone() const = 0;
    virtual void draw() const = 0;
};

class Circle : public Shape {
    double r_;
    std::string color_;
public:
    Circle(double r, std::string c) : r_(r), color_(std::move(c)) {}
    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Circle>(*this);
    }
    void draw() const override {
        std::cout << "Circle r=" << r_ << " color=" << color_ << "\n";
    }
};

class Rectangle : public Shape {
    double w_, h_;
    std::string color_;
public:
    Rectangle(double w, double h, std::string c) : w_(w), h_(h), color_(std::move(c)) {}
    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Rectangle>(*this);
    }
    void draw() const override {
        std::cout << "Rect " << w_ << "x" << h_ << " color=" << color_ << "\n";
    }
};

int main() {
    std::unique_ptr<Shape> proto = std::make_unique<Circle>(10, "red");
    auto copy1 = proto->clone();
    auto copy2 = proto->clone();
    copy1->draw();
    copy2->draw();
    return 0;
}
