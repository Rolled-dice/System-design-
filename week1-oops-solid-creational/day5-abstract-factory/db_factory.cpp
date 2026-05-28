#include <iostream>
#include <memory>
#include <string>

class Connection {
public:
    virtual ~Connection() = default;
    virtual void connect() = 0;
};
class Command {
public:
    virtual ~Command() = default;
    virtual void execute(const std::string& sql) = 0;
};

class MySqlConnection : public Connection { public: void connect() override { std::cout << "[MySQL] connected\n"; } };
class PgConnection    : public Connection { public: void connect() override { std::cout << "[Postgres] connected\n"; } };
class MySqlCommand    : public Command    { public: void execute(const std::string& s) override { std::cout << "[MySQL] " << s << "\n"; } };
class PgCommand       : public Command    { public: void execute(const std::string& s) override { std::cout << "[Postgres] " << s << "\n"; } };

class DbFactory {
public:
    virtual ~DbFactory() = default;
    virtual std::unique_ptr<Connection> connection() = 0;
    virtual std::unique_ptr<Command>    command()    = 0;
};

class MySqlFactory : public DbFactory {
public:
    std::unique_ptr<Connection> connection() override { return std::make_unique<MySqlConnection>(); }
    std::unique_ptr<Command>    command()    override { return std::make_unique<MySqlCommand>(); }
};

class PgFactory : public DbFactory {
public:
    std::unique_ptr<Connection> connection() override { return std::make_unique<PgConnection>(); }
    std::unique_ptr<Command>    command()    override { return std::make_unique<PgCommand>(); }
};

int main() {
    std::unique_ptr<DbFactory> f = std::make_unique<MySqlFactory>();
    f->connection()->connect();
    f->command()->execute("SELECT * FROM users");

    f = std::make_unique<PgFactory>();
    f->connection()->connect();
    f->command()->execute("SELECT NOW()");
    return 0;
}
