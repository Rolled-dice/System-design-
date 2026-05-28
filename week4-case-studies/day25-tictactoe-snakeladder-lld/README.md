# Day 25 - LLD: Tic-Tac-Toe + Snake & Ladder

## LLD Interview Approach (Quick Reference)

Refer to [Day 22](../day22-parking-lot-lld/README.md) for the full 6-step LLD framework. Applied to game design:

1. **Clarify**: Board game with turn-based mechanics, win/loss detection, extensible rules
2. **Entities**: Game, Board, Player, Move, Dice (for S&L), Cell
3. **Relationships**: Game has Board, Board has Cells, Game has Players, Players make Moves
4. **Patterns**: State (game lifecycle), Command (moves with undo), Strategy (dice, win detection), Builder (board config)
5. **Concurrency**: Multiplayer online games need turn validation and state sync
6. **Extensibility**: NxN boards, AI players, networked multiplayer, custom rules

---

## Game Design LLD Theory

### Core Principle: Separate Game Logic from Rendering

The most important architectural decision in game LLD is separating the game engine (rules, state, validation) from the presentation layer (console, GUI, network):

```
+-------------------+     +------------------+     +----------------+
| Input Handler     | --> | Game Engine      | --> | Renderer       |
| (Console/Network) |     | (Pure Logic)     |     | (Console/GUI)  |
+-------------------+     +------------------+     +----------------+
                               |
                          Pure functions:
                          - validateMove(state, move) -> bool
                          - applyMove(state, move) -> newState
                          - checkWin(state) -> Result
                          - nextTurn(state) -> Player
```

**Why this matters**:
- Game engine can be unit tested without any I/O
- Same engine works for console, GUI, web, or networked play
- AI players interact with the same interface as human players
- Replay/undo is trivial (just reapply moves to initial state)

### Game Lifecycle State Machine

```
    create()         start()          move()           end condition
+----------+  -->  +----------+  -->  +------------+  -->  +-----------+
| CREATED  |       | WAITING  |       | IN_PROGRESS|       | COMPLETED |
| (setup)  |       | (players)|       | (playing)  |       | (result)  |
+----------+       +----------+       +------------+       +-----------+
                        |                    |                     |
                        | timeout            | player_disconnect   | 
                        v                    v                     |
                   +----------+        +----------+               |
                   | CANCELLED |       | PAUSED   |               |
                   +----------+        +----------+               |
                                            |                     |
                                            | resume/timeout      |
                                            v                     |
                                       +-----------+              |
                                       | FORFEITED | <------------+
                                       +-----------+   (timeout)
```

### State Transitions

| From | To | Trigger | Validation |
|------|----|---------|------------|
| CREATED | WAITING | Host creates game | Valid board config |
| WAITING | IN_PROGRESS | All players joined + host starts | Min players met |
| IN_PROGRESS | IN_PROGRESS | Valid move made | Move is legal, correct player's turn |
| IN_PROGRESS | COMPLETED | Win/draw detected | Win condition check passes |
| IN_PROGRESS | PAUSED | Player disconnects (online) | Grace period starts |
| PAUSED | IN_PROGRESS | Player reconnects | Within grace period |
| PAUSED | FORFEITED | Grace period expires | Timer elapsed |

### Command Pattern for Moves (Enables Undo)

```cpp
class MoveCommand {
public:
    virtual void execute(Board& board) = 0;
    virtual void undo(Board& board) = 0;
    virtual ~MoveCommand() = default;
};

class PlacePieceCommand : public MoveCommand {
    int row, col;
    char piece; // 'X' or 'O'
public:
    void execute(Board& board) override {
        board.place(row, col, piece);
    }
    void undo(Board& board) override {
        board.clear(row, col);
    }
};

class GameHistory {
    stack<unique_ptr<MoveCommand>> moves;
public:
    void executeMove(unique_ptr<MoveCommand> cmd, Board& board) {
        cmd->execute(board);
        moves.push(std::move(cmd));
    }
    void undoLastMove(Board& board) {
        if (!moves.empty()) {
            moves.top()->undo(board);
            moves.pop();
        }
    }
};
```

**Why Command Pattern?**
- Undo/redo is trivial (pop from stack, call undo)
- Game replay: store all commands, replay sequentially
- Network sync: send commands over network, apply on remote board
- AI analysis: try a move (execute), evaluate, undo, try another

