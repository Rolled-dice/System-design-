# Day 23 - LLD: Splitwise

## LLD Interview Approach (Quick Reference)

Refer to [Day 22](../day22-parking-lot-lld/README.md) for the full 6-step LLD framework. Here we apply it directly:

1. **Clarify**: Multi-user expense splitting with group management, multiple split types, debt simplification
2. **Entities**: User, Group, Expense, Split, Balance, Settlement
3. **Relationships**: Group contains Users, Expense belongs to Group, Expense has Splits, Balances derived from Splits
4. **Patterns**: Strategy (split logic), Observer (notifications), Singleton (ExpenseManager), Factory (split creation)
5. **Concurrency**: Simultaneous expense additions in same group, balance consistency
6. **Extensibility**: Multi-currency, recurring expenses, payment integration

---

## Problem Statement

Design an expense-sharing application (like Splitwise) where:
- Users create groups and log shared expenses
- Expenses can be split equally, by exact amounts, or by percentage
- System maintains a balance sheet showing who owes whom
- Debts can be simplified to minimize the number of transactions needed to settle

---

## Requirements Analysis

### Functional Requirements
1. Create users and groups
2. Add expense with split type: EQUAL, EXACT, PERCENT
3. Show balance for a user (who they owe, who owes them)
4. Settle up - transfer money to clear debt
5. Simplify debts (minimize number of transfers in a group)
6. Expense history and audit trail
7. Group management (add/remove members)

### Non-Functional Requirements
- Strong consistency for balance calculations (no money appears/disappears)
- Concurrent expense additions must not corrupt balances
- Audit trail must be immutable
- Low-latency balance queries (precomputed, not recalculated)

---

## Graph-Based Debt Simplification Algorithm

### Why Naive Pairwise Settlement Fails

Consider a group of 4 people:
```
Alice owes Bob: $10
Bob owes Charlie: $10
Charlie owes Alice: $10
Alice owes David: $5
David owes Bob: $5
```

Naive approach: settle each pair independently = 5 transactions.

But notice: Alice owes Bob $10, Bob owes Charlie $10, Charlie owes Alice $10 - this is a cycle! It cancels out perfectly. Net result: Alice owes David $5, David owes Bob $5 = only 2 transactions needed.

### Modeling as a Directed Graph

Each person is a node. Each debt is a directed edge with weight (amount owed).

```
Step 1: Build the debt graph
    Alice --$10--> Bob --$10--> Charlie --$10--> Alice
    Alice --$5--> David --$5--> Bob

Step 2: Compute NET balance for each person
    Alice: pays $10 + $5, receives $10 = net -$5 (owes $5 overall)
    Bob: pays $10, receives $10 + $5 = net +$5 (is owed $5 overall)
    Charlie: pays $10, receives $10 = net 0 (settled)
    David: pays $5, receives $5 = net 0 (settled)

Step 3: Only non-zero balances matter
    Debtors: Alice (-$5)
    Creditors: Bob (+$5)
    Result: Alice pays Bob $5. ONE transaction.
```

### The Min-Cash-Flow Algorithm

This is equivalent to the **minimum number of transactions** to settle all debts:

```
Algorithm: Greedy Settlement
1. Compute net balance for each person
2. Separate into creditors (positive balance) and debtors (negative balance)
3. Sort both lists by absolute amount (descending)
4. Match largest creditor with largest debtor:
   - Transfer min(credit, |debt|)
   - Reduce both balances
   - Remove anyone who reaches zero
5. Repeat until all balances are zero

Time complexity: O(n log n) for sorting + O(n) for settlement = O(n log n)
Space complexity: O(n) for balance map
```

### Worked Example with Numbers

```
Group: Alice, Bob, Charlie, David, Eve

Expenses:
  - Alice paid $100 dinner, split equally among 5 -> each owes $20
  - Bob paid $50 cab, split equally among 5 -> each owes $10
  - Charlie paid $75 groceries, split among Alice, Charlie, David -> each owes $25

Net balances after all expenses:
  Alice: paid $100, owes ($20 + $10 + $25) = $55. Net = +$45
  Bob: paid $50, owes ($20 + $10) = $30. Net = +$20
  Charlie: paid $75, owes ($20 + $10 + $25) = $55. Net = +$20
  David: paid $0, owes ($20 + $10 + $25) = $55. Net = -$55
  Eve: paid $0, owes ($20 + $10) = $30. Net = -$30

Verification: +45 + 20 + 20 - 55 - 30 = 0  (balanced)

Greedy settlement:
  1. David (-$55) pays Alice (+$45): transfer $45. David now -$10, Alice now 0.
  2. David (-$10) pays Bob (+$20): transfer $10. David now 0, Bob now +$10.
  3. Eve (-$30) pays Bob (+$10): transfer $10. Eve now -$20, Bob now 0.
  4. Eve (-$20) pays Charlie (+$20): transfer $20. Both now 0.

Total: 4 transactions (vs up to 10 pairwise transactions naive)
```

