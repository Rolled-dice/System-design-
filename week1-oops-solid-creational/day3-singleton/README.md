# Day 3 - Singleton Pattern

## Intent
Ensure a class has only one instance and provide a global access point.

## When to Use
- Logger, Configuration manager, Connection pool, Cache manager
- Hardware interface (printer spooler)

## When NOT to Use
- Hides dependencies, hard to unit test, global state
- Prefer Dependency Injection where possible

## Variants
1. **Eager** - instance created at static initialization
2. **Lazy** - created on first call
3. **Thread-safe** (Meyer's Singleton - C++11 guarantees thread-safe local static)
4. **Double-checked locking** (legacy)

## Files
- [singleton_basic.cpp](singleton_basic.cpp)
- [singleton_meyers.cpp](singleton_meyers.cpp) - **preferred in modern C++**
- [singleton_threadsafe.cpp](singleton_threadsafe.cpp) - double-checked locking
- [logger_singleton.cpp](logger_singleton.cpp) - real-world

## Interview Questions
1. Why is Meyer's Singleton thread-safe in C++11+?
2. Problems with Singleton (testability, lifetime, global state).
3. How does double-checked locking work pre-C++11? Why is it broken without `atomic`?
4. Singleton vs Static class - differences.
5. How to destroy a Singleton deterministically?
6. Can a Singleton be subclassed?
7. Singleton in multi-DLL/shared library - why does it break?

## Daily Assignment
1. Implement a thread-safe `ConfigManager` singleton that loads settings from a file once.
2. Refactor it to use Dependency Injection instead - compare testability.
3. Build a `ConnectionPool` singleton with `getConnection()` / `release()`.