### Event-Driven Architecture for Game Events

```cpp
enum class GameEvent {
    MOVE_MADE, GAME_WON, GAME_DRAW, TURN_CHANGED,
    PLAYER_JOINED, PLAYER_LEFT, TIMER_EXPIRED
};

class GameEventListener {
public:
    virtual void onEvent(GameEvent event, const EventData& data) = 0;
};

class Game {
    vector<GameEventListener*> listeners;
    
    void makeMove(Move m) {
        // Apply move logic...
        notify(GameEvent::MOVE_MADE, {m});
        
        if (checkWin()) notify(GameEvent::GAME_WON, {currentPlayer});
        else if (checkDraw()) notify(GameEvent::GAME_DRAW, {});
        else { switchTurn(); notify(GameEvent::TURN_CHANGED, {nextPlayer}); }
    }
};

// Listeners: ConsoleRenderer, SoundEffects, NetworkSync, Analytics
```

---

## Part A: Tic-Tac-Toe Deep Dive

### Board Representation Strategies

| Strategy | Storage | Move | Win Check | Best For |
|----------|---------|------|-----------|----------|
| 2D array `char[N][N]` | O(N^2) | O(1) | O(N) | Simple, readable |
| 1D array `char[N*N]` | O(N^2) | O(1) | O(N) | Cache-friendly |
| Bitmask (2 ints for X, O) | O(1) | O(1) | O(1) | Performance-critical, 3x3 only |
| Row/col/diag counters | O(N) | O(1) | O(1) | General NxN with O(1) win check |

### O(1) Win Detection Algorithm

The key insight: instead of scanning the entire board after each move, maintain running counters.

**The Math**:
- For an NxN board with 2 players (+1 for X, -1 for O):
- Maintain: `rowCount[N]`, `colCount[N]`, `diagCount`, `antiDiagCount`
- On move at (r, c) by player p (where p = +1 or -1):
  - `rowCount[r] += p`
  - `colCount[c] += p`
  - if (r == c): `diagCount += p`
  - if (r + c == N-1): `antiDiagCount += p`
- Win condition: any counter reaches +N (X wins) or -N (O wins)

```cpp
class TicTacToe {
    int N;
    vector<int> rows, cols;
    int diag = 0, antiDiag = 0;
    
public:
    TicTacToe(int n) : N(n), rows(n, 0), cols(n, 0) {}
    
    // Returns 0 if no win, 1 if player 1 wins, 2 if player 2 wins
    int move(int row, int col, int player) {
        int val = (player == 1) ? 1 : -1;
        
        rows[row] += val;
        cols[col] += val;
        if (row == col) diag += val;
        if (row + col == N - 1) antiDiag += val;
        
        // Check if this move caused a win
        if (abs(rows[row]) == N || abs(cols[col]) == N ||
            abs(diag) == N || abs(antiDiag) == N) {
            return player;
        }
        return 0; // No winner yet
    }
};
```

**Complexity Analysis**:
- Time per move: O(1) - just update 2-4 counters and check
- Space: O(N) - for row and column arrays
- Compare to naive scan: O(N) per move (check row + col + diags)
- For 3x3 the difference is negligible, but for 100x100 it matters

**Why this works mathematically**: A player wins if and only if they fill an entire row, column, or diagonal. By tracking the cumulative "ownership" of each line, we know a line is complete when its counter reaches N (all cells filled by same player). The use of +1/-1 ensures the two players' contributions are distinguishable.

### NxN Generalization

```cpp
class GeneralizedTicTacToe {
    int N;          // Board size
    int K;          // Win condition: K in a row (K <= N)
    // For K < N, the counter approach does not work directly
    // Need sliding window or different approach
    
    // Approach: for K == N, use counters (O(1))
    // For K < N (e.g., Connect-4 style), use directional scan from last move
    
    int checkWinFromMove(int row, int col, int player) {
        // Check 4 directions from (row, col): horizontal, vertical, 2 diagonals
        // Count consecutive same-player pieces in each direction
        // If any direction has K consecutive -> win
        // Time: O(K) per move (check at most K cells in each of 4 directions)
    }
};
```

### Multiplayer Extension (> 2 Players)

