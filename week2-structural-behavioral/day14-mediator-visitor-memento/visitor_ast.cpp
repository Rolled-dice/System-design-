#include <iostream>
#include <memory>
#include <string>

class NumberNode;
class AddNode;
class MulNode;

class Visitor {
public:
    virtual ~Visitor() = default;
    virtual void visit(NumberNode& n) = 0;
    virtual void visit(AddNode& n)    = 0;
    virtual void visit(MulNode& n)    = 0;
};

class Node {
public:
    virtual ~Node() = default;
    virtual void accept(Visitor& v) = 0;
};

class NumberNode : public Node {
public:
    int value;
    explicit NumberNode(int v) : value(v) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

class AddNode : public Node {
public:
    std::unique_ptr<Node> l, r;
    AddNode(std::unique_ptr<Node> a, std::unique_ptr<Node> b) : l(std::move(a)), r(std::move(b)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

class MulNode : public Node {
public:
    std::unique_ptr<Node> l, r;
    MulNode(std::unique_ptr<Node> a, std::unique_ptr<Node> b) : l(std::move(a)), r(std::move(b)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

class EvalVisitor : public Visitor {
public:
    int result = 0;
    void visit(NumberNode& n) override { result = n.value; }
    void visit(AddNode& n) override {
        EvalVisitor le, re;
        n.l->accept(le); n.r->accept(re);
        result = le.result + re.result;
    }
    void visit(MulNode& n) override {
        EvalVisitor le, re;
        n.l->accept(le); n.r->accept(re);
        result = le.result * re.result;
    }
};

class PrintVisitor : public Visitor {
public:
    std::string out;
    void visit(NumberNode& n) override { out += std::to_string(n.value); }
    void visit(AddNode& n) override {
        out += "(";
        n.l->accept(*this); out += " + "; n.r->accept(*this);
        out += ")";
    }
    void visit(MulNode& n) override {
        out += "(";
        n.l->accept(*this); out += " * "; n.r->accept(*this);
        out += ")";
    }
};

int main() {
    auto expr = std::make_unique<AddNode>(
        std::make_unique<NumberNode>(2),
        std::make_unique<MulNode>(std::make_unique<NumberNode>(3), std::make_unique<NumberNode>(4)));

    PrintVisitor pv; expr->accept(pv);
    EvalVisitor  ev; expr->accept(ev);
    std::cout << pv.out << " = " << ev.result << "\n";
    return 0;
}
