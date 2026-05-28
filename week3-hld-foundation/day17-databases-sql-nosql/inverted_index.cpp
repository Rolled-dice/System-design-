#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class InvertedIndex {
    std::unordered_map<std::string, std::set<int>> postings_;

    static std::vector<std::string> tokenize(std::string s) {
        for (auto& c : s) c = std::tolower(static_cast<unsigned char>(c));
        std::vector<std::string> out;
        std::istringstream iss(s);
        std::string w;
        while (iss >> w) out.push_back(w);
        return out;
    }
public:
    void addDoc(int id, const std::string& text) {
        for (const auto& w : tokenize(text)) postings_[w].insert(id);
    }

    std::set<int> search(const std::string& query) {
        auto words = tokenize(query);
        if (words.empty()) return {};
        std::set<int> result = postings_[words[0]];
        for (size_t i = 1; i < words.size(); ++i) {
            std::set<int> tmp;
            const auto& cur = postings_[words[i]];
            std::set_intersection(result.begin(), result.end(),
                                  cur.begin(), cur.end(),
                                  std::inserter(tmp, tmp.begin()));
            result = std::move(tmp);
        }
        return result;
    }
};

int main() {
    InvertedIndex idx;
    idx.addDoc(1, "the quick brown fox");
    idx.addDoc(2, "the lazy dog sleeps");
    idx.addDoc(3, "quick brown dog runs");
    idx.addDoc(4, "fox and dog");

    auto r = idx.search("brown dog");
    std::cout << "matches for 'brown dog': ";
    for (int x : r) std::cout << x << " ";
    std::cout << "\n";

    auto r2 = idx.search("fox");
    std::cout << "matches for 'fox': ";
    for (int x : r2) std::cout << x << " ";
    std::cout << "\n";
    return 0;
}
