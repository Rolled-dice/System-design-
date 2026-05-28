#include <iostream>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

class Dice {
public:
    virtual ~Dice() = default;
    virtual int roll() = 0;
};

class StandardDice : public Dice {
    std::mt19937 rng_;
    std::uniform_int_distribution<int> dist_;
public:
    explicit StandardDice(int seed = 42) : rng_(seed), dist_(1, 6) {}
    int roll() override { return dist_(rng_); }
};

class Player {
public:
    std::string name;
    int pos = 0;
    explicit Player(std::string n) : name(std::move(n)) {}
};

class Board {
    int size_;
    std::unordered_map<int, int> snakes_;
    std::unordered_map<int, int> ladders_;
public:
    explicit Board(int size = 100) : size_(size) {
        snakes_  = {{17, 7}, {54, 34}, {62, 19}, {98, 79}};
        ladders_ = {{3, 22}, {8, 30}, {28, 84}, {58, 77}};
    }
    int size() const { return size_; }
    int adjust(int pos) const {
        if (auto it = snakes_.find(pos);  it != snakes_.end()) return it->second;
        if (auto it = ladders_.find(pos); it != ladders_.end()) return it->second;
        return pos;
    }
};

class Game {
    Board board_;
    std::unique_ptr<Dice> dice_;
    std::queue<Player> turns_;
public:
    Game(Board b, std::unique_ptr<Dice> d, const std::vector<std::string>& names)
        : board_(b), dice_(std::move(d)) {
        for (const auto& n : names) turns_.push(Player(n));
    }
    Player play() {
        while (true) {
            Player p = turns_.front(); turns_.pop();
            int r = dice_->roll();
            int next = p.pos + r;
            if (next > board_.size()) { turns_.push(p); continue; }
            int adjusted = board_.adjust(next);
            if (adjusted != next)
                std::cout << p.name << " rolled " << r << " -> " << next << " warped -> " << adjusted << "\n";
            else
                std::cout << p.name << " rolled " << r << " -> " << adjusted << "\n";
            p.pos = adjusted;
            if (p.pos == board_.size()) { std::cout << p.name << " WINS!\n"; return p; }
            turns_.push(p);
        }
    }
};

int main() {
    Game g(Board(100), std::make_unique<StandardDice>(7), {"Alice", "Bob"});
    g.play();
    return 0;
}
