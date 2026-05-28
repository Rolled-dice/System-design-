#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct Employee {
    int id;
    std::string name;
    double salary;
};

class EmployeeRepository {
public:
    void save(const Employee& e) {
        std::cout << "[DB] saved " << e.name << "\n";
    }
};

class SalaryCalculator {
public:
    double netSalary(const Employee& e) const {
        return e.salary * 0.8;
    }
};

class ReportGenerator {
public:
    void generate(const std::vector<Employee>& employees, const std::string& path) {
        std::ofstream f(path);
        for (const auto& e : employees) f << e.id << "," << e.name << "," << e.salary << "\n";
        std::cout << "[Report] written to " << path << "\n";
    }
};

class EmailService {
public:
    void send(const Employee& e, const std::string& msg) {
        std::cout << "[Email] to " << e.name << ": " << msg << "\n";
    }
};

int main() {
    Employee e{1, "Alice", 100000};
    EmployeeRepository repo;
    SalaryCalculator calc;
    ReportGenerator rg;
    EmailService es;

    repo.save(e);
    std::cout << "Net: " << calc.netSalary(e) << "\n";
    rg.generate({e}, "/tmp/report.csv");
    es.send(e, "Welcome!");
    return 0;
}
