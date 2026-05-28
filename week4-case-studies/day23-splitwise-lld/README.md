# Day 23 - LLD: Splitwise

## Problem
Design an expense-sharing app. Users create groups, log expenses, split among participants (equally, by exact amount, by percentage). System maintains balance sheet showing who owes whom.

## Requirements
1. Create users and groups
2. Add expense with split type: EQUAL, EXACT, PERCENT
3. Show balance for a user
4. Settle up - transfer money to clear debt
5. Simplify debts (graph optimization)

## Patterns Used
| Pattern | Where |
|---------|-------|
| Strategy | Split logic (Equal/Exact/Percent) |
| Factory | Split factory |
| Observer | Notify users on balance change |
| Singleton | ExpenseManager |

## Files
- [splitwise.cpp](splitwise.cpp)

## Key Classes
```
User { id, name, email }
Expense { id, payer, amount, splits[] }
SplitStrategy (interface)
  - EqualSplit, ExactSplit, PercentSplit
ExpenseManager
  - addExpense(...)
  - balance(userId) -> map<otherUser, owed>
  - settleUp(from, to, amount)
```

## Interview Questions
1. Data structure for balances - why is `map<user, map<user, amount>>` good?
2. How to simplify debts? (Greedy: max-creditor pays max-debtor)
3. How to handle currency conversion?
4. Concurrent expense additions - lock granularity?
5. How to scale to millions of users? (Shard by group_id)
6. How to support partial payments / installments?

## Daily Assignment
1. Implement balance simplification: minimize transactions to settle a group.
2. Add currency conversion (multi-currency expense).
3. Persist to JSON; reload state on restart.
4. Add expense categories + monthly summary report.
