#include <iostream>

class Shape {
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;
};

class Rectangle : public Shape {
protected:
    double w_, h_;
public:
    Rectangle(double w, double h) : w_(w), h_(h) {}
    double area() const override { return w_ * h_; }
    double width() const { return w_; }
    double height() const { return h_; }
};

class Square : public Shape {
    double side_;
public:
    explicit Square(double s) : side_(s) {}
    double area() const override { return side_ * side_; }
    double side() const { return side_; }
};

void printArea(const Shape& s) {
    std::cout << "Area = " << s.area() << "\n";
}

int main() {
    Rectangle r(4, 5);
    Square sq(6);
    printArea(r);
    printArea(sq);
    return 0;
}
