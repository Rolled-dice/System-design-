# Day 13 - Iterator + Template Method + Chain of Responsibility

## Iterator
**Intent**: Provide a way to access elements of an aggregate sequentially without exposing its underlying representation.

C++ STL is built entirely on Iterator. You'll mostly use STL iterators - implement custom only for custom containers.

## Template Method
**Intent**: Define the skeleton of an algorithm in a base class, deferring some steps to subclasses.

**Use cases**: Build pipelines, test setup/teardown, frameworks (`onCreate`, `onResume`).

## Chain of Responsibility
**Intent**: Pass a request along a chain of handlers; each decides to handle it or pass on.

**Use cases**: Middleware (Express.js, ASP.NET), event bubbling, approval workflows, log levels.

## Files
- [iterator_custom.cpp](iterator_custom.cpp)
- [template_method.cpp](template_method.cpp) - report generator
- [chain_middleware.cpp](chain_middleware.cpp) - HTTP middleware

## Interview Questions
1. Iterator vs Visitor.
2. External vs internal iterator.
3. Template Method - how does it enforce DIP via the Hollywood Principle ("don't call us, we'll call you")?
4. CoR vs Decorator - both chain.
5. CoR - how to ensure a request is always handled?

## Daily Assignment
1. Iterator: build a custom `BSTIterator` for in-order traversal.
2. Template Method: build `DataProcessor` with `read -> process -> write` template; subclass for CSV and JSON.
3. CoR: build approval chain `Manager -> Director -> CEO` with monetary limits.
