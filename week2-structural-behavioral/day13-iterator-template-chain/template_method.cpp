#include <iostream>
#include <string>

class DataProcessor {
public:
    virtual ~DataProcessor() = default;
    void process() {
        auto raw = read();
        auto transformed = transform(raw);
        write(transformed);
    }
protected:
    virtual std::string read() = 0;
    virtual std::string transform(const std::string& d) = 0;
    virtual void write(const std::string& d) = 0;
};

class CsvProcessor : public DataProcessor {
protected:
    std::string read() override { std::cout << "[CSV] read\n"; return "a,b,c"; }
    std::string transform(const std::string& d) override { std::cout << "[CSV] transform\n"; return d + "|done"; }
    void write(const std::string& d) override { std::cout << "[CSV] write: " << d << "\n"; }
};

class JsonProcessor : public DataProcessor {
protected:
    std::string read() override { std::cout << "[JSON] read\n"; return R"({"x":1})"; }
    std::string transform(const std::string& d) override { std::cout << "[JSON] transform\n"; return d + " /processed/"; }
    void write(const std::string& d) override { std::cout << "[JSON] write: " << d << "\n"; }
};

int main() {
    CsvProcessor c;  c.process();
    JsonProcessor j; j.process();
    return 0;
}
