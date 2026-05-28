#include <iostream>
#include <memory>
#include <string>

class Shape {
public:
    virtual ~Shape() = default;
    virtual void draw() const = 0;
};

class Circle    : public Shape { public: void draw() const override { std::cout << "Circle\n"; } };
class Square    : public Shape { public: void draw() const override { std::cout << "Square\n"; } };
class Triangle  : public Shape { public: void draw() const override { std::cout << "Triangle\n"; } };

class ShapeFactory {
public:
    static std::unique_ptr<Shape> create(const std::string& type) {
        if (type == "circle")   return std::make_unique<Circle>();
        if (type == "square")   return std::make_unique<Square>();
        if (type == "triangle") return std::make_unique<Triangle>();
        return nullptr;
    }
};

int main() {
    auto s1 = ShapeFactory::create("circle");
    auto s2 = ShapeFactory::create("square");
    s1->draw();
    s2->draw();
    return 0;
}
