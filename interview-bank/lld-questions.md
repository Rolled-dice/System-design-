# LLD Interview Question Bank

## How to Approach Any LLD Interview

1. **Clarify requirements** (5 min) - functional + non-functional, scale, must-have vs nice-to-have
2. **Identify core entities** - nouns from requirements
3. **Define relationships** + cardinality
4. **Define APIs / public methods** - verbs from requirements
5. **Apply patterns** - which design pattern fits each problem area?
6. **Code skeleton** - classes, interfaces, key methods (use C++ here)
7. **Walk through use cases** - happy path + edge cases
8. **Extensions** - how would you add X?

---

## Easy

| # | Problem | Patterns to use |
|---|---------|-----------------|
| 1 | Tic-Tac-Toe | Strategy (win check), State |
| 2 | Snake & Ladder | Strategy (dice), State |
| 3 | Vending Machine | State, Command, Strategy (payment) |
| 4 | Stack Overflow lite | Composite, Observer (notifications) |
| 5 | ATM | State (card states), Strategy (transaction type) |
| 6 | Logger | Singleton, Strategy (sink), Chain of Responsibility (level) |
| 7 | Pub/Sub system | Observer |
| 8 | Cache (LRU/LFU) | - |

## Medium

| # | Problem | Patterns to use |
|---|---------|-----------------|
| 9  | Parking Lot | Strategy (pricing, allocation), Factory, Singleton |
| 10 | Splitwise | Strategy (split type), Singleton, Observer |
| 11 | BookMyShow / IMDb | State (booking lifecycle), Singleton, Strategy (pricing) |
| 12 | Library Management | Strategy (search), Observer (overdue notification) |
| 13 | Online Shopping Cart | Strategy (discount, shipping, payment), Composite (cart items), Decorator |
| 14 | Hotel Booking | State, Strategy, Factory |
| 15 | Food Delivery (Swiggy/Zomato) | State, Observer, Strategy (matching) |
| 16 | Airline Reservation | State, Strategy, Composite (multi-leg) |
| 17 | Elevator System | State, Strategy (scheduling), Observer |
| 18 | Chess / Checkers | Strategy (move rules per piece), State, Memento (history) |
| 19 | Notification System | Strategy (channel), Observer, Chain of Responsibility |
| 20 | Online Code Editor | Memento, Command, Composite (tabs/files) |

## Hard

| # | Problem | Patterns to use |
|---|---------|-----------------|
| 21 | Uber-like rider/driver matching | Observer, Strategy (matching), State (trip lifecycle) |
| 22 | Concurrent Hash Map | - |
| 23 | Distributed Job Scheduler | Command, Strategy, State |
| 24 | Stock Trading System | Observer, State (order), Strategy (matching algorithm) |
| 25 | Word Search / Auto-suggest | Trie + composite |
| 26 | Calendar / Meeting scheduler | Strategy (recurrence), Composite (events), Observer |
| 27 | Distributed Cache (custom) | Strategy (eviction, sharding), Observer |
| 28 | Twitter LLD (within one server) | Observer, Strategy (timeline), Composite (tweet/retweet) |

---

## Per-Problem: What to Discuss

### Parking Lot
- Slot types vs vehicle types fit
- Concurrency: 2 cars, 2 entry gates, same last slot
- Pricing strategy hot-swappable
- Multi-floor expansion
- Reservations / electric charging

### Splitwise
- Balance representation (`map<user, map<user, amount>>`)
- Split types: equal, exact, percent
- Debt simplification (greedy max-creditor matches max-debtor)
- Multi-currency
- Concurrency on group balance

### BookMyShow
- Seat lock with TTL (5 min) before payment
- Race condition on same seat (lock per show)
- State transitions: AVAILABLE -> LOCKED -> BOOKED, LOCKED -> AVAILABLE on timeout
- Pricing per row/seat type
- Refund flow

### Tic-Tac-Toe
- O(1) win check via row/col/diag counters
- Extension to NxM, K-in-a-row
- Detect draw early

### Snake & Ladder
- Fair dice abstraction (mock for tests)
- Multi-player turn queue
- Detect winner at exact position

### Vending Machine
- States: IDLE, HAS_COIN, DISPENSING, OUT_OF_STOCK
- Command pattern for buttons
- Strategy for payment

### ATM
- States: CARD_INSERTED, AUTHENTICATING, READY, TRANSACTION
- Strategy for transactions (withdraw, deposit, balance)
- Daily limit + concurrency

### Library
- Membership tiers (decorator on user)
- Hold/reserve queue
- Fine calculation (strategy)

### Logger
- Singleton vs DI debate
- Levels via Chain of Responsibility
- Sinks: console, file, HTTP - Strategy

### Elevator
- Multi-elevator scheduling
- LOOK / SCAN / shortest-seek strategies
- State per elevator (UP, DOWN, IDLE)

### Online Cart
- Decorator for discounts (member, coupon, festive)
- Strategy for shipping, payment
- Composite for bundles

### Notification System
- Channels: email, SMS, push, in-app (Strategy)
- User preferences -> channel selection
- Retry + DLQ
- Rate limit per user/channel

---

## Common Mistakes to Avoid
- Jumping to code before clarifying
- Putting everything in one God class
- Missing concurrency consideration
- Not declaring interfaces (vs concrete classes)
- Ignoring extensibility - "what if we add X?"
- Forgetting validation / error handling
- No mention of testability
