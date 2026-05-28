#include <iostream>
#include <stack>
#include <string>

class Memento {
    std::string state_;
public:
    explicit Memento(std::string s) : state_(std::move(s)) {}
    const std::string& state() const { return state_; }
};

class Editor {
    std::string text_;
public:
    void type(const std::string& s) { text_ += s; }
    Memento save() const { return Memento(text_); }
    void restore(const Memento& m) { text_ = m.state(); }
    const std::string& text() const { return text_; }
};

class History {
    std::stack<Memento> stk_;
public:
    void push(Memento m) { stk_.push(std::move(m)); }
    bool empty() const { return stk_.empty(); }
    Memento pop() { Memento m = stk_.top(); stk_.pop(); return m; }
};

int main() {
    Editor e;
    History h;

    e.type("Hello");
    h.push(e.save());

    e.type(" World");
    h.push(e.save());

    e.type("!!!");
    std::cout << "current: " << e.text() << "\n";

    e.restore(h.pop());
    std::cout << "undo:    " << e.text() << "\n";

    e.restore(h.pop());
    std::cout << "undo:    " << e.text() << "\n";
    return 0;
}