### Optimality Discussion

The greedy approach gives a good solution but is NOT always optimal for minimizing transaction count. The truly optimal solution (minimum number of transactions) is NP-hard in general. However:

- For most real-world groups (< 20 people), greedy is near-optimal
- Splitwise uses a variant that prioritizes settling between pairs who have direct debts
- The max-flow based approach can find the theoretical minimum but is complex to implement

---

## Data Model Design

### Normalized vs Denormalized - Trade-offs

```
+--------------------------------------------------------------+
| Approach      | Query Performance | Write Performance | Space |
+---------------+-------------------+-------------------+-------+
| Fully Normal. | Slow (joins)      | Fast (single row) | Min   |
| Denormalized  | Fast (one read)   | Slow (update many)| More  |
| Hybrid (CQRS) | Fast reads        | Fast writes       | 2x    |
+---------------+-------------------+-------------------+-------+
```

### Recommended: Hybrid Approach

**Write model** (normalized - source of truth):
```
User { id, name, email, phone }
Group { id, name, created_at }
GroupMember { group_id, user_id, joined_at }
Expense { id, group_id, payer_id, amount, description, split_type, created_at }
ExpenseSplit { expense_id, user_id, share_amount }
Settlement { id, group_id, payer_id, payee_id, amount, created_at }
```

**Read model** (denormalized - for fast balance queries):
```
Balance { user_id_1, user_id_2, group_id, net_amount }
  -- Positive means user_1 is owed by user_2
  -- Updated on every expense/settlement via event handler
```

### Why This Split?

- **Write path**: Adding an expense writes to `Expense` + `ExpenseSplit` tables. Simple, atomic, append-only.
- **Read path**: "Show me my balances" reads from pre-computed `Balance` table. O(1) per relationship.
- **Consistency**: Balance table is derived from Expense + Settlement tables. Can be recomputed if corrupted.

---

## Concurrency Concerns

### Problem: Simultaneous Expense Additions

Two users in the same group add expenses at the same instant:
1. User A adds: "Dinner $100, split equally among 4"
2. User B adds: "Drinks $40, split equally among 4"

If both read current balances, compute new balances, and write - one update overwrites the other.

### Solution 1: Group-Level Lock

```cpp
class ExpenseManager {
    map<string, mutex> groupLocks; // one mutex per group
    
    void addExpense(string groupId, Expense exp) {
        lock_guard<mutex> lk(groupLocks[groupId]);
        // Compute splits
        // Update balances atomically
    }
};
```

**Trade-off**: Simple but serializes all operations within a group. For a group of friends adding 1 expense per day, this is fine. For a corporate expense group with 100 daily transactions, it might bottleneck.

### Solution 2: Optimistic Concurrency with Version

```cpp
// Each balance row has a version number
// On update: read version, compute new balance, write with version check
// If version changed (someone else updated), retry

bool updateBalance(userId1, userId2, groupId, delta, expectedVersion) {
    // CAS: UPDATE balance SET amount = amount + delta, version = version + 1
    //       WHERE user1 = ? AND user2 = ? AND version = expectedVersion
    // Returns false if version mismatch -> caller retries
}
```

**Trade-off**: No blocking, higher throughput, but requires retry logic and may fail under high contention.

### Solution 3: Event Sourcing (Best for Audit)

```
All expenses are EVENTS in an append-only log.
Balances are computed by REPLAYING events.
No concurrent modification - only appends.

On balance query: materialize from event log (or read from cache/snapshot)
On expense add: append to log, update materialized view asynchronously
```

**Trade-off**: Perfect audit trail, no concurrency issues on writes, but eventual consistency on balance reads (can be mitigated with synchronous materialization).

---

## Split Strategy Implementation

### Strategy Pattern for Split Types

