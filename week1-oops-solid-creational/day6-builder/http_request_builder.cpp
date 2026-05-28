#include <iostream>
#include <map>
#include <string>

class HttpRequest {
public:
    std::string method;
    std::string url;
    std::map<std::string, std::string> headers;
    std::string body;
    int timeoutMs = 30000;

    void send() const {
        std::cout << method << " " << url << " (timeout=" << timeoutMs << "ms)\n";
        for (const auto& [k, v] : headers) std::cout << "  " << k << ": " << v << "\n";
        if (!body.empty()) std::cout << "  body: " << body << "\n";
    }
};

class HttpRequestBuilder {
    HttpRequest req_;
public:
    HttpRequestBuilder& method(const std::string& m) { req_.method = m; return *this; }
    HttpRequestBuilder& url(const std::string& u) { req_.url = u; return *this; }
    HttpRequestBuilder& header(const std::string& k, const std::string& v) { req_.headers[k] = v; return *this; }
    HttpRequestBuilder& body(const std::string& b) { req_.body = b; return *this; }
    HttpRequestBuilder& timeout(int ms) { req_.timeoutMs = ms; return *this; }
    HttpRequest build() { return std::move(req_); }
};

int main() {
    HttpRequest r = HttpRequestBuilder()
                       .method("POST")
                       .url("https://api.example.com/orders")
                       .header("Content-Type", "application/json")
                       .header("Authorization", "Bearer xyz")
                       .body(R"({"item":"book","qty":2})")
                       .timeout(5000)
                       .build();
    r.send();
    return 0;
}
