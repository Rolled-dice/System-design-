#include <iostream>
#include <string>
#include <vector>

enum class Mark { EMPTY, X, O };
enum class GameState { ONGOING, X_WINS, O_WINS, DRAW };

class TicTacToe {
    int n_;
    std::vector<std::vector<Mark>> board_;
    std::vector<int> rowX_, rowO_, colX_, colO_;
    int diagX_ = 0, diagO_ = 0, antiX_ = 0, antiO_ = 0;
    int filled_ = 0;
    Mark turn_ = Mark::X;
    GameState state_ = GameState::ONGOING;

    static char ch(Mark m) { return m == Mark::X ? 'X' : m == Mark::O ? 'O' : '.'; }
public:
    explicit TicTacToe(int n = 3)
        : n_(n), board_(n, std::vector<Mark>(n, Mark::EMPTY)),
          rowX_(n), rowO_(n), colX_(n), colO_(n) {}

    bool play(int r, int c) {
        if (state_ != GameState::ONGOING) return false;
        if (r < 0 || r >= n_ || c < 0 || c >= n_) return false;
        if (board_[r][c] != Mark::EMPTY) return false;

        board_[r][c] = turn_;
        ++filled_;
        auto& row = (turn_ == Mark::X) ? rowX_ : rowO_;
        auto& col = (turn_ == Mark::X) ? colX_ : colO_;
        int& diag = (turn_ == Mark::X) ? diagX_ : diagO_;
        int& anti = (turn_ == Mark::X) ? antiX_ : antiO_;
        ++row[r]; ++col[c];
        if (r == c) ++diag;
        if (r + c == n_ - 1) ++anti;

        bool win = row[r] == n_ || col[c] == n_ || diag == n_ || anti == n_;
        if (win) state_ = (turn_ == Mark::X) ? GameState::X_WINS : GameState::O_WINS;
        else if (filled_ == n_ * n_) state_ = GameState::DRAW;
        else turn_ = (turn_ == Mark::X) ? Mark::O : Mark::X;
        return true;
    }

    GameState state() const { return state_; }

    void print() const {
        for (int i = 0; i < n_; ++i) {
            for (int j = 0; j < n_; ++j) std::cout << ch(board_[i][j]) << ' ';
            std::cout << "\n";
        }
        std::cout << "---\n";
    }
};

int main() {
    TicTacToe g(3);
    g.play(0, 0); g.play(1, 1);
    g.play(0, 1); g.play(2, 2);
    g.play(0, 2);
    g.print();
    std::cout << "X wins? " << (g.state() == GameState::X_WINS) << "\n";
    return 0;
}