For M players on NxN board, K in a row to win:

```
Changes needed:
1. Turn order: circular (player 1 -> 2 -> ... -> M -> 1 -> ...)
2. Win check: each player has their own set of counters
   - rows[player][N], cols[player][N], diag[player], antiDiag[player]
3. Draw condition: board full AND no winner
4. Elimination: optional rule where a player surrounded loses

Counter approach still works: O(1) per move, O(M*N) space total
```

### Minimax AI (Unbeatable for 3x3)

```cpp
int minimax(Board& board, int depth, bool isMaximizing, int alpha, int beta) {
    int score = evaluate(board);
    if (score != 0 || board.isFull()) return score - depth * sign(score);
    // Subtract depth to prefer faster wins
    
    if (isMaximizing) {
        int best = INT_MIN;
        for (auto& move : board.getAvailableMoves()) {
            board.place(move, 'X');
            best = max(best, minimax(board, depth + 1, false, alpha, beta));
            board.undo(move);
            alpha = max(alpha, best);
            if (beta <= alpha) break; // Alpha-beta pruning
        }
        return best;
    } else {
        // Similar for minimizing player
    }
}
```

**Complexity**: Without pruning O(9!) = 362,880 for 3x3. With alpha-beta pruning: effectively O(sqrt(9!)) in practice. For larger boards, use heuristic evaluation + depth limit.

---

## Part B: Snake & Ladder Deep Dive

### Board Configuration with Builder Pattern

```cpp
class BoardBuilder {
    int size;
    map<int, int> snakes;  // head -> tail (head > tail)
    map<int, int> ladders; // bottom -> top (bottom < top)
    
public:
    BoardBuilder(int sz) : size(sz) {}
    
    BoardBuilder& addSnake(int head, int tail) {
        assert(head > tail); // Snake goes DOWN
        assert(snakes.find(head) == snakes.end()); // No duplicate at same cell
        assert(ladders.find(head) == ladders.end()); // Can't have both at same cell
        snakes[head] = tail;
        return *this;
    }
    
    BoardBuilder& addLadder(int bottom, int top) {
        assert(top > bottom); // Ladder goes UP
        assert(ladders.find(bottom) == ladders.end());
        assert(snakes.find(bottom) == snakes.end());
        ladders[bottom] = top;
        return *this;
    }
    
    Board build() {
        // Validate: no cycles (snake bottom at ladder top that leads to snake head)
        // Validate: all positions within board bounds
        // Validate: winning cell (100) has no snake
        return Board(size, snakes, ladders);
    }
};

// Usage:
Board board = BoardBuilder(100)
    .addSnake(99, 4)
    .addSnake(62, 19)
    .addLadder(3, 51)
    .addLadder(22, 58)
    .build();
```

**Why Builder?**
- Board configuration has many optional elements (snakes, ladders, power-ups)
- Validation can happen at build time (fail fast)
- Same builder creates standard boards, custom boards, or randomly-generated boards
- Immutable Board after construction (thread-safe for reads)

### Dice Abstraction with Strategy Pattern

```cpp
class DiceStrategy {
public:
    virtual int roll() = 0;
    virtual int minValue() = 0;
    virtual int maxValue() = 0;
    virtual ~DiceStrategy() = default;
};

class FairDice : public DiceStrategy {
    int faces;
    mt19937 rng;
public:
    FairDice(int f = 6) : faces(f), rng(random_device{}()) {}
    int roll() override { return uniform_int_distribution<>(1, faces)(rng); }
    int minValue() override { return 1; }
    int maxValue() override { return faces; }
};

class LoadedDice : public DiceStrategy {
    // Higher probability for 6
    discrete_distribution<> dist;
    mt19937 rng;
public:
    LoadedDice() : dist({1, 1, 1, 1, 1, 3}), rng(random_device{}()) {} // 6 is 3x likely
    int roll() override { return dist(rng) + 1; }
};

class DoubleDice : public DiceStrategy {
    // Two dice, sum of both (2-12 range)
    mt19937 rng;
public:
    int roll() override {
        auto d = uniform_int_distribution<>(1, 6);
        return d(rng) + d(rng);
    }
    int minValue() override { return 2; }
    int maxValue() override { return 12; }
};

class FixedDice : public DiceStrategy {
    // For testing - returns predetermined sequence
    queue<int> values;
public:
    FixedDice(vector<int> seq) { for (int v : seq) values.push(v); }
    int roll() override { int v = values.front(); values.pop(); return v; }
};
```

