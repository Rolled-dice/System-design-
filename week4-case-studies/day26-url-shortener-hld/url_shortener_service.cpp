#include <atomic>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class Base62 {
    static constexpr const char* ALPHABET =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
public:
    static std::string encode(uint64_t n) {
        if (n == 0) return "0";
        std::string s;
        while (n > 0) { s += ALPHABET[n % 62]; n /= 62; }
        return std::string(s.rbegin(), s.rend());
    }
};

class UrlCache {
    std::unordered_map<std::string, std::string> store_;
    std::mutex mu_;
public:
    std::optional<std::string> get(const std::string& code) {
        std::lock_guard lk(mu_);
        auto it = store_.find(code);
        if (it == store_.end()) return std::nullopt;
        return it->second;
    }
    void put(const std::string& code, const std::string& url) {
        std::lock_guard lk(mu_);
        store_[code] = url;
    }
};

class UrlDb {
    std::unordered_map<std::string, std::string> codeToUrl_;
    std::unordered_map<std::string, std::string> urlToCode_;
    std::mutex mu_;
public:
    bool save(const std::string& code, const std::string& url) {
        std::lock_guard lk(mu_);
        if (codeToUrl_.count(code)) return false;
        codeToUrl_[code] = url;
        urlToCode_[url] = code;
        return true;
    }
    std::optional<std::string> resolve(const std::string& code) {
        std::lock_guard lk(mu_);
        auto it = codeToUrl_.find(code);
        return it == codeToUrl_.end() ? std::nullopt : std::optional(it->second);
    }
    std::optional<std::string> codeFor(const std::string& url) {
        std::lock_guard lk(mu_);
        auto it = urlToCode_.find(url);
        return it == urlToCode_.end() ? std::nullopt : std::optional(it->second);
    }
};

class UrlShortener {
    UrlDb& db_;
    UrlCache& cache_;
    std::atomic<uint64_t> counter_{1000000};
    int dbHits_ = 0;
public:
    UrlShortener(UrlDb& d, UrlCache& c) : db_(d), cache_(c) {}

    std::string shorten(const std::string& longUrl, const std::string& alias = "") {
        if (auto existing = db_.codeFor(longUrl)) return *existing;
        std::string code = alias.empty() ? Base62::encode(counter_.fetch_add(1)) : alias;
        if (!db_.save(code, longUrl)) {
            if (!alias.empty()) throw std::runtime_error("alias taken");
            return shorten(longUrl);
        }
        cache_.put(code, longUrl);
        return code;
    }

    std::optional<std::string> expand(const std::string& code) {
        if (auto v = cache_.get(code)) return v;
        ++dbHits_;
        auto v = db_.resolve(code);
        if (v) cache_.put(code, *v);
        return v;
    }

    int dbHits() const { return dbHits_; }
};

int main() {
    UrlDb db;
    UrlCache cache;
    UrlShortener svc(db, cache);

    auto c1 = svc.shorten("https://example.com/very/long/path?x=1");
    auto c2 = svc.shorten("https://example.com/another");
    auto c3 = svc.shorten("https://example.com/very/long/path?x=1");
    auto c4 = svc.shorten("https://example.com/custom", "myAlias");

    std::cout << "code1=" << c1 << " code2=" << c2 << " code3=" << c3
              << " (dedup) alias=" << c4 << "\n";

    std::cout << "expand " << c1 << " -> " << svc.expand(c1).value_or("(nil)") << "\n";
    std::cout << "expand " << c1 << " (cached) -> " << svc.expand(c1).value_or("(nil)") << "\n";
    std::cout << "DB hits: " << svc.dbHits() << " (should be 1)\n";
    return 0;
}
