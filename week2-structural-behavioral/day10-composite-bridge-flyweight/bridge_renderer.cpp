#include <iostream>
#include <memory>

class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void renderCircle(double r) = 0;
    virtual void renderSquare(double s) = 0;
};

class VectorRenderer : public Renderer {
public:
    void renderCircle(double r) override { std::cout << "[Vector] circle r=" << r << "\n"; }
    void renderSquare(double s) override { std::cout << "[Vector] square s=" << s << "\n"; }
};

class RasterRenderer : public Renderer {
public:
    void renderCircle(double r) override { std::cout << "[Raster] pixels for circle r=" << r << "\n"; }
    void renderSquare(double s) override { std::cout << "[Raster] pixels for square s=" << s << "\n"; }
};

class Shape {
protected:
    Renderer& renderer_;
public:
    explicit Shape(Renderer& r) : renderer_(r) {}
    virtual ~Shape() = default;
    virtual void draw() = 0;
};

class Circle : public Shape {
    double r_;
public:
    Circle(Renderer& r, double radius) : Shape(r), r_(radius) {}
    void draw() override { renderer_.renderCircle(r_); }
};

class Square : public Shape {
    double s_;
public:
    Square(Renderer& r, double side) : Shape(r), s_(side) {}
    void draw() override { renderer_.renderSquare(s_); }
};

int main() {
    VectorRenderer vec;
    RasterRenderer ras;

    Circle c1(vec, 5);
    Circle c2(ras, 5);
    Square s1(vec, 3);
    Square s2(ras, 3);

    c1.draw(); c2.draw(); s1.draw(); s2.draw();
    return 0;
}