**Why Strategy for Dice?**
- Testing: `FixedDice` gives deterministic game sequences for unit tests
- Game variants: double dice, loaded dice, custom dice for power-ups
- Online multiplayer: server-authoritative dice (generate on server, send to clients)
- Same game logic works regardless of dice implementation

### Movement Rules and Edge Cases

```cpp
class GameEngine {
    Board board;
    vector<Player> players;
    int currentPlayerIndex = 0;
    DiceStrategy* dice;
    
    MoveResult processMove(int playerId) {
        int roll = dice->roll();
        Player& p = players[currentPlayerIndex];
        int newPos = p.position + roll;
        
        // Rule 1: Cannot exceed final cell
        if (newPos > board.size()) {
            // Option A: Stay in place (common rule)
            return MoveResult{roll, p.position, p.position, STAY};
            // Option B: Bounce back (variant rule)
            // newPos = board.size() - (newPos - board.size());
        }
        
        // Rule 2: Apply snake or ladder
        if (board.hasSnake(newPos)) {
            int finalPos = board.getSnakeTail(newPos);
            return MoveResult{roll, p.position, finalPos, SNAKE};
        }
        if (board.hasLadder(newPos)) {
            int finalPos = board.getLadderTop(newPos);
            return MoveResult{roll, p.position, finalPos, LADDER};
        }
        
        // Rule 3: Exact landing on final cell = WIN
        if (newPos == board.size()) {
            p.position = newPos;
            return MoveResult{roll, p.position - roll, newPos, WIN};
        }
        
        p.position = newPos;
        return MoveResult{roll, newPos - roll, newPos, NORMAL};
    }
};
```

### Game Loop Separation from Rendering

```cpp
// Pure game logic - no I/O
class SnakeLadderGame {
public:
    GameState getState() const;       // Current board state
    MoveResult makeMove();            // Roll dice, move current player
    bool isOver() const;              // Check if game ended
    Player getWinner() const;         // Who won
    Player getCurrentPlayer() const;  // Whose turn
};

// Rendering layer - handles I/O
class ConsoleRenderer {
    SnakeLadderGame& game;
public:
    void displayBoard() {
        auto state = game.getState();
        // Print board with player positions, snakes, ladders
    }
    void displayMove(MoveResult result) {
        cout << result.player.name << " rolled " << result.diceValue
             << ", moved from " << result.from << " to " << result.to;
        if (result.type == SNAKE) cout << " (bitten by snake!)";
        if (result.type == LADDER) cout << " (climbed ladder!)";
    }
};

// Game loop (orchestrator)
void playGame() {
    SnakeLadderGame game(board, players, dice);
    ConsoleRenderer renderer(game);
    
    renderer.displayBoard();
    while (!game.isOver()) {
        renderer.promptPlayer(game.getCurrentPlayer());
        MoveResult result = game.makeMove();
        renderer.displayMove(result);
        renderer.displayBoard();
    }
    renderer.displayWinner(game.getWinner());
}
```

**Why separate?** The same `SnakeLadderGame` class works with:
- Console renderer (text-based)
- GUI renderer (graphical board)
- Network renderer (send state to remote clients)
- Test harness (no rendering, just validate game logic)

---

## Design Principles Applied

### Testability of Pure Game State

```cpp
// Test: Snake sends player back
TEST(SnakeLadder, SnakeMoveTest) {
    Board board = BoardBuilder(100).addSnake(50, 10).build();
    vector<Player> players = {Player("Alice"), Player("Bob")};
    FixedDice dice({50}); // Force roll of 50 (reach snake head)
    
    SnakeLadderGame game(board, players, &dice);
    players[0].position = 0;
    
    MoveResult result = game.makeMove();
    
    ASSERT_EQ(result.to, 10);        // Landed on snake tail
    ASSERT_EQ(result.type, SNAKE);   // Was a snake event
}

// Test: Win requires exact landing
TEST(TicTacToe, WinDetectionO1) {
    TicTacToe game(3);
    
    game.move(0, 0, 1); // X at (0,0)
    game.move(1, 0, 2); // O at (1,0)
    game.move(0, 1, 1); // X at (0,1)
    game.move(1, 1, 2); // O at (1,1)
    int result = game.move(0, 2, 1); // X at (0,2) - completes row 0
    
    ASSERT_EQ(result, 1); // Player 1 (X) wins
}
```