```cpp
class SplitStrategy {
public:
    virtual vector<Split> computeSplits(
        double totalAmount,
        string payerId,
        vector<string> participants,
        map<string, double> params  // optional: exact amounts or percentages
    ) = 0;
    
    virtual bool validate(double totalAmount, map<string, double> params) = 0;
};

class EqualSplit : public SplitStrategy {
    vector<Split> computeSplits(...) override {
        double share = totalAmount / participants.size();
        // Each non-payer participant owes 'share' to payer
        // Payer's own share cancels out
    }
};

class ExactSplit : public SplitStrategy {
    bool validate(...) override {
        // Sum of exact amounts must equal totalAmount
        double sum = accumulate(params.values());
        return abs(sum - totalAmount) < 0.01; // floating point tolerance
    }
};

class PercentSplit : public SplitStrategy {
    bool validate(...) override {
        // Percentages must sum to 100
        double sum = accumulate(params.values());
        return abs(sum - 100.0) < 0.01;
    }
};
```

### Handling Rounding Errors

When splitting $100 among 3 people: $33.33 + $33.33 + $33.33 = $99.99 (missing $0.01!)

**Solutions**:
1. **Last person absorbs**: First N-1 get floor(amount/N * 100)/100, last person gets remainder
2. **Round-robin cent distribution**: Distribute extra cents one at a time to first few people
3. **Track in smallest unit (cents)**: 10000 cents / 3 = 3333 + 3333 + 3334

Splitwise uses approach #1 in practice.

---

## Notification Design

### When to Notify

| Event | Who to Notify | Priority |
|-------|---------------|----------|
| Expense added | All participants in expense | HIGH |
| Settlement made | Payer and payee | HIGH |
| Member added to group | All existing members | MEDIUM |
| Member left group | All remaining members | MEDIUM |
| Payment reminder | Person who owes | LOW (batched daily) |
| Monthly summary | All users with balances | LOW (scheduled) |

### Notification Architecture

```
ExpenseManager
    |
    | (on expense added)
    v
NotificationService (Observer)
    |
    +-- PushNotificationHandler -> APNs / FCM
    +-- EmailNotificationHandler -> Email service (batched)
    +-- InAppNotificationHandler -> WebSocket push
    +-- SMSNotificationHandler -> SMS gateway (high-priority only)
```

### Why Observer Here?

- ExpenseManager should not know HOW notifications are sent
- Adding a new channel (WhatsApp notification) requires only a new Observer
- Different events can have different subscriber lists
- Easy to add per-user notification preferences (mute group, email-only, etc.)

---

## Group Management Complexity

### Adding a Member Mid-Settlement

Scenario: Group has unsettled debts. New member joins. Should they inherit existing debts?

**Answer**: No. New member starts with zero balance. Only future expenses affect them.

Implementation:
```cpp
void addMember(string groupId, string userId) {
    // 1. Add to group membership
    // 2. Initialize balance with all existing members as 0
    // 3. From this point forward, new expenses include this member
    // 4. Historical expenses are NOT retroactively split
}
```

### Removing a Member with Outstanding Balance

Scenario: Alice owes Bob $50 in a group. Alice leaves the group.

**Options**:
1. **Block removal**: "Settle all debts before leaving" (Splitwise default)
2. **Keep debt alive**: Debt persists outside group context
3. **Group absorbs**: Redistribute Alice's debt among remaining members

**Trade-off**: Option 1 is simplest and most fair. Option 2 requires "orphan debt" tracking. Option 3 punishes remaining members unfairly.

---

## Multi-Currency Handling

### The Problem

Alice pays EUR 100 for dinner in Paris. Bob pays USD 50 for drinks. Group is USD-based. How to compute fair splits?

### Approach 1: Convert at Time of Expense

```
Expense recorded: EUR 100 at rate 1.08 = USD 108
All balances stored in base currency (USD)
Simple but exchange rate at expense time may differ from settlement time
```

### Approach 2: Track Per-Currency Balances

```
Alice's balance: { USD: +45, EUR: +30 }
Bob's balance: { USD: -20, EUR: -30 }

Settlement: Bob can pay Alice 30 EUR + 20 USD
OR convert: Bob pays Alice USD 52.40 (at current rate)
```

### Approach 3: Settlement-Time Conversion (Splitwise Approach)

```
Record expenses in original currency with exchange rate at time of entry.
Display balances in user's preferred currency using current rates.
At settlement time, use live rate for actual transfer.
```

**Why this is practical**: Users care about "how much do I owe in MY currency" not historical rates. The slight unfairness from rate fluctuation is accepted as a cost of simplicity.

