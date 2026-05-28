#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

class IFileService {
public:
    virtual ~IFileService() = default;
    virtual void readFile(const std::string& path) = 0;
    virtual void deleteFile(const std::string& path) = 0;
};

class RealFileService : public IFileService {
public:
    void readFile(const std::string& path) override   { std::cout << "Read "   << path << "\n"; }
    void deleteFile(const std::string& path) override { std::cout << "Deleted "<< path << "\n"; }
};

class ProtectionProxy : public IFileService {
    std::shared_ptr<IFileService> real_;
    std::string role_;
public:
    ProtectionProxy(std::shared_ptr<IFileService> r, std::string role)
        : real_(std::move(r)), role_(std::move(role)) {}

    void readFile(const std::string& path) override { real_->readFile(path); }
    void deleteFile(const std::string& path) override {
        if (role_ != "admin") throw std::runtime_error("Forbidden: not admin");
        real_->deleteFile(path);
    }
};

int main() {
    auto real = std::make_shared<RealFileService>();
    ProtectionProxy admin(real, "admin");
    ProtectionProxy guest(real, "guest");

    admin.deleteFile("/etc/passwd");
    try { guest.deleteFile("/etc/passwd"); }
    catch (const std::exception& e) { std::cout << e.what() << "\n"; }
    return 0;
}
