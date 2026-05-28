# Day 3 - Singleton Pattern

## Overview

The Singleton pattern is simultaneously one of the simplest design patterns to understand and one of the most controversial to use. It addresses a real need (ensuring exactly one instance of something exists) but brings significant costs (global state, hidden dependencies, testing difficulties). Understanding both sides deeply is essential for making informed design decisions.

---

## GoF Documentation

### Intent

Ensure a class has only one instance and provide a global point of access to it.

### Motivation

Some system components are inherently singular. A system has one file system, one window manager, one print spooler. Multiple instances of these would cause conflicts (two print spoolers fighting over the same printer, two window managers drawing overlapping frames). The pattern ensures that:
1. A class has exactly one instance
2. That instance is globally accessible
3. The class itself controls its instantiation (not the clients)

The naive approach of using a global variable provides global access but does not prevent multiple instantiation. Singleton encapsulates both concerns in the class itself.

### Applicability

Use Singleton when:
- There must be exactly one instance of a class, and it must be accessible to clients from a well-known access point
- The sole instance should be extensible by subclassing, and clients should be able to use an extended instance without modifying their code
- The instance's creation is expensive and you want lazy initialization (created only when first needed)

**Decision Checklist:**
1. Is there truly only one valid instance in the entire process? (Not "only one right now")
2. Is global access genuinely needed, or would passing the instance as a parameter work?
3. Will you need to test code that depends on this class? (If yes, DI is probably better)
4. Is the class stateless or nearly so? (Stateless singletons have fewer problems)

### Structure

```
+-----------------------------------+
|           Singleton               |
+-----------------------------------+
| - static instance: Singleton*     |
| - (private data members)          |
+-----------------------------------+
| - Singleton()            [private]|
| - Singleton(const&)     = delete  |
| - operator=(const&)     = delete  |
+-----------------------------------+
| + static getInstance(): Singleton&|
| + (public operations)             |
+-----------------------------------+

  Client ---uses---> Singleton::getInstance()
                         |
                         v
              [Creates instance on first call]
              [Returns same instance thereafter]
```

### Participants

- **Singleton** - defines a static `getInstance()` operation that lets clients access its unique instance. May be responsible for creating its own unique instance.
- **Client** - accesses the Singleton solely through its `getInstance()` operation.

### Collaborations

Clients access a Singleton instance solely through Singleton's `getInstance()` operation. There is no public constructor; the class controls its own lifecycle.

### Consequences

**Benefits:**
1. Controlled access to sole instance - encapsulates the instance, giving you complete control over how and when clients access it
2. Reduced name space - avoids polluting the global namespace with free variables
3. Permits refinement of operations and representation - the Singleton class can be subclassed, and an application can be configured with an instance of the extended class at runtime
4. Permits a variable number of instances - the pattern makes it easy to change your mind and allow more than one instance (just modify `getInstance()`)
5. More flexible than class operations (static methods) - you can use polymorphism with Singleton, which you cannot with static methods

**Costs:**
1. Introduces global state - any code anywhere can access and modify the Singleton, making data flow opaque
2. Hides dependencies - a class using a Singleton does not declare it in its interface, making dependencies invisible to callers
3. Complicates testing - you cannot easily replace the Singleton with a mock for unit testing
4. Thread safety complexity - ensuring thread-safe initialization adds complexity
5. Lifetime management - when should the Singleton be destroyed? In what order relative to other singletons?
6. Violates SRP - the class manages its own lifecycle in addition to its actual responsibility

### Implementation (C++ Specific)

#### Meyers Singleton (Preferred in Modern C++)

```cpp
class Singleton {
public:
    static Singleton& getInstance() {
        static Singleton instance;  // Thread-safe in C++11+
        return instance;
    }
    
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    
private:
    Singleton() = default;
    ~Singleton() = default;
};
```

**Why this works:** The C++11 standard (Section 6.7) guarantees that local static variables are initialized exactly once, even when multiple threads call the function simultaneously. The compiler generates the necessary synchronization. This is the simplest and most correct approach in modern C++.

#### Double-Checked Locking Pattern (DCLP) - Historical

