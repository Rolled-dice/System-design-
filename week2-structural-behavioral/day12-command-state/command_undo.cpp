#include <iostream>
#include <memory>
#include <stack>
#include <string>

class TextDocument {
    std::string text_;
public:
    void append(const std::string& s) { text_ += s; }
    void erase(size_t n) { if (n <= text_.size()) text_.erase(text_.size() - n); }
    const std::string& text() const { return text_; }
};

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo()    = 0;
};

class WriteCommand : public Command {
    TextDocument& doc_;
    std::string str_;
public:
    WriteCommand(TextDocument& d, std::string s) : doc_(d), str_(std::move(s)) {}
    void execute() override { doc_.append(str_); }
    void undo() override    { doc_.erase(str_.size()); }
};

class Editor {
    TextDocument doc_;
    std::stack<std::unique_ptr<Command>> history_;
public:
    void run(std::unique_ptr<Command> c) {
        c->execute();
        history_.push(std::move(c));
    }
    void undo() {
        if (history_.empty()) return;
        history_.top()->undo();
        history_.pop();
    }
    const std::string& text() const { return doc_.text(); }
    TextDocument& doc() { return doc_; }
};

int main() {
    Editor e;
    e.run(std::make_unique<WriteCommand>(e.doc(), "Hello "));
    e.run(std::make_unique<WriteCommand>(e.doc(), "World"));
    std::cout << e.text() << "\n";
    e.undo();
    std::cout << e.text() << "\n";
    e.undo();
    std::cout << "[" << e.text() << "]\n";
    return 0;
}
