# Day 13 - Iterator + Template Method + Chain of Responsibility

## Table of Contents
- [Iterator Pattern](#iterator-pattern)
- [Template Method Pattern](#template-method-pattern)
- [Chain of Responsibility Pattern](#chain-of-responsibility-pattern)
- [Pattern Comparison Matrix](#pattern-comparison-matrix)
- [Code Examples](#code-examples)
- [Interview Questions](#interview-questions)
- [Daily Assignment](#daily-assignment)

---

## Iterator Pattern

### Intent (GoF)
Provide a way to access the elements of an aggregate object sequentially without exposing its underlying representation.

### Also Known As
- Cursor

### Real-World Analogy
Think of a TV remote with channel up/down buttons. You do not need to know how channels are stored internally (array, linked list, satellite feed, cable tuner). You just press "next" and "previous" to traverse them sequentially. The remote is an iterator over the channel collection. Different TV brands have different internal channel storage, but the remote interface is the same.

Another analogy: a Spotify playlist. You press "next track" without knowing whether the playlist is stored locally, streamed from a server, or generated algorithmically. The playback cursor (iterator) abstracts the underlying data source.

### Motivation
Consider a `List` aggregate that can be represented internally as an array, linked list, or tree. Client code that needs to traverse the elements should not depend on the internal structure. If it does, changing the representation breaks all clients.

The Iterator pattern extracts traversal behavior into a separate object. Each aggregate provides a factory method (`createIterator()`) that returns an appropriate iterator for its representation. All iterators share the same interface (`hasNext()`, `next()`, `current()`).

### Applicability
Use the Iterator pattern when:
- You want to access an aggregate's contents without exposing its internal representation
- You want to support multiple traversals of aggregate objects (forward, backward, filtered)
- You want to provide a uniform interface for traversing different aggregate structures (polymorphic iteration)
- You need multiple simultaneous traversals on the same aggregate

### Structure

```
  +-------------------+              +-------------------+
  |    Aggregate      |              |     Iterator      |
  +-------------------+  creates     +-------------------+
  | +createIterator() |------------->| + first()         |
  +-------------------+              | + next()          |
         ^                           | + isDone()        |
         |                           | + currentItem()   |
  +-------------------+              +-------------------+
  | ConcreteAggregate |                     ^
  +-------------------+                     |
  | - items[]         |              +-------------------+
  | +createIterator() |              | ConcreteIterator  |
  |  { return new     |              +-------------------+
  |    ConcreteIter   |              | - aggregate       |
  |    (this); }      |              | - currentIndex    |
  +-------------------+              | + first()         |
                                     | + next()          |
                                     | + isDone()        |
                                     | + currentItem()   |
                                     +-------------------+
```

### Internal vs External Iterators

| Aspect | External Iterator | Internal Iterator |
|--------|------------------|-------------------|
| **Control** | Client controls iteration (calls `next()`) | Iterator controls (takes a callback/functor) |
| **Flexibility** | Can stop, skip, compare two iterators | Must process all elements (or use exceptions) |
| **Example** | `while (it.hasNext()) { process(it.next()); }` | `collection.forEach([](auto& item) { ... });` |
| **C++ style** | STL iterators (`begin()`, `end()`, `++it`) | `std::for_each`, range-based for with lambdas |
| **Multiple iterators** | Easy (each is separate object) | Harder (nested internal iterations are awkward) |
| **Parallelism** | Sequential by default | Can parallelize internally |

**External (C++ STL style):**
```cpp
for (auto it = vec.begin(); it != vec.end(); ++it) {
    if (*it > threshold) break;  // Easy to stop early
}
```

**Internal (functional style):**
```cpp
std::for_each(vec.begin(), vec.end(), [](int x) {
    process(x);  // Cannot easily break out
});
```

### C++ STL Iterator Categories

The STL defines a hierarchy of iterator categories with increasing capabilities:

```
  +------------------+
  | Input Iterator   |  (read once, forward only: istream_iterator)
  +------------------+
         ^
         |
  +------------------+
  | Forward Iterator |  (read/write, forward only: forward_list::iterator)
  +------------------+
         ^
         |
  +------------------+
  | Bidirectional    |  (forward + backward: list::iterator, set::iterator)
  | Iterator         |
  +------------------+
         ^
         |
  +------------------+
  | Random Access    |  (bidirectional + arithmetic: vector::iterator, deque::iterator)
  | Iterator         |
  +------------------+
         ^
         |
  +------------------+
  | Contiguous       |  (random access + contiguous memory: C++20, vector, array)
  | Iterator         |
  +------------------+
```

| Category | Operations | Example |
|----------|-----------|---------|
| Input | `++`, `*`, `==`, `!=` | `istream_iterator` |
| Output | `++`, `*` (write only) | `ostream_iterator`, `back_inserter` |
| Forward | Input + multi-pass guarantee | `forward_list::iterator` |
| Bidirectional | Forward + `--` | `list::iterator`, `map::iterator` |
| Random Access | Bidirectional + `+n`, `-n`, `[]`, `<` | `vector::iterator` |
| Contiguous (C++20) | Random Access + elements in contiguous memory | `vector::iterator`, `array::iterator` |

### Robustness: Modification During Iteration
A critical implementation concern: what happens when the aggregate is modified while being iterated?

**Strategies:**
1. **Copy-on-iterate**: Iterator works on a snapshot (safe but memory-expensive)
2. **Fail-fast**: Detect modification, throw exception (Java's `ConcurrentModificationException`)
3. **Robust iterators**: Register with aggregate, get notified of changes, adjust position
4. **Immutable collections**: Prevent modification entirely during iteration

In C++, modifying a container during iteration with STL iterators is **undefined behavior** for most operations (inserting into a vector invalidates all iterators). Use erase-remove idiom or iterate over a copy.

### Consequences

**Benefits:**
1. Supports variations in traversal (forward, reverse, filtered, depth-first, breadth-first)
2. Simplifies the Aggregate interface (traversal logic moved out)
3. Multiple traversals can be in progress simultaneously
4. Polymorphic iteration across different container types

**Liabilities:**
1. Additional classes (iterator per aggregate type) - though STL templates solve this elegantly
2. Iterator can become invalid if aggregate is modified
3. Performance overhead for simple containers (raw pointer would be faster than iterator object)

### Implementation Notes (C++ Specific)
- Implement `begin()` and `end()` methods returning iterators for range-based for support
- Define `iterator_traits` specializations for custom iterators
- Use `std::iterator_traits<It>::value_type` etc. for generic algorithm compatibility
- For C++20: implement `std::input_or_output_iterator` concept
- Consider `std::ranges::view` for lazy evaluation (C++20 ranges library)
- Use `operator++` (pre and post), `operator*`, `operator==` as minimum for forward iterator

### Known Uses
- **C++ STL**: The entire STL is built around iterators (algorithms work on iterator pairs)
- **Database Cursors**: `ResultSet` in JDBC, cursor in MongoDB/SQL
- **File System Traversal**: `std::filesystem::directory_iterator`, `recursive_directory_iterator`
- **Python**: `__iter__` and `__next__` protocol
- **Java**: `Iterator<T>` interface with `hasNext()`, `next()`, `remove()`
- **C++20 Ranges**: `std::views::filter`, `std::views::transform` create lazy iterator chains

### Related Patterns
- **Composite**: iterators are often used to traverse Composite structures
- **Factory Method**: polymorphic iterators rely on factory methods (`createIterator()`)
- **Memento**: iterator can use memento to capture iteration state for bookmarking
- **Visitor**: both traverse structures; Iterator sequentially, Visitor applies operations

---

## Template Method Pattern

### Intent (GoF)
Define the skeleton of an algorithm in an operation, deferring some steps to subclasses. Template Method lets subclasses redefine certain steps of an algorithm without changing the algorithm's structure.

### Real-World Analogy
Think of building a house. The overall process (template) is fixed: lay foundation, build walls, install roof, install windows, furnish interior. Every house follows this sequence. But the specific implementation of each step varies: wooden walls vs brick walls, tile roof vs metal roof, etc. The construction company defines the process (template method); subcontractors implement specific steps.

Another analogy: a recipe template. "Making soup" always follows: boil water, add base ingredients, simmer, add seasonings, serve. But the specific ingredients change based on what kind of soup you are making.

### Motivation: The Hollywood Principle
"Don't call us, we'll call you."

In traditional programming, your code calls library/framework code. With Template Method, the framework calls YOUR code. You override specific steps, and the framework decides when to call them.

```
  Traditional:                    Template Method (Hollywood Principle):
  
  Your Code                       Framework (Template)
      |                                |
      +---> Library.doA()              +---> calls YOUR step1()
      |                                |
      +---> Library.doB()              +---> calls YOUR step2()
      |                                |
      +---> Library.doC()              +---> calls YOUR step3()
  
  YOU control flow                 FRAMEWORK controls flow
```

This is **Inversion of Control (IoC)** - the foundation of all frameworks (Qt, JUnit, game engines).

### Applicability
Use the Template Method pattern when:
- You want to implement the invariant parts of an algorithm once and leave variable parts to subclasses
- Common behavior among subclasses should be factored into a common class to avoid code duplication
- You want to control extensions - subclasses can extend specific steps but not change the overall structure
- You need "hook" methods - optional steps that subclasses MAY override but are not required to

### Structure

```
  +-------------------------------+
  |       AbstractClass           |
  +-------------------------------+
  | + templateMethod()            |  <- final (non-virtual), defines skeleton
  |   {                           |
  |     step1();                  |
  |     step2();                  |
  |     if (hook()) {             |
  |       step3();                |
  |     }                         |
  |   }                           |
  | # step1()  = 0                |  <- pure virtual (MUST override)
  | # step2()  = 0                |  <- pure virtual (MUST override)
  | # step3()  = 0                |  <- pure virtual (MUST override)
  | # hook() { return true; }     |  <- virtual with default (MAY override)
  +-------------------------------+
          ^              ^
          |              |
  +-------+------+  +---+----------+
  | ConcreteClassA|  |ConcreteClassB|
  +---------------+  +--------------+
  | # step1() { A1 }| | # step1() { B1 }|
  | # step2() { A2 }| | # step2() { B2 }|
  | # step3() { A3 }| | # step3() { B3 }|
  +---------------+  +--------------+
```

### Participants
- **AbstractClass** - defines abstract primitive operations that subclasses override; implements the template method defining the algorithm skeleton; may define hook methods with default behavior
- **ConcreteClass** - implements the primitive operations to carry out subclass-specific steps

### Hook Methods
Hooks are methods with a default (often empty) implementation that subclasses MAY override:

```cpp
class DataProcessor {
public:
    // Template method (final - cannot be overridden)
    void process() {
        openFile();
        readData();
        if (shouldValidate()) {  // HOOK - optional step
            validate();
        }
        transform();
        writeOutput();
        cleanup();               // HOOK - default does nothing
    }
    
protected:
    virtual void openFile() = 0;     // Must override
    virtual void readData() = 0;     // Must override
    virtual void transform() = 0;    // Must override
    virtual void writeOutput() = 0;  // Must override
    
    virtual bool shouldValidate() { return true; }  // Hook - may override
    virtual void validate() {}       // Hook - may override
    virtual void cleanup() {}        // Hook - may override
};
```

### Consequences

**Benefits:**
1. Code reuse - common algorithm structure written once in base class
2. Inversion of Control - framework calls your code at the right time
3. Easy to enforce algorithm invariants (template method is non-virtual/final)
4. Hooks provide flexible extension points without forcing overrides

**Liabilities:**
1. Subclasses are tightly coupled to base class (fragile base class problem)
2. Hard to understand the flow (must read base class to understand what methods are called when)
3. Suppressing steps is awkward (must override with empty body or check flags)
4. Inheritance-based: limits to single inheritance in many languages (C++ allows MI but it is complex)

### Template Method vs Strategy

| Aspect | Template Method | Strategy |
|--------|----------------|----------|
| **Mechanism** | Inheritance (IS-A) | Composition (HAS-A) |
| **Granularity** | Varies some steps of algorithm | Varies entire algorithm |
| **Coupling** | Tight (subclass knows base) | Loose (strategy is independent) |
| **When to use** | Algorithm skeleton is fixed; steps vary | Entire algorithm interchangeable |
| **Runtime switch** | No (fixed at compile time) | Yes (swap strategy at runtime) |
| **Number of override points** | Multiple fine-grained steps | Usually one coarse-grained operation |

### Implementation Notes (C++ Specific)
- Make the template method `final` (C++11) to prevent subclasses from overriding the skeleton
- Use `private` virtual functions (NVI - Non-Virtual Interface idiom) for steps
- Mark pure virtual steps with `= 0`; hooks get default implementations
- Consider CRTP for static template method (compile-time polymorphism)
- Destructor must be virtual in the base class

### Known Uses
- **Testing Frameworks**: JUnit/Google Test `setUp()` -> `testMethod()` -> `tearDown()`
- **Application Frameworks**: Qt `QApplication::exec()` calls your event handlers
- **Game Loops**: `initialize()` -> `update()` -> `render()` -> `cleanup()` per frame
- **Servlet Lifecycle**: `init()` -> `service()` -> `destroy()`
- **Document Processing**: `open()` -> `parse()` -> `transform()` -> `save()`
- **STL Algorithms**: `std::sort` with custom comparator is template method via templates

### Common Misconceptions
1. **"Template Method uses C++ templates"** - No! The name "template" refers to the algorithm skeleton, not the C++ language feature. Though templates CAN implement the pattern (CRTP).
2. **"Template Method and Strategy do the same thing"** - Template Method varies STEPS of an algorithm via inheritance. Strategy varies the ENTIRE algorithm via composition.
3. **"All virtual methods in a base class constitute Template Method"** - Only when there is a non-virtual method that defines the calling order of those virtual methods.

### Related Patterns
- **Strategy**: composition alternative to Template Method's inheritance-based approach
- **Factory Method**: often called by template methods (a specialization of Template Method)
- **Hook Method**: not a separate pattern but a technique within Template Method

---

## Chain of Responsibility Pattern

### Intent (GoF)
Avoid coupling the sender of a request to its receiver by giving more than one object a chance to handle the request. Chain the receiving objects and pass the request along the chain until an object handles it.

### Also Known As
- Chain of Command
- Middleware Pipeline

### Real-World Analogy
Think of a corporate approval process. An employee submits an expense report. If the amount is under $100, the team lead can approve it. Under $1000, the department manager handles it. Under $10,000, the VP. Above that, the CEO. The employee does not need to know who specifically will approve their request; they just submit it and it flows up the chain until someone with sufficient authority handles it.

Another analogy: DOM event bubbling in web browsers. When you click a button, the click event first goes to the button. If unhandled, it bubbles to the parent div, then to the body, then to the document. Each element in the hierarchy gets a chance to handle the event.

### Motivation
Consider an online help system. When a user presses F1, the system should show context-sensitive help. If the current UI widget has specific help, show that. Otherwise, check its parent container. If no container-level help exists, show the general application help. The request for help propagates up the containment hierarchy until something handles it.

### Applicability
Use Chain of Responsibility when:
- More than one object may handle a request, and the handler is not known a priori
- You want to issue a request without specifying the receiver explicitly
- The set of handlers should be configurable dynamically
- Requests should be handled by different handlers based on runtime conditions

### Structure

```
  +----------+         +-------------------+
  |  Client  |-------->|     Handler       |
  +----------+         +-------------------+
                       | - successor: Handler*
                       | + handleRequest()  |
                       |   { if canHandle() |
                       |       // handle it |
                       |     else           |
                       |       successor->  |
                       |       handleRequest()}
                       +-------------------+
                            ^       ^      ^
                            |       |      |
                     +------+-+ +---+--+ +-+-------+
                     |Handler1| |Handler2| |Handler3|
                     +---------+ +--------+ +---------+
```

### Chain Flow Diagram

```
  Request arrives
       |
       v
  +----------+   can't handle   +----------+   can't handle   +----------+
  | Handler1 | --------------->  | Handler2 | ----------------> | Handler3 |
  +----------+                   +----------+                   +----------+
  | Check $100|                  | Check $1K |                  | Check $10K|
  | limit     |                  | limit     |                  | limit     |
  +----------+                   +----------+                   +----------+
       |                              |                              |
       | can handle                   | can handle                   | can handle
       v                              v                              v
    [APPROVE]                      [APPROVE]                      [APPROVE]
    
  If NO handler processes it:
  +----------+
  | Default  |  <- optional final handler (or request is rejected)
  | Handler  |
  +----------+
```

### Handler Ordering and Termination

**Ordering strategies:**
- **Priority-based**: Handlers ordered by specificity (most specific first)
- **Authority-based**: Handlers ordered by capability level (lowest authority first)
- **Pipeline**: Every handler processes the request AND passes it on (middleware style)

**Termination strategies:**
1. **First handler wins**: Once handled, stop propagation (pure CoR)
2. **All handlers process**: Every handler does something, request flows through entire chain (pipeline/middleware)
3. **Vote/Aggregate**: All handlers vote, final decision based on consensus

### Middleware Pipeline (Modern Interpretation)
In web frameworks, CoR is implemented as middleware where EVERY handler processes the request:

```
  Request -> [Auth] -> [Logging] -> [Compression] -> [CORS] -> [Route Handler]
                                                                       |
  Response <- [Auth] <- [Logging] <- [Compression] <- [CORS] <--------+
```

Each middleware:
1. Does pre-processing
2. Calls `next()` to pass to the next middleware
3. Does post-processing on the response

```cpp
using Middleware = function<Response(Request, function<Response(Request)>)>;

auto authMiddleware = [](Request req, auto next) {
    if (!req.hasToken()) return Response(401);  // Stop chain
    return next(req);  // Continue chain
};

auto loggingMiddleware = [](Request req, auto next) {
    log("Request: " + req.path());   // Pre-processing
    auto resp = next(req);           // Continue chain
    log("Response: " + resp.code()); // Post-processing
    return resp;
};
```

### Consequences

**Benefits:**
1. Reduced coupling - sender does not know which object handles the request
2. Flexibility in assigning responsibilities - can change the chain dynamically at runtime
3. Single Responsibility - each handler focuses on one concern
4. Open/Closed Principle - add new handlers without modifying existing ones

**Liabilities:**
1. No guarantee of handling - request might fall off the end of the chain unhandled
2. Hard to debug - tracing which handler processed a request requires logging
3. Performance - long chains with many handlers that "pass" add overhead
4. Ordering bugs - wrong handler order can cause subtle issues

### Implementation Notes (C++ Specific)
- Use `std::unique_ptr<Handler>` for the next handler (ownership chain)
- Alternatively, `std::shared_ptr` if handlers are shared across chains
- Consider `std::vector<unique_ptr<Handler>>` for configurable chain ordering
- Use `std::function` for lightweight handlers (no need for class hierarchy)
- For middleware: pass a `std::function` representing the rest of the chain

### Known Uses
- **Web Middleware**: Express.js, ASP.NET Core, Django middleware
- **Java Servlet Filters**: `doFilter(request, response, chain)` with `chain.doFilter()`
- **Logging Frameworks**: log4cpp, spdlog level-based filtering (DEBUG->INFO->WARN->ERROR)
- **Exception Handling**: try/catch blocks form a chain up the call stack
- **Event Bubbling**: DOM events bubble from child to parent elements
- **ATM Dispenser**: Chain of denomination handlers ($100 -> $50 -> $20 -> $10)
- **Linux I/O Schedulers**: Request queues with multiple scheduling policies

### Common Misconceptions
1. **"CoR and Decorator are the same"** - Both chain objects, but Decorator always forwards (adds behavior to ALL requests). CoR may stop at any point (handles OR forwards).
2. **"The chain must handle every request"** - Not necessarily. You can have a "default handler" at the end, or the request can fail if unhandled.
3. **"Handlers must be the same type"** - In typed languages yes (common interface). But in dynamic systems, handlers can be arbitrary callables.

### Related Patterns
- **Composite**: parent link in composite can be used as the "successor" chain
- **Command**: a request passed along the chain can be encapsulated as a Command
- **Decorator**: similar chaining structure but Decorator does not have the concept of "handling" (it always delegates)

---

## Pattern Comparison Matrix

| Aspect | Iterator | Template Method | Chain of Responsibility |
|--------|----------|----------------|------------------------|
| **Category** | Behavioral | Behavioral | Behavioral |
| **Mechanism** | Object composition | Class inheritance | Object composition |
| **Purpose** | Sequential access | Algorithm skeleton | Request routing |
| **Who controls?** | Client (external) or Aggregate (internal) | Base class (framework) | Each handler decides |
| **Flexibility** | Multiple traversal strategies | Fixed skeleton, variable steps | Dynamic chain, any ordering |
| **Coupling** | Iterator to aggregate | Subclass to base class | Handlers to each other (minimal) |
| **Key principle** | Encapsulate traversal | Hollywood Principle (IoC) | Decoupled sender/receiver |

### CoR vs Decorator

| Aspect | Chain of Responsibility | Decorator |
|--------|------------------------|-----------|
| **Propagation** | May stop at any handler | Always propagates through all |
| **Purpose** | Find the RIGHT handler | Add behavior from ALL wrappers |
| **Return** | One handler returns result | Each decorator contributes |
| **Independence** | Handlers handle OR pass | Decorators always delegate AND add |
| **Example** | Approval chain (one approver) | Stream layers (all contribute) |

---

## Code Examples

### Files
- [iterator_custom.cpp](iterator_custom.cpp) - Custom iterator for a binary search tree (in-order traversal)
- [template_method.cpp](template_method.cpp) - Report generator with configurable steps
- [chain_middleware.cpp](chain_middleware.cpp) - HTTP middleware pipeline for request processing

---

## Interview Questions

1. **Iterator vs Visitor** - both traverse structures. What differs?
   - Iterator provides sequential access to elements. Visitor applies operations TO elements. Iterator focuses on traversal ORDER; Visitor focuses on WHAT to do with each element.

2. **External vs Internal iterator** - when to prefer each?
   - External when you need fine-grained control (break, compare, zip two iterators). Internal for simple processing of all elements (cleaner code, potentially parallelizable).

3. **Template Method: How does it enforce DIP via the Hollywood Principle?**
   - The base class (high-level module) defines the algorithm and calls subclass methods (low-level modules). Low-level modules never call high-level code directly. "Don't call us, we'll call you."

4. **CoR vs Decorator** - both chain objects.
   - CoR: a handler either handles the request OR passes it on. Decorator: always wraps and enhances (every decorator participates). CoR can stop; Decorator always delegates.

5. **CoR: How to ensure a request is always handled?**
   - Add a "default handler" or "catch-all" at the end of the chain. Log unhandled requests as errors. Use a Null Object pattern for the terminal handler.

6. **What makes an STL iterator "random access" vs "bidirectional"?**
   - Random access supports arithmetic (`it + 5`, `it[3]`, `it1 - it2`) and comparisons (`<`, `>`). Bidirectional only supports increment/decrement (`++`, `--`). Vector iterators are random access; list iterators are bidirectional.

7. **Why is Template Method considered an "inversion of control" pattern?**
   - Normally client code calls library functions. With Template Method, the base class (framework/library) calls YOUR virtual methods. Control is inverted: the framework runs the show.

---

## Daily Assignment

1. **Iterator**: Build a custom `BSTIterator` for in-order traversal of a binary search tree. Implement `begin()`, `end()`, `operator++`, `operator*`, `operator!=` so it works with range-based for loops.

2. **Template Method**: Build a `DataProcessor` with template method `process()` calling `read() -> validate() -> transform() -> write()`. Implement subclasses for CSV and JSON that override each step differently. Add a `shouldValidate()` hook.

3. **Chain of Responsibility**: Build an expense approval chain: `TeamLead` (up to $100) -> `Manager` (up to $1,000) -> `Director` (up to $10,000) -> `CEO` (any amount). If nobody handles it (should not happen with CEO as terminal), log an error.

4. **Middleware Pipeline**: Build an HTTP middleware pipeline with `AuthMiddleware`, `LoggingMiddleware`, `CompressionMiddleware`, and `RouteHandler`. Each middleware does pre-processing, calls next, then post-processing.