```cpp
// WARNING: Broken without proper memory ordering!
class Singleton {
    static std::atomic<Singleton*> instance;
    static std::mutex mtx;
public:
    static Singleton* getInstance() {
        Singleton* tmp = instance.load(std::memory_order_acquire);
        if (tmp == nullptr) {
            std::lock_guard<std::mutex> lock(mtx);
            tmp = instance.load(std::memory_order_relaxed);
            if (tmp == nullptr) {
                tmp = new Singleton();
                instance.store(tmp, std::memory_order_release);
            }
        }
        return tmp;
    }
};
```

**Memory Model Issues:** Before C++11, DCLP was notoriously broken. The problem: without memory fences, the compiler or CPU might reorder operations such that the instance pointer is published before the object is fully constructed. Another thread sees a non-null pointer but accesses an uninitialized object. This was a famous concurrency bug that motivated the C++11 memory model. With `std::atomic` and proper memory ordering, DCLP works correctly but is unnecessarily complex compared to Meyers Singleton.

#### Eager Initialization

```cpp
class Singleton {
    static Singleton instance;  // Created at program start
public:
    static Singleton& getInstance() { return instance; }
    // ...
};
Singleton Singleton::instance;  // Definition in .cpp
```

**Tradeoff:** No thread-safety concerns (initialized before `main()`), but no lazy initialization and susceptible to the "static initialization order fiasco" across translation units.

### Known Uses

- **Logging frameworks** (log4j, spdlog) - one logger per category
- **Spring Framework** - beans are singletons by default in the application context
- **Database connection pools** - one pool per datasource
- **Hardware managers** - print spooler, GPU device context
- **Configuration managers** - load once, access everywhere
- **Thread pools** - typically one pool for the application

### Related Patterns

- **Abstract Factory** - often implemented as a Singleton (one factory instance)
- **Builder** - can be a Singleton if you reuse the same builder
- **Prototype** - alternative to Singleton when you need configured instances (clone a prototype instead of accessing a global)
- **Monostate** - alternative where all instances share state (via static members) but multiple instances can exist

---

## The Anti-Pattern Debate

### Why Many Consider Singleton Harmful

The software engineering community has increasingly viewed Singleton as an anti-pattern. Here is why:

**1. Hidden Dependencies**
When a class calls `Singleton::getInstance()` internally, that dependency is invisible in the class's interface. You cannot know what a class depends on without reading its implementation. This makes systems harder to reason about.

**2. Testing Nightmare**
How do you unit test a class that internally calls `DatabaseConnection::getInstance()`? You cannot inject a mock. Solutions exist (template the Singleton type, use interfaces) but they add complexity that defeats the simplicity Singleton is supposed to provide.

**3. God Object Tendency**
Singletons tend to accumulate responsibilities because "well, it is already globally accessible, let me add one more thing." Over years, the Config Singleton becomes a God object knowing everything about the application.

**4. Concurrency Hazard**
A globally accessible, mutable object in a multi-threaded program is a recipe for race conditions. Every access needs synchronization consideration.

### When Singleton IS Appropriate

Despite the criticism, Singleton remains appropriate for:
- **Truly global, stateless or read-only resources:** A configuration loaded at startup and never modified
- **Hardware abstractions:** There is genuinely one GPU, one network interface
- **Logging:** The logging infrastructure is a cross-cutting concern that does not fit naturally into any dependency hierarchy
- **Legacy integration:** When working with C libraries or system APIs that inherently have global state

### The Modern Alternative: Dependency Injection

Instead of:
```cpp
void processOrder() {
    auto& db = Database::getInstance();  // Hidden dependency
    auto& logger = Logger::getInstance();  // Hidden dependency
}
```

Prefer:
```cpp
void processOrder(Database& db, Logger& logger) {  // Explicit dependencies
    // Now testable with mocks, clear about what it needs
}
```

---

## Common Misconceptions

1. **"Singleton = global variable"** - Not exactly. A global variable does not control instantiation or guarantee uniqueness. Singleton encapsulates both. However, the effect on code quality is similar.

2. **"Meyers Singleton has no overhead"** - The first call has overhead: the compiler generates a thread-safe check (typically a compare-and-branch on an atomic flag). Subsequent calls are near-zero overhead.

3. **"Singleton is always thread-safe"** - Only Meyers Singleton (C++11+) is automatically thread-safe for initialization. The Singleton's METHODS still need explicit synchronization if they modify shared state.

