#include <iostream>
#include <map>
#include <string>

class Config {
public:
    static Config& instance() {
        static Config cfg;
        return cfg;
    }

    void set(const std::string& k, const std::string& v) { data_[k] = v; }
    std::string get(const std::string& k) const {
        auto it = data_.find(k);
        return it != data_.end() ? it->second : "";
    }

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

private:
    Config() { std::cout << "Config initialized\n"; }
    ~Config() = default;
    std::map<std::string, std::string> data_;
};

int main() {
    Config::instance().set("db.host", "localhost");
    std::cout << "host=" << Config::instance().get("db.host") << "\n";
    return 0;
}
