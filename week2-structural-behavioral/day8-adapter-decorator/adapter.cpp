#include <iostream>
#include <memory>
#include <string>

class LegacyLogger {
public:
    void writeLog(const std::string& severity, const std::string& msg) {
        std::cout << "[LEGACY:" << severity << "] " << msg << "\n";
    }
};

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void info(const std::string& msg)  = 0;
    virtual void error(const std::string& msg) = 0;
};

class LegacyLoggerAdapter : public ILogger {
    std::shared_ptr<LegacyLogger> legacy_;
public:
    explicit LegacyLoggerAdapter(std::shared_ptr<LegacyLogger> l) : legacy_(std::move(l)) {}
    void info(const std::string& msg) override  { legacy_->writeLog("INFO",  msg); }
    void error(const std::string& msg) override { legacy_->writeLog("ERROR", msg); }
};

void runApp(ILogger& log) {
    log.info("starting up");
    log.error("connection failed");
}

int main() {
    auto legacy = std::make_shared<LegacyLogger>();
    LegacyLoggerAdapter adapter(legacy);
    runApp(adapter);
    return 0;
}