4. **"You can subclass Singleton easily"** - You technically can, but it is awkward. Which subclass gets instantiated? This usually requires a registry or configuration mechanism.

5. **"Singleton prevents multiple instances"** - In C++, determined code can bypass it (placement new, memcpy, serialization/deserialization). Singleton is a convention enforced by the class interface, not a hardware guarantee.

6. **"Static methods are the same as Singleton"** - Static methods cannot be polymorphic, cannot implement interfaces, cannot be passed around as objects, and cannot have their implementation swapped at runtime.

---

## Singleton Destruction and Lifetime Management

### The Problem

When does a Singleton get destroyed? For Meyers Singleton (static local), destruction happens during static destruction at program exit, in reverse order of construction. This creates issues:

1. **Order of destruction:** If Singleton A uses Singleton B, and B is destroyed first, A's destructor crashes trying to use a dead B. This is the "dead reference" problem.

2. **Phoenix Singleton:** One solution is to detect the dead state and re-create the Singleton if accessed after destruction. The Singleton "rises from the ashes." This is complex and rarely worth the trouble.

3. **Longevity-based destruction:** Andrei Alexandrescu (Modern C++ Design) proposed assigning longevity values to Singletons to control destruction order. Singletons with higher longevity are destroyed last.

### Practical Guidance

- Prefer Singletons that do not depend on other Singletons during destruction
- If destruction order matters, use `std::atexit()` with registered cleanup functions in the correct order
- For long-running services that do not shut down gracefully (servers killed by SIGTERM), leaking Singletons is acceptable - the OS reclaims all memory anyway
- For libraries that might be loaded/unloaded, explicit `init()` / `shutdown()` functions are safer than Singleton

---

## Singleton in Multi-DLL/Shared Library Environments

A subtle C++ problem: if a Singleton header is included in multiple DLLs, each DLL gets its own copy of the static local variable. You end up with multiple "singletons." Solutions:
- Export the Singleton from one specific DLL
- Use `__declspec(dllexport/dllimport)` on Windows
- On Linux, use default visibility and link against the shared library

This is a real-world issue that catches teams migrating monolithic applications to plugin architectures.

---

## Why Does This Matter in System Design?

In High-Level Design (HLD), Singleton appears as:
- **Configuration Service** - one service holding feature flags and settings
- **Service Registry** (Consul, Eureka) - one registry per cluster
- **Load Balancer** - one balancer fronting a service group

Understanding Singleton's tradeoffs helps you evaluate:
- When to use a centralized vs distributed configuration
- When global state (Redis cache) vs local state (in-memory cache) is appropriate
- Why service meshes avoid single points of failure (anti-Singleton at the architecture level)

---

## Files

- [singleton_basic.cpp](singleton_basic.cpp)
- [singleton_meyers.cpp](singleton_meyers.cpp) - **preferred in modern C++**
- [singleton_threadsafe.cpp](singleton_threadsafe.cpp) - double-checked locking
- [logger_singleton.cpp](logger_singleton.cpp) - real-world

---

## Interview Questions

1. Why is Meyer's Singleton thread-safe in C++11+?
2. Problems with Singleton (testability, lifetime, global state).
3. How does double-checked locking work pre-C++11? Why is it broken without `atomic`?
4. Singleton vs Static class - differences.
5. How to destroy a Singleton deterministically?
6. Can a Singleton be subclassed?
7. Singleton in multi-DLL/shared library - why does it break?

**Advanced**
8. Explain the static initialization order fiasco and how it relates to Singleton.
9. How would you make a Singleton testable without changing its interface?
10. Design a Singleton that supports graceful shutdown (releasing resources in order).
11. What is the Monostate pattern and when would you use it instead of Singleton?
12. How does the Service Locator pattern compare to Singleton? Is it better or worse?

---

## Daily Assignment

1. Implement a thread-safe `ConfigManager` singleton that loads settings from a file once.
2. Refactor it to use Dependency Injection instead - compare testability.
3. Build a `ConnectionPool` singleton with `getConnection()` / `release()`.
4. Implement a Singleton that can be reset for testing purposes (discuss why this is a code smell but sometimes necessary).
5. Create a `Logger` singleton that supports multiple output targets (console, file) and is safe to call from multiple threads.
