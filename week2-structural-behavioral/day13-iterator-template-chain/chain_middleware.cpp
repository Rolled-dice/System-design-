#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

struct Request {
    std::string path;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

class Middleware {
protected:
    std::shared_ptr<Middleware> next_;
public:
    virtual ~Middleware() = default;
    void setNext(std::shared_ptr<Middleware> n) { next_ = std::move(n); }
    virtual bool handle(Request& r) {
        return next_ ? next_->handle(r) : true;
    }
};

class AuthMiddleware : public Middleware {
public:
    bool handle(Request& r) override {
        if (r.headers["Authorization"].empty()) {
            std::cout << "[Auth] missing token, reject\n";
            return false;
        }
        std::cout << "[Auth] OK\n";
        return Middleware::handle(r);
    }
};

class RateLimitMiddleware : public Middleware {
    int allowed_ = 2;
public:
    bool handle(Request& r) override {
        if (allowed_-- <= 0) { std::cout << "[Rate] 429\n"; return false; }
        std::cout << "[Rate] OK\n";
        return Middleware::handle(r);
    }
};

class LoggingMiddleware : public Middleware {
public:
    bool handle(Request& r) override {
        std::cout << "[Log] " << r.path << "\n";
        return Middleware::handle(r);
    }
};

int main() {
    auto auth = std::make_shared<AuthMiddleware>();
    auto rate = std::make_shared<RateLimitMiddleware>();
    auto log  = std::make_shared<LoggingMiddleware>();
    auth->setNext(rate);
    rate->setNext(log);

    Request r1{"/api/orders", {{"Authorization", "tok"}}, ""};
    Request r2{"/api/orders", {}, ""};

    std::cout << "Req1 -> " << (auth->handle(r1) ? "200" : "fail") << "\n";
    std::cout << "Req2 -> " << (auth->handle(r2) ? "200" : "fail") << "\n";
    std::cout << "Req1 -> " << (auth->handle(r1) ? "200" : "fail") << "\n";
    std::cout << "Req1 -> " << (auth->handle(r1) ? "200" : "fail") << "\n";
    return 0;
}
