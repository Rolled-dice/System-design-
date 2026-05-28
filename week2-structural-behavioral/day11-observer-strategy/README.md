# Day 11 - Observer + Strategy

## Observer
**Intent**: Define a one-to-many dependency so when one object changes state, all dependents are notified automatically.

**Use cases**: Event systems, MVC, pub-sub, stock tickers, GUI listeners.

## Strategy
**Intent**: Define a family of algorithms, encapsulate each, make them interchangeable at runtime.

**Use cases**: Sorting algorithms, payment processing, compression, routing.

## Files
- [observer.cpp](observer.cpp) - Stock price observers
- [strategy.cpp](strategy.cpp) - Payment strategies
- [strategy_sort.cpp](strategy_sort.cpp) - Pluggable sort

## Interview Questions
1. Observer vs Pub-Sub - what's different? (Hint: broker)
2. Push vs Pull observer model.
3. How to avoid memory leaks with observers in C++? (`weak_ptr`)
4. Strategy vs State - both polymorphic. What differs?
5. Strategy + Factory combo - common interview question.

## Daily Assignment
1. Observer: build a `WeatherStation` notifying `PhoneDisplay`, `WebDisplay`, `LogDisplay`.
2. Strategy: payment system with `CreditCard`, `UPI`, `Wallet`, `NetBanking`.
3. Strategy: implement compression with Zip/Gzip/Bzip2 strategies.
