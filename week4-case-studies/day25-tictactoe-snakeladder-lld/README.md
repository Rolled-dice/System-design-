# Day 25 - LLD: Tic-Tac-Toe + Snake & Ladder

## Tic-Tac-Toe

### Requirements
- 2 players, NxN board (default 3)
- Take turns placing X / O
- Detect win (row, col, diagonals), draw, ongoing

### Patterns
- Strategy for win-detection (extensible to bigger boards / NxM)
- Iterator (positions)

## Snake & Ladder

### Requirements
- NxN board, snakes pull down, ladders go up
- 2-N players, dice 1-6
- Roll, move, apply snake/ladder, switch turn, detect winner

### Patterns
- Strategy for dice (fair, weighted)
- State for player turn
- Observer (notify on win)

## Files
- [tic_tac_toe.cpp](tic_tac_toe.cpp)
- [snake_ladder.cpp](snake_ladder.cpp)

## Interview Questions
1. How to detect win in O(1) instead of O(N) on each move? (Track row/col/diag counters.)
2. How to extend Tic-Tac-Toe to Connect-4?
3. Snake & Ladder - how to fair-shuffle dice for online multiplayer? (Server-authoritative)
4. How to add bots / AI players?
5. How to persist game state for resume?

## Daily Assignment
1. Tic-Tac-Toe: extend to N-in-a-row on MxN board.
2. Snake & Ladder: support 4 players, with a custom dice strategy.
3. Add unit tests using `assert()` statements.
4. Bonus: implement minimax for unbeatable Tic-Tac-Toe AI.
