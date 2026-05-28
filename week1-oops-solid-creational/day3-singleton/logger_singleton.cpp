#include <iostream>
#include <mutex>
#include <string>

enum class Level { INFO, WARN, ERR };

class Logger {
public:
    static Logger& instance() {
        static Logger lg;
        return lg;
    }

    void log(Level lvl, const std::string& msg) {
        std::lock_guard<std::mutex> lk(mu_);
        std::cout << "[" << prefix(lvl) << "] " << msg << "\n";
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger() = default;
    std::mutex mu_;

    static const char* prefix(Level l) {
        switch (l) {
            case Level::INFO: return "INFO";
            case Level::WARN: return "WARN";
            case Level::ERR:  return "ERR";
        }
        return "?";
    }
};

int main() {
    Logger::instance().log(Level::INFO, "service started");
    Logger::instance().log(Level::WARN, "slow query detected");
    Logger::instance().log(Level::ERR,  "DB unreachable");
    return 0;
}