---

## Settlement Strategies

### Direct Settlement
```
Alice owes Bob $50 -> Alice pays Bob $50 directly (Venmo, cash, bank transfer)
Simple but requires N*(N-1)/2 potential transactions in worst case
```

### Simplified Settlement (Min-Transactions)
```
Compute net balances -> minimize transactions using greedy algorithm
Reduces N*(N-1)/2 potential transactions to at most N-1 actual transactions
```

### Through-Platform Settlement
```
All payments route through the app's payment system
Advantages: tracking, confirmation, automation
Disadvantages: requires payment integration, regulatory compliance, fees
```

### Periodic Auto-Settlement
```
At end of each month (or when balance exceeds threshold):
  - Compute simplified debts
  - Generate settlement requests
  - Users confirm and pay

Reduces cognitive overhead of tracking individual expenses
```

---

## Data Integrity and Invariants

### Critical Invariant: Zero-Sum

For any group, at any point in time:
```
SUM(all balances in group) = 0
```

This is the fundamental correctness check. If it ever violates, data is corrupted.

### How to Maintain

1. Every expense CREATES equal positive and negative balance changes
2. Every settlement MOVES money between two people (net zero)
3. No operation can create or destroy money

### Verification

```cpp
bool verifyGroupIntegrity(string groupId) {
    double sum = 0;
    for (auto& [userId, balance] : getGroupBalances(groupId)) {
        sum += balance;
    }
    return abs(sum) < 0.001; // floating point tolerance
}
```

Run this as a background job periodically. Alert if violated.

---

## Scalability Considerations

### Sharding Strategy

| Shard Key | Pros | Cons |
|-----------|------|------|
| `group_id` | All group ops local | Hot groups (large corporate) |
| `user_id` | User queries fast | Cross-shard for group operations |
| Composite `group_id + user_id` | Balanced | Complex routing |

**Recommended**: Shard by `group_id` because most operations (add expense, view balances, simplify debts) are group-scoped. Accept that large groups may be hot partitions and handle with dedicated resources.

### Read Optimization

```
Balance queries are the most frequent operation (every app open).
Pre-compute and cache:
  - Per-user total owed/owing across all groups
  - Per-group balance summary
  - Simplified settlement suggestions

Cache invalidation: event-driven (on expense/settlement, update relevant caches)
```

---

## Key Classes Summary

```
User { id, name, email }
Group { id, name, members[], created_at }
Expense { id, group_id, payer_id, amount, currency, description, splits[], created_at }
Split { expense_id, user_id, share_amount }
Balance { user_id_1, user_id_2, group_id, net_amount }
Settlement { id, group_id, from_id, to_id, amount, status, created_at }

SplitStrategy (interface)
  - EqualSplit
  - ExactSplit
  - PercentSplit

ExpenseManager (Singleton)
  - addExpense(groupId, payerId, amount, splitType, participants)
  - getBalance(userId) -> map<otherUser, netAmount>
  - simplifyDebts(groupId) -> vector<Settlement>
  - settleUp(fromId, toId, amount)

NotificationService (Observer)
  - notify(Event event)
  - subscribe(userId, eventType, channel)
```

## Files
- [splitwise.cpp](splitwise.cpp)

## Interview Questions
1. Data structure for balances - why is `map<user, map<user, amount>>` good? (O(1) lookup per pair)
2. How to simplify debts? (Compute net balances, greedy match largest creditor/debtor)
3. Prove the zero-sum invariant holds for all operations.
4. How to handle currency conversion? (Convert at expense time, display at current rate)
5. Concurrent expense additions - lock granularity? (Per-group lock or optimistic versioning)
6. How to scale to millions of users? (Shard by group_id, cache balance summaries)
7. How to support partial payments / installments? (Settlement with partial flag, track remaining)
8. Why is minimum-transaction settlement NP-hard in general? (Reduction from subset-sum)
9. How would you handle expense disputes? (Add DISPUTED status, require resolution before settlement)
10. How to implement recurring expenses? (Cron job generates expense at interval, uses same split logic)

## Daily Assignment
1. Implement the greedy debt simplification algorithm with the worked example above.
2. Add multi-currency expense support with conversion at expense time.
3. Persist to JSON; reload state on restart and verify zero-sum invariant.
4. Add expense categories + monthly summary report per user.
5. Implement the percentage split with rounding error handling.
