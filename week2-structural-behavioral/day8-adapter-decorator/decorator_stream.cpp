#include <iostream>
#include <memory>
#include <string>

class DataSource {
public:
    virtual ~DataSource() = default;
    virtual void write(const std::string& data) = 0;
    virtual std::string read() = 0;
};

class RawStream : public DataSource {
    std::string buf_;
public:
    void write(const std::string& d) override { buf_ = d; }
    std::string read() override { return buf_; }
};

class StreamDecorator : public DataSource {
protected:
    std::unique_ptr<DataSource> wrappee_;
public:
    explicit StreamDecorator(std::unique_ptr<DataSource> s) : wrappee_(std::move(s)) {}
};

class CompressionDecorator : public StreamDecorator {
public:
    using StreamDecorator::StreamDecorator;
    void write(const std::string& d) override {
        std::string c = "C(" + d + ")";
        wrappee_->write(c);
    }
    std::string read() override {
        std::string raw = wrappee_->read();
        return "decompressed[" + raw + "]";
    }
};

class EncryptionDecorator : public StreamDecorator {
public:
    using StreamDecorator::StreamDecorator;
    void write(const std::string& d) override {
        std::string e = "E{" + d + "}";
        wrappee_->write(e);
    }
    std::string read() override {
        std::string raw = wrappee_->read();
        return "decrypted<" + raw + ">";
    }
};

int main() {
    std::unique_ptr<DataSource> ds = std::make_unique<RawStream>();
    ds = std::make_unique<CompressionDecorator>(std::move(ds));
    ds = std::make_unique<EncryptionDecorator>(std::move(ds));

    ds->write("hello world");
    std::cout << ds->read() << "\n";
    return 0;
}