### Separation of Concerns Summary

```
+--------------------------------------------------------------------+
| Layer           | Responsibility          | Depends On              |
+-----------------+-------------------------+-------------------------+
| Game Engine     | Rules, state, validation| Nothing (pure logic)    |
| Command/History | Move tracking, undo     | Game Engine              |
| Player (Human)  | Input capture           | Game Engine interface    |
| Player (AI)     | Move computation        | Game Engine interface    |
| Renderer        | Display state           | Game Engine (read-only)  |
| Network Sync    | Multiplayer state sync  | Game Engine + Commands   |
| Persistence     | Save/load game          | Game Engine state        |
+--------------------------------------------------------------------+
```

Each layer can be developed, tested, and replaced independently.

---

## Extensibility Examples

### Adding a New Game (Chess) to the Framework

With the architecture above, a new game requires:
1. Implement `Board` (8x8 with piece positions)
2. Implement `MoveValidator` (per-piece legal moves)
3. Implement `WinChecker` (checkmate detection)
4. Reuse: Command pattern, game lifecycle state machine, event system, renderer interface

The game framework is reusable. Only game-specific rules change.

### Online Multiplayer Extension

```
Client A                    Server                    Client B
   |                          |                          |
   | -- makeMove(cmd) -----> |                          |
   |                          | -- validate(cmd)         |
   |                          | -- apply(cmd)            |
   |                          | -- broadcast(cmd) -----> |
   |                          |                          | -- apply(cmd)
   | <-- ack + newState ----- |                          |
   |                          |                          |
```

Server is authoritative (validates all moves). Clients send commands, receive confirmed state. This prevents cheating and ensures consistency.

---

## Patterns Used Summary

| Pattern | Where | Why (Trade-off Reasoning) |
|---------|-------|---------------------------|
| State | Game lifecycle (Created -> Playing -> Ended) | Clear state transitions, prevent invalid operations |
| Command | Moves (execute/undo) | Enables undo, replay, network sync |
| Strategy | Dice types, win detection algorithms | Swap behaviors without changing game loop |
| Builder | Board configuration (snakes, ladders, size) | Complex construction with validation |
| Observer | Game events (move, win, turn change) | Decouple rendering from logic |
| Template Method | Game loop (setup -> play -> end) | Shared structure, game-specific steps |

## Files
- [tic_tac_toe.cpp](tic_tac_toe.cpp)
- [snake_ladder.cpp](snake_ladder.cpp)

## Interview Questions
1. How to detect win in O(1) instead of O(N) on each move? (Row/col/diag counters with +1/-1 per player)
2. Prove the counter-based approach is correct. (Player wins iff counter reaches +N or -N; only one player can reach this)
3. How to extend Tic-Tac-Toe to Connect-4? (Change K < N, use directional scan O(K) from last move)
4. Snake & Ladder: how to ensure fair dice in online multiplayer? (Server-authoritative generation, signed random)
5. How to add bots / AI players? (Same Player interface, different strategy: random, minimax, heuristic)
6. How to persist game state for resume? (Serialize board + move history, replay commands on load)
7. How would you handle disconnection in online play? (Grace period -> timeout -> forfeit state transition)
8. Why Command pattern over just storing board snapshots? (Commands are tiny, snapshots are large; commands enable selective undo)
9. How to implement a tournament system on top? (Bracket state machine, composed of individual game state machines)
10. How to prevent move conflicts in real-time multiplayer? (Server validates, rejects invalid, broadcasts only valid moves)

## Daily Assignment
1. Tic-Tac-Toe: implement O(1) win detection with row/col/diag counters for NxN board.
2. Snake & Ladder: support 4 players with DoubleDice strategy and Builder-configured board.
3. Add undo functionality using Command pattern (at least 3 moves back).
4. Implement minimax with alpha-beta pruning for unbeatable 3x3 Tic-Tac-Toe AI.
5. Add game event system and write a test that replays a complete game from command history.
