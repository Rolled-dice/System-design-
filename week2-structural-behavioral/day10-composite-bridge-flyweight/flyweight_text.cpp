#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Glyph {
public:
    std::string font;
    int size;
    std::string color;
    Glyph(std::string f, int s, std::string c) : font(std::move(f)), size(s), color(std::move(c)) {}
    void render(char ch, int x, int y) const {
        std::cout << "'" << ch << "' at (" << x << "," << y << ") "
                  << font << "/" << size << "/" << color << "\n";
    }
};

class GlyphFactory {
    std::unordered_map<std::string, std::shared_ptr<Glyph>> cache_;
public:
    std::shared_ptr<Glyph> get(const std::string& font, int size, const std::string& color) {
        std::string key = font + "_" + std::to_string(size) + "_" + color;
        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;
        auto g = std::make_shared<Glyph>(font, size, color);
        cache_[key] = g;
        return g;
    }
    size_t unique() const { return cache_.size(); }
};

struct Char {
    char ch;
    int x, y;
    std::shared_ptr<Glyph> glyph;
};

int main() {
    GlyphFactory factory;
    std::vector<Char> doc;
    std::string text = "hello hello hello";
    auto g1 = factory.get("Arial", 12, "black");
    auto g2 = factory.get("Arial", 12, "red");

    int x = 0;
    for (char c : text) {
        doc.push_back({c, x, 0, (c == 'h' ? g2 : g1)});
        ++x;
    }
    for (const auto& d : doc) d.glyph->render(d.ch, d.x, d.y);
    std::cout << "Unique glyph objects: " << factory.unique() << "\n";
    return 0;
}
