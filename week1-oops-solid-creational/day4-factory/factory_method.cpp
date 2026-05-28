#include <iostream>
#include <memory>

class Document {
public:
    virtual ~Document() = default;
    virtual void open() = 0;
    virtual void save() = 0;
};

class PdfDocument : public Document {
public:
    void open() override { std::cout << "PDF opened\n"; }
    void save() override { std::cout << "PDF saved\n"; }
};

class WordDocument : public Document {
public:
    void open() override { std::cout << "Word opened\n"; }
    void save() override { std::cout << "Word saved\n"; }
};

class Application {
public:
    virtual ~Application() = default;
    virtual std::unique_ptr<Document> createDocument() = 0;

    void newFile() {
        auto doc = createDocument();
        doc->open();
        doc->save();
    }
};

class PdfApp : public Application {
public:
    std::unique_ptr<Document> createDocument() override {
        return std::make_unique<PdfDocument>();
    }
};

class WordApp : public Application {
public:
    std::unique_ptr<Document> createDocument() override {
        return std::make_unique<WordDocument>();
    }
};

int main() {
    std::unique_ptr<Application> app = std::make_unique<PdfApp>();
    app->newFile();
    app = std::make_unique<WordApp>();
    app->newFile();
    return 0;
}
