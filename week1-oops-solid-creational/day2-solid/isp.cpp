#include <iostream>

class IPrinter {
public:
    virtual ~IPrinter() = default;
    virtual void print(const std::string& doc) = 0;
};

class IScanner {
public:
    virtual ~IScanner() = default;
    virtual void scan(const std::string& doc) = 0;
};

class IFax {
public:
    virtual ~IFax() = default;
    virtual void fax(const std::string& doc) = 0;
};

class SimplePrinter : public IPrinter {
public:
    void print(const std::string& doc) override { std::cout << "Printing: " << doc << "\n"; }
};

class MultiFunction : public IPrinter, public IScanner, public IFax {
public:
    void print(const std::string& doc) override { std::cout << "MFP print: " << doc << "\n"; }
    void scan(const std::string& doc) override  { std::cout << "MFP scan: "  << doc << "\n"; }
    void fax(const std::string& doc) override   { std::cout << "MFP fax: "   << doc << "\n"; }
};

int main() {
    SimplePrinter sp;
    sp.print("hello.pdf");

    MultiFunction mf;
    mf.print("doc.pdf");
    mf.scan("doc.pdf");
    mf.fax("doc.pdf");
    return 0;
}
