#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

class User;

class IChatRoom {
public:
    virtual ~IChatRoom() = default;
    virtual void send(const std::string& from, const std::string& to, const std::string& msg) = 0;
    virtual void registerUser(std::shared_ptr<User> u) = 0;
};

class User {
    std::string name_;
    IChatRoom* room_;
public:
    User(std::string n, IChatRoom* r) : name_(std::move(n)), room_(r) {}
    const std::string& name() const { return name_; }
    void send(const std::string& to, const std::string& msg) { room_->send(name_, to, msg); }
    void receive(const std::string& from, const std::string& msg) {
        std::cout << "[" << name_ << "] from " << from << ": " << msg << "\n";
    }
};

class ChatRoom : public IChatRoom {
    std::unordered_map<std::string, std::shared_ptr<User>> users_;
public:
    void registerUser(std::shared_ptr<User> u) override {
        users_[u->name()] = u;
    }
    void send(const std::string& from, const std::string& to, const std::string& msg) override {
        auto it = users_.find(to);
        if (it != users_.end()) it->second->receive(from, msg);
    }
};

int main() {
    auto room = std::make_shared<ChatRoom>();
    auto a = std::make_shared<User>("Alice", room.get());
    auto b = std::make_shared<User>("Bob",   room.get());
    auto c = std::make_shared<User>("Carol", room.get());
    room->registerUser(a);
    room->registerUser(b);
    room->registerUser(c);

    a->send("Bob", "Hi!");
    c->send("Alice", "Yo");
    return 0;
}
