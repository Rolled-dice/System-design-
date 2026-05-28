# Day 11 - Observer + Strategy Patterns

## Table of Contents
- [Observer Pattern](#observer-pattern)
- [Strategy Pattern](#strategy-pattern)
- [Strategy vs State Deep Comparison](#strategy-vs-state-deep-comparison)
- [Code Examples](#code-examples)
- [Interview Questions](#interview-questions)
- [Daily Assignment](#daily-assignment)

---

## Observer Pattern

### Intent (GoF)
Define a one-to-many dependency between objects so that when one object changes state, all its dependents are notified and updated automatically.

### Also Known As
- Publish-Subscribe
- Dependents
- Event-Subscriber
- Listener

### Real-World Analogy
Think of a newspaper subscription. The publisher (Subject) does not go door-to-door asking if people want news. Instead, subscribers (Observers) register their interest. When a new edition is published, all subscribers receive their copy automatically. Subscribers can cancel at any time without affecting other subscribers or the publisher. The publisher does not need to know anything about the subscribers beyond their mailing address (interface).

Another analogy: a stock ticker display. Multiple displays (web dashboard, mobile app, trading terminal) all show the same stock price. When the price changes on the exchange (Subject), all displays update simultaneously without polling.

### Motivation
Consider a spreadsheet with data cells and multiple charts (bar chart, pie chart, line chart) displaying that data. When a user changes a cell value, all charts must update. The data model should not be hardcoded to specific chart types. New chart types should be addable without modifying the data model. The Observer pattern achieves this decoupling.

### Applicability
Use the Observer pattern when:
- A change to one object requires changing others, and you do not know how many objects need to change
- An object should be able to notify other objects without making assumptions about who those objects are
- You need event-driven architecture with loose coupling
- You want to implement distributed event-handling systems

### Structure

```
  +-------------------+              +-------------------+
  |     Subject       |              |     Observer      |
  +-------------------+              +-------------------+
  | - observers[]     |<>----------->| + update()        |
  | + attach(Observer)|              +-------------------+
  | + detach(Observer)|                    ^       ^
  | + notify()       |                    |       |
  |   { for each o:  |              +-----+--+ +--+-------+
  |     o->update(); }|              |ConcreteA| |ConcreteB |
  +-------------------+              |Observer | |Observer  |
         ^                           +----------+ +----------+
         |
  +-------------------+
  | ConcreteSubject   |
  +-------------------+
  | - state           |
  | + getState()      |
  | + setState()      |
  |   { state = ...;  |
  |     notify(); }   |
  +-------------------+
```

### Sequence Diagram (Push Model)

```
  Subject        ObserverA        ObserverB        ObserverC
     |               |               |               |
     | setState(x)   |               |               |
     |----+          |               |               |
     |    | (state changes)          |               |
     |<---+          |               |               |
     |               |               |               |
     | notify()      |               |               |
     |----+          |               |               |
     |    |          |               |               |
     |    +--------->| update(x)     |               |
     |    |          |----+          |               |
     |    |          |    | process  |               |
     |    |          |<---+          |               |
     |    |          |               |               |
     |    +------------------------->| update(x)     |
     |    |          |               |----+          |
     |    |          |               |    | process  |
     |    |          |               |<---+          |
     |    |          |               |               |
     |    +----------------------------------------->| update(x)
     |    |          |               |               |----+
     |    |          |               |               |    |
     |    |          |               |               |<---+
     |<---+          |               |               |
     |               |               |               |
```

### Push vs Pull Models

| Aspect | Push Model | Pull Model |
|--------|-----------|------------|
| **Mechanism** | Subject sends data in `update(data)` | Subject calls `update()`; observer queries subject |
| **Coupling** | Subject knows what observers need | Observers know about subject's getters |
| **Efficiency** | May send unnecessary data | Observer fetches only what it needs |
| **Flexibility** | Subject decides what to push | Observer decides what to pull |
| **When to use** | All observers need same data | Different observers need different data |

**Push example:**
```cpp
void notify() {
    for (auto& obs : observers)
        obs->update(temperature, humidity, pressure);  // Push all data
}
```

**Pull example:**
```cpp
void notify() {
    for (auto& obs : observers)
        obs->update(this);  // Observer queries what it needs
}
void ConcreteObserver::update(Subject* subject) {
    auto temp = subject->getTemperature();  // Pull only needed data
}
```

### Event Bus Architecture
A more decoupled variant where observers do not subscribe directly to subjects but through an intermediary:

```
  +----------+    publish     +-----------+    dispatch    +----------+
  | Publisher |-------------->| Event Bus |<-------------->| Subscriber|
  +----------+    (event)    +-----------+    subscribe   +----------+
  | emits events |           | routes events  (topic)    | receives events|
  +----------+              | by topic/type  |            +----------+
                            +-----------+
                            Topic-based routing:
                            "price.updated" -> [ObsA, ObsC]
                            "order.placed"  -> [ObsB, ObsD]
```

Benefits of Event Bus over direct Observer:
- Publisher and subscriber do not know each other at all
- Multiple subjects can publish to the same topic
- Filtering by topic/type reduces unnecessary notifications
- Can persist events for replay (Event Sourcing)

### Memory Management: Preventing Observer Leaks

In C++, a common bug is the "lapsed listener" problem:

```
Problem: Observer is destroyed but Subject still holds a raw pointer to it.
         Subject calls update() on a dangling pointer -> UNDEFINED BEHAVIOR!
```

**Solution 1: `weak_ptr` (preferred in modern C++)**
```cpp
class Subject {
    vector<weak_ptr<Observer>> observers;
    
    void notify() {
        // Remove expired observers and notify live ones
        observers.erase(
            remove_if(observers.begin(), observers.end(),
                [](const weak_ptr<Observer>& wp) { return wp.expired(); }),
            observers.end());
        
        for (auto& wp : observers) {
            if (auto sp = wp.lock()) {
                sp->update();
            }
        }
    }
};
```

**Solution 2: RAII-based subscription token**
```cpp
class Subscription {
    Subject* subject;
    Observer* observer;
public:
    ~Subscription() { subject->detach(observer); }  // Auto-unsubscribe
};
```

**Solution 3: Explicitly call `detach()` in observer destructor**
```cpp
ConcreteObserver::~ConcreteObserver() {
    subject->detach(this);  // Must not forget this!
}
```

### Consequences

**Benefits:**
1. Abstract coupling between Subject and Observer - they can belong to different layers
2. Support for broadcast communication - subject does not need to know receiver count
3. Dynamic relationships - observers can be added/removed at runtime
4. Supports the Open/Closed Principle - new observers without modifying subject

**Liabilities:**
1. Unexpected updates - a chain of updates can be hard to trace (cascade problem)
2. No guarantee on notification order (unless specifically implemented)
3. Memory leaks from forgotten subscriptions (lapsed listener)
4. Performance - notifying thousands of observers synchronously can be slow
5. Update storms - observer A's update modifies subject, triggering another notification cycle

### Implementation Notes (C++ Specific)
- Use `std::weak_ptr<Observer>` to prevent dangling pointers
- Consider `std::function<void(const Event&)>` for lambda-based observers
- Use `std::shared_mutex` for thread-safe subscribe/notify in concurrent systems
- Implement `notify()` with copy of observer list to handle mid-notification subscribe/unsubscribe
- Consider async notification with `std::async` or a thread pool for expensive observers

### Known Uses
- **Qt Signals and Slots**: Type-safe Observer implementation with compile-time checking
- **C# Events/Delegates**: Language-level Observer support
- **DOM Event Listeners**: `addEventListener` in web browsers
- **RxCpp/ReactiveX**: Reactive extensions build on Observer for stream processing
- **MVC/MVVM**: Model notifies View(s) of changes through Observer
- **std::condition_variable**: Low-level notification mechanism in C++

### Common Misconceptions
1. **"Observer and Pub-Sub are identical"** - In classic Observer, subject knows about observers directly. In Pub-Sub, there is a broker/bus in between providing full decoupling.
2. **"Observer is always synchronous"** - It can be async with event queues, message buses, or thread pools.
3. **"Subject must notify on every state change"** - Batch notifications or "dirty flag" patterns defer notification until a synchronization point.

### Related Patterns
- **Mediator**: centralizes communication; Observer decentralizes it
- **Command**: can be used as the event/message sent through Observer
- **Memento**: observers can request mementos from the subject to implement undo

---

## Strategy Pattern

### Intent (GoF)
Define a family of algorithms, encapsulate each one, and make them interchangeable. Strategy lets the algorithm vary independently from clients that use it.

### Also Known As
- Policy

### Real-World Analogy
Think of navigation apps. When you ask for directions, you can choose a transportation strategy: driving, walking, cycling, or public transit. Each strategy calculates a different route using a different algorithm. The navigation app (Context) does not care which algorithm is used; it just calls `calculateRoute()` on whichever strategy is selected. You can switch strategies at any time (e.g., car breaks down, switch to public transit).

### Motivation
A text editor needs to break text into lines (line-breaking algorithm). There are multiple approaches:
- Simple: break at spaces nearest to line width
- TeX: minimize "badness" across entire paragraph (dynamic programming)
- Greedy: fill each line as much as possible

Without Strategy, the editor class would contain all three algorithms in a giant switch statement. Adding a new algorithm means modifying the class. Testing each algorithm in isolation is difficult.

With Strategy, each algorithm is encapsulated in its own class. The editor holds a pointer to the current strategy and delegates line-breaking to it.

### Applicability
Use the Strategy pattern when:
- Many related classes differ only in their behavior (algorithm families)
- You need different variants of an algorithm and want to switch at runtime
- An algorithm uses data that clients should not know about (hide complex algorithm-specific structures)
- A class defines many behaviors via conditional statements (replace conditionals with strategy objects)

### Structure

```
  +-------------------+              +-------------------+
  |     Context       |              |     Strategy      |
  +-------------------+  has-a       +-------------------+
  | - strategy: Strat*|------------->| + algorithm()     |
  | + setStrategy()   |              +-------------------+
  | + doWork()        |                    ^       ^
  |   { strategy->    |                    |       |
  |     algorithm(); }|              +-----+--+ +--+-------+
  +-------------------+              |ConcreteA| |ConcreteB |
                                     |Strategy | |Strategy  |
                                     +----------+ +----------+
                                     |+algorithm| |+algorithm|
                                     +----------+ +----------+
```

### Participants
- **Strategy** - declares an interface common to all supported algorithms; Context uses this to call the algorithm defined by a ConcreteStrategy
- **ConcreteStrategy** - implements the algorithm using the Strategy interface
- **Context** - configured with a ConcreteStrategy object; maintains a reference to a Strategy object; may define an interface for Strategy to access its data

### Compile-Time Strategy (Templates) vs Runtime Strategy

C++ uniquely offers BOTH approaches:

**Runtime Strategy (classic OOP - virtual dispatch):**
```cpp
class SortStrategy {
public:
    virtual void sort(vector<int>& data) = 0;
    virtual ~SortStrategy() = default;
};

class QuickSort : public SortStrategy {
public:
    void sort(vector<int>& data) override { /* ... */ }
};

class Sorter {
    unique_ptr<SortStrategy> strategy;
public:
    void setStrategy(unique_ptr<SortStrategy> s) { strategy = move(s); }
    void sort(vector<int>& data) { strategy->sort(data); }
};
```

**Compile-Time Strategy (templates - Policy-Based Design):**
```cpp
template<typename SortPolicy>
class Sorter : private SortPolicy {
public:
    void sort(vector<int>& data) { SortPolicy::sort(data); }
};

struct QuickSortPolicy {
    static void sort(vector<int>& data) { /* ... */ }
};

// Usage: Sorter<QuickSortPolicy> sorter;
```

| Aspect | Runtime Strategy | Compile-Time Strategy (Policy) |
|--------|-----------------|-------------------------------|
| Dispatch | Virtual function call | Inlined, zero overhead |
| Switchable at runtime? | Yes | No (fixed at compile time) |
| Binary size | One binary, multiple strategies | Separate instantiation per policy |
| Flexibility | Maximum runtime flexibility | Maximum performance |
| Use when | Algorithm chosen by user input/config | Algorithm known at compile time |

**STL uses compile-time strategy extensively:**
- `std::sort` takes a comparison policy
- Allocators are policies on containers
- Hash functions are policies on `unordered_map`

### Consequences

**Benefits:**
1. Algorithm families - hierarchies of Strategy classes define reusable algorithm families
2. Alternative to subclassing - encapsulate behavior without subclassing Context
3. Eliminates conditional statements - no switch/if-else for algorithm selection
4. Choice of implementations - same algorithm family, different time/space tradeoffs
5. Easy to add new strategies without modifying Context (Open/Closed Principle)

**Liabilities:**
1. Clients must be aware of different strategies and choose (increased awareness)
2. Communication overhead - Strategy interface must be general enough for all algorithms, some concrete strategies may not use all passed data
3. Increased number of objects - each strategy is a separate object
4. Overkill for 2-3 fixed algorithms that never change

### Implementation Notes (C++ Specific)
- Use `std::function<ReturnType(Args...)>` for lightweight strategies (especially lambdas)
- Use `std::unique_ptr<Strategy>` for ownership in Context
- For stateless strategies, consider making them singletons or static functions
- Template parameters (policies) provide zero-overhead strategy at compile time
- STL algorithms are a built-in strategy pattern: `std::sort(v.begin(), v.end(), comparator)`

### Known Uses
- **STL Algorithms**: Comparators (`std::less`, `std::greater`, custom lambdas)
- **STL Allocators**: `std::allocator`, `pmr::polymorphic_allocator`
- **Compression Libraries**: zlib, snappy, lz4 are interchangeable compression strategies
- **Payment Processing**: Stripe/PayPal/Square as interchangeable payment strategies
- **Routing Algorithms**: Dijkstra, A*, BFS as interchangeable pathfinding strategies
- **Serialization**: JSON, XML, Protobuf, MessagePack as format strategies

### Common Misconceptions
1. **"Strategy and State are the same thing"** - See the comparison below. Key difference: strategies are independent; states transition between each other.
2. **"You always need a Strategy interface class"** - In modern C++, `std::function` or templates can replace a formal Strategy hierarchy for simple cases.
3. **"Strategy requires runtime polymorphism"** - C++ templates allow compile-time strategy (policy-based design) with zero overhead.

### Related Patterns
- **State**: similar structure but strategies are independent while states know about and transition to each other
- **Template Method**: uses inheritance to vary parts of an algorithm; Strategy uses composition
- **Flyweight**: strategy objects often have no state and can be shared as flyweights
- **Bridge**: similar structure but different intent (Bridge separates abstraction levels; Strategy selects algorithms)

---

## Strategy vs State Deep Comparison

These two patterns have nearly identical class diagrams but fundamentally different behaviors:

```
  STRATEGY:                           STATE:
  +---------+     +----------+        +---------+     +----------+
  | Context |---->| Strategy |        | Context |---->|  State   |
  +---------+     +----------+        +---------+     +----------+
  | -strategy|    | +execute()|       | -state  |     | +handle()|
  +---------+     +----------+        | +request|     +----------+
                    ^      ^          +---------+       ^      ^
                    |      |                            |      |
              +-----+  +---+---+                  +----+-+ +--+---+
              |SortA |  |SortB |                  |StateA| |StateB|
              +------+  +------+                  +------+ +------+
                                                  (knows about StateB,
                                                   can transition to it)
```

| Aspect | Strategy | State |
|--------|----------|-------|
| **Who decides?** | Client selects the strategy | State transitions happen internally |
| **Awareness** | Strategies are unaware of each other | States know about and transition to other states |
| **Lifetime** | Strategy typically set once or changed explicitly by client | State changes automatically based on events/conditions |
| **Number active** | One at a time, chosen by client | One at a time, transitions automatically |
| **Purpose** | "How to do something" (algorithm selection) | "What I am right now" (behavior based on state) |
| **Example** | Choose payment method: credit/debit/wallet | Order lifecycle: pending -> paid -> shipped -> delivered |
| **Transition** | External (client calls `setStrategy()`) | Internal (state calls `context->setState(new NextState)`) |
| **Replace conditional** | Replaces `if/switch` on algorithm type | Replaces `if/switch` on object state |

### When to Choose Which
- Object's behavior changes based on its lifecycle/mode? -> **State**
- Need to select an algorithm from a family? -> **Strategy**
- Transitions happen without client intervention? -> **State**
- Client explicitly picks the variant? -> **Strategy**

---

## Code Examples

### Files
- [observer.cpp](observer.cpp) - Stock price observer with multiple display types
- [strategy.cpp](strategy.cpp) - Payment processing with interchangeable strategies
- [strategy_sort.cpp](strategy_sort.cpp) - Pluggable sorting algorithm selection

---

## Interview Questions

1. **Observer vs Pub-Sub** - what is different?
   - Classic Observer: subject directly references observers. Pub-Sub: a broker/bus decouples publishers from subscribers. Pub-Sub adds indirection and usually topic-based routing.

2. **Push vs Pull observer model** - when would you use each?
   - Push when all observers need the same data (simpler). Pull when different observers need different subsets (more efficient, less coupling).

3. **How to avoid memory leaks with observers in C++?**
   - Use `weak_ptr` in the subject's observer list. The observer can be garbage-collected when no other `shared_ptr` references it. Or use RAII subscription tokens that auto-unsubscribe on destruction.

4. **Strategy vs State** - both use polymorphism. What fundamentally differs?
   - Strategy: client selects the algorithm explicitly, strategies are independent. State: transitions happen internally, states know about each other.

5. **Strategy + Factory combo** - why is this common in interviews?
   - Factory creates the appropriate strategy based on configuration/input. This separates strategy creation (Factory) from strategy usage (Context), following SRP.

6. **How does `std::sort` implement the Strategy pattern?**
   - The comparator parameter is a compile-time strategy (policy). You can pass `std::less<>`, `std::greater<>`, or a custom lambda to change sorting behavior without modifying `std::sort`.

7. **Observer notification ordering** - does it matter? How to guarantee it?
   - By default, order is unspecified. For ordered notification, use a priority queue of observers or explicit ordering during subscription (observer priority/weight).

8. **What is the "update storm" problem in Observer?**
   - ObserverA's update modifies the subject, triggering re-notification to all observers (including A again). Solution: batch changes, defer notifications, use "dirty flags".

---

## Daily Assignment

1. **Observer**: Build a `WeatherStation` (subject) notifying `PhoneDisplay`, `WebDisplay`, and `LogDisplay` (observers). Implement both push (data in update) and pull (observer queries subject) variants.

2. **Strategy - Payment**: Build a payment system with `CreditCard`, `UPI`, `Wallet`, `NetBanking` strategies. Context: `PaymentProcessor` that delegates to the selected strategy. Allow runtime switching.

3. **Strategy - Compression**: Implement a file compressor with `ZipStrategy`, `GzipStrategy`, `Bzip2Strategy`. The context chooses the strategy based on file size (small=zip, medium=gzip, large=bzip2).

4. **Observer + Strategy Combined**: Build a `PriceMonitor` (Observer) that watches stock prices (Subject). Each monitor has a configurable `AlertStrategy` (email, SMS, push notification) that determines how it alerts the user when price crosses a threshold.
