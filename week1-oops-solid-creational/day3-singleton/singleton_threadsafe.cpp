#include <atomic>
#include <iostream>
#include <mutex>

class DbConnection {
public:
    static DbConnection* instance() {
        DbConnection* tmp = inst_.load(std::memory_order_acquire);
        if (!tmp) {
            std::lock_guard<std::mutex> lk(mu_);
            tmp = inst_.load(std::memory_order_relaxed);
            if (!tmp) {
                tmp = new DbConnection();
                inst_.store(tmp, std::memory_order_release);
            }
        }
        return tmp;
    }

    void query(const std::string& sql) { std::cout << "exec: " << sql << "\n"; }

    DbConnection(const DbConnection&) = delete;
    DbConnection& operator=(const DbConnection&) = delete;

private:
    DbConnection() { std::cout << "DB connected\n"; }

    static std::atomic<DbConnection*> inst_;
    static std::mutex mu_;
};

std::atomic<DbConnection*> DbConnection::inst_{nullptr};
std::mutex DbConnection::mu_;

int main() {
    DbConnection::instance()->query("SELECT 1");
    DbConnection::instance()->query("SELECT 2");
    return 0;
}
