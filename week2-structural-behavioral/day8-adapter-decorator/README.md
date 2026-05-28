# Day 8 - Adapter + Decorator Patterns

## Table of Contents
- [Adapter Pattern](#adapter-pattern)
- [Decorator Pattern](#decorator-pattern)
- [Adapter vs Decorator vs Proxy Comparison](#adapter-vs-decorator-vs-proxy-comparison)
- [Code Examples](#code-examples)
- [Interview Questions](#interview-questions)
- [Daily Assignment](#daily-assignment)

---

## Adapter Pattern

### Intent (GoF)
Convert the interface of a class into another interface clients expect. Adapter lets classes work together that could not otherwise because of incompatible interfaces.

### Also Known As
- Wrapper

### Real-World Analogy
Think of a power plug adapter when you travel internationally. Your laptop charger has a US two-prong plug, but the wall socket in Europe accepts only round pins. The adapter sits between your plug and the socket, translating one physical interface into another without modifying either the plug or the socket. The laptop still gets power; the socket still provides it. The adapter simply bridges the incompatibility.

Another analogy: a human interpreter between two people who speak different languages. Neither person changes how they speak; the interpreter translates between them.

### Motivation
You are building a graphics editor that works with `Shape` objects and their `draw()` method. You find an excellent third-party library called `LegacyTextView` that renders text beautifully but exposes a completely different interface: `getExtent()` and `display()`. You cannot modify the library code (you only have the compiled binary or it is maintained externally). You also cannot change all your client code that depends on `Shape::draw()`.

The Adapter pattern solves this by creating a `TextShape` adapter class that implements the `Shape` interface while internally delegating to `LegacyTextView`.

### Applicability
Use the Adapter pattern when:
- You want to use an existing class but its interface does not match what you need
- You want to create a reusable class that cooperates with unrelated or unforeseen classes
- You need to use several existing subclasses but it is impractical to adapt each by subclassing every one (use an object adapter)
- You are integrating legacy systems or third-party SDKs into a modern codebase

### Structure

#### Object Adapter (uses composition - preferred in C++)

```
  +----------+          +------------------+
  |  Client  |          |     Target       |
  +----------+          +------------------+
       |                | + request()      |
       |                +------------------+
       |                         ^
       |                         |
       |                +------------------+
       +--------------->|     Adapter      |
                        +------------------+
                        | - adaptee: Adaptee*  |
                        | + request()      |
                        |   { adaptee->   |
                        |     specificReq()}|
                        +------------------+
                                 |
                                 v
                        +------------------+
                        |     Adaptee      |
                        +------------------+
                        | + specificRequest()|
                        +------------------+
```

#### Class Adapter (uses multiple inheritance)

```
  +----------+      +--------+      +------------------+
  |  Client  |----->| Target |      |     Adaptee      |
  +----------+      +--------+      +------------------+
                    |request()|      | specificRequest()|
                    +--------+      +------------------+
                         ^                   ^
                         |                   |
                         +-------+-----------+
                                 |
                        +------------------+
                        |     Adapter      |
                        +------------------+
                        | + request()      |
                        |   { specificReq()}|
                        +------------------+
```

### Participants
- **Target** - defines the domain-specific interface that Client uses
- **Client** - collaborates with objects conforming to the Target interface
- **Adaptee** - defines an existing interface that needs adapting
- **Adapter** - adapts the interface of Adaptee to the Target interface

### Object Adapter vs Class Adapter

| Aspect | Object Adapter | Class Adapter |
|--------|---------------|---------------|
| Mechanism | Composition (holds a pointer to Adaptee) | Multiple inheritance |
| Flexibility | Can adapt any subclass of Adaptee | Adapts only the specific Adaptee class |
| Override behavior | Cannot override Adaptee methods directly | Can override Adaptee behavior since it inherits |
| C++ suitability | Preferred (avoids diamond problems) | Possible with MI but less common |
| Runtime swap | Can swap adaptee at runtime | Locked at compile time |

### Two-Way Adapter
A two-way adapter adapts BOTH interfaces so that either can be used as the other. This is useful when two different subsystems need to work with each other and both consider themselves the "primary" interface:

```
  +--------+                              +--------+
  |Interface|<--- TwoWayAdapter -------->|Interface|
  |   A    |     (implements both)       |   B    |
  +--------+                              +--------+
```

In C++, a two-way adapter inherits from both interfaces and delegates appropriately based on which interface method is called.

### Consequences

**Benefits:**
1. Single Responsibility - separates interface translation from business logic
2. Open/Closed Principle - introduce new adapters without changing existing client code
3. Works with legacy code you cannot modify

**Liabilities:**
1. Overall complexity increases (more classes)
2. Sometimes it is simpler to just change the service class interface directly
3. Object adapter requires forwarding each method (can be tedious for large interfaces)

### Implementation Notes (C++ Specific)
- Prefer object adapter using composition; multiple inheritance introduces complexity
- Use `std::function` or lambdas as lightweight adapters for single-method interfaces
- STL already uses adapters: `stack` adapts `deque`, `priority_queue` adapts `vector`
- Iterator adapters: `reverse_iterator`, `back_insert_iterator` adapt standard iterators

### Known Uses
- **STL Container Adapters**: `std::stack`, `std::queue`, `std::priority_queue` adapt underlying containers
- **STL Iterator Adapters**: `std::reverse_iterator`, `std::move_iterator`
- **Java**: `Arrays.asList()` adapts an array to the List interface
- **Legacy database drivers**: ODBC drivers adapt vendor-specific APIs to a uniform interface
- **Wrappers for C libraries in C++**: e.g., wrapping `sqlite3_*` C functions in a C++ class

### Common Misconceptions
1. **"Adapter changes the adaptee's behavior"** - No. Adapter only translates the interface. If you are adding or modifying behavior, that is Decorator or Proxy.
2. **"Adapter and Facade are the same"** - Facade simplifies a complex subsystem into a new, simpler interface. Adapter makes an existing interface conform to an expected one without simplification.
3. **"Always use class adapter in C++"** - Object adapter is almost always preferred; class adapter introduces MI complications.

### Related Patterns
- **Bridge**: separates interface from implementation at design time; Adapter makes things work after they are designed
- **Decorator**: enhances an object without changing its interface; Adapter changes the interface without enhancing
- **Proxy**: provides the same interface; Adapter provides a different interface
- **Facade**: defines a new simpler interface; Adapter reuses an existing interface

---

## Decorator Pattern

### Intent (GoF)
Attach additional responsibilities to an object dynamically. Decorators provide a flexible alternative to subclassing for extending functionality.

### Also Known As
- Wrapper

### Real-World Analogy
Think of clothing layers. You start with a basic t-shirt (the core component). On a cold day, you add a sweater (first decorator). If it rains, you add a raincoat (second decorator). Each layer adds functionality (warmth, water protection) without modifying the t-shirt itself. You can combine layers in any order and remove them independently.

Another analogy: pizza toppings. You have a base pizza (Margherita), and each topping (mushrooms, pepperoni, extra cheese) decorates it with additional flavor and cost.

### Motivation: The Inheritance Explosion Problem

Consider a notification system. You have a base `Notifier` that sends email. Now you need SMS, Slack, and push notifications too. Using inheritance:

```
                    Notifier
                   /   |    \
          SMSNotifier SlackNotifier PushNotifier
         /     \         |
  SMS+Slack  SMS+Push  Slack+Push
         \       |      /
          SMS+Slack+Push
```

With 4 notification types, you need 2^4 - 1 = 15 subclasses to cover all combinations! This is the **class explosion** problem. Adding one more type doubles the number of classes.

With Decorator, you need only 4 decorator classes that can be wrapped in any combination at runtime:

```cpp
auto notifier = make_shared<SlackDecorator>(
                  make_shared<SMSDecorator>(
                    make_shared<BaseNotifier>("email@example.com")));
notifier->send("Server is down!");
// Sends: email + SMS + Slack
```

### Applicability
Use the Decorator pattern when:
- You need to add responsibilities to individual objects dynamically and transparently, without affecting other objects
- You want to add responsibilities that can be withdrawn later
- Extension by subclassing is impractical due to an explosion of subclasses
- You need to layer cross-cutting concerns (logging, caching, authentication, compression)

### Structure

```
  +------------------+
  |    Component     |<---------------------------------+
  +------------------+                                  |
  | + operation()    |                                  |
  +------------------+                                  |
       ^         ^                                      |
       |         |                                      |
+-----------+ +------------------+                      |
|Concrete   | |    Decorator     |--- component: Component*
|Component  | +------------------+                      |
+-----------+ | + operation()    |                      |
| +operation| |  { component->   |                      |
|  ()|      | |    operation();  |                      |
+-----------+ |    // + extra }  |                      |
              +------------------+                      |
                   ^        ^                           |
                   |        |                           |
          +--------+  +--------+                        |
          |ConcreteA|  |ConcreteB|                      |
          |Decorator|  |Decorator|                      |
          +--------+  +--------+
```

The key structural insight: Decorator both IS-A Component (same interface) and HAS-A Component (holds a reference). This allows unlimited recursive wrapping.

### Participants
- **Component** - defines the interface for objects that can have responsibilities added dynamically
- **ConcreteComponent** - defines an object to which additional responsibilities can be attached
- **Decorator** - maintains a reference to a Component object and defines an interface conforming to Component's interface
- **ConcreteDecorator** - adds responsibilities to the component

### How Layering Works

```
  Client calls operation() on outermost decorator:

  +----------------------------------------------+
  | ConcreteDecoratorB                           |
  |  +----------------------------------------+ |
  |  | ConcreteDecoratorA                      | |
  |  |  +----------------------------------+  | |
  |  |  |     ConcreteComponent            |  | |
  |  |  |     (core behavior)              |  | |
  |  |  +----------------------------------+  | |
  |  |  (adds behavior A before/after)        | |
  |  +----------------------------------------+ |
  |  (adds behavior B before/after)              |
  +----------------------------------------------+

  Execution flow:
  DecoratorB::operation()
    -> pre-B logic
    -> DecoratorA::operation()
         -> pre-A logic
         -> ConcreteComponent::operation()  // core
         -> post-A logic
    -> post-B logic
```

### Consequences

**Benefits:**
1. More flexibility than static inheritance - add/remove at runtime
2. Avoids feature-laden classes high up in the hierarchy
3. A decorator and its component are not identical - decorator is transparent to the component
4. Incremental feature addition: pay-as-you-go complexity

**Liabilities:**
1. Lots of little objects - system becomes harder to debug (deep stack traces)
2. Decorator is not identical to its component (identity checks fail: `decorator != component`)
3. Order can matter - composing decorators in wrong order may produce bugs
4. Interface pollution: if Component interface is large, every decorator must implement every method

### When Decorator Becomes Too Complex
If you find yourself with 10+ decorators stacked on an object, or if decorator ordering has complex rules, consider:
- **Strategy pattern** for interchangeable algorithms
- **Chain of Responsibility** for request processing pipelines
- **Middleware pattern** (formalized Chain of Responsibility)
- A full **Plugin architecture** for maximum extensibility

### Implementation Notes (C++ Specific)
- Use `std::shared_ptr<Component>` for the wrapped reference to manage lifetime
- Keep the Component interface small (otherwise every decorator must implement many methods)
- Consider using CRTP (Curiously Recurring Template Pattern) for static decoration
- Decorator constructors take the component to wrap as a parameter
- Ensure proper copy/move semantics for decorator chains

### Known Uses
- **Java I/O Streams**: `BufferedReader(new InputStreamReader(new FileInputStream("file.txt")))` is 3 decorators deep
- **C++ `std::` smart pointers**: `shared_ptr` can be seen as decorating raw pointers with reference counting
- **Web middleware**: Express.js, ASP.NET, Django middleware each "decorate" the request handling pipeline
- **GUI toolkits**: Scrollbars, borders, and shadows as decorators on base widgets (original Smalltalk MVC)
- **Logging/Metrics wrappers**: Add timing, logging, retry logic around service calls

### Common Misconceptions
1. **"Decorator always goes before/after the call"** - A decorator can modify arguments, suppress the call entirely, or modify the return value.
2. **"Decorator and inheritance do the same thing"** - Inheritance is static and applies to the entire class; Decorator is dynamic and applies per-instance.
3. **"You must have an abstract Decorator base class"** - In simple cases, you can have concrete decorators inherit directly from Component.

### Related Patterns
- **Adapter**: changes an object's interface; Decorator enhances without changing interface
- **Composite**: both have recursive structure, but Composite aggregates children while Decorator adds behavior
- **Strategy**: changes the guts of an object; Decorator changes the skin
- **Proxy**: controls access; Decorator adds behavior (but their structures are nearly identical)

---

## Adapter vs Decorator vs Proxy Comparison

All three patterns involve wrapping another object. Here is how they differ:

| Aspect | Adapter | Decorator | Proxy |
|--------|---------|-----------|-------|
| **Primary Purpose** | Interface conversion | Add responsibilities | Control access |
| **Interface Change** | Provides a DIFFERENT interface | Provides the SAME interface (enhanced) | Provides the SAME interface |
| **Modifies Behavior?** | No (only translates) | Yes (adds behavior) | May (controls access, adds caching) |
| **Wraps** | An incompatible class | A compatible class (same interface) | The same class (same interface) |
| **When Introduced** | After design (fix incompatibility) | During design (flexible extension) | During design (access control) |
| **Number of Wrappers** | Usually one | Often multiple stacked | Usually one |
| **Typical Use** | Legacy integration | Cross-cutting concerns | Lazy loading, security, remote |
| **Client Awareness** | Client uses new interface | Client unaware of decoration | Client unaware of proxy |
| **Lifecycle** | Lives as long as adaptation needed | Can be added/removed dynamically | Lives as long as real subject |

### Decision Guide
- Need to make an old/foreign interface work with your code? -> **Adapter**
- Need to add optional behavior to an object at runtime? -> **Decorator**
- Need to control when/how/if a real object is accessed? -> **Proxy**

---

## Code Examples

### Files
- [adapter.cpp](adapter.cpp) - Adapts a legacy XML logger to a modern ILogger interface
- [decorator_coffee.cpp](decorator_coffee.cpp) - Classic coffee shop example with stackable toppings
- [decorator_stream.cpp](decorator_stream.cpp) - Real-world data stream with encryption + compression layers

---

## Interview Questions

1. **Adapter vs Decorator vs Proxy** - all three wrap another object. What fundamentally differs?
   - Adapter changes interface, Decorator enhances same interface, Proxy controls access with same interface.

2. **Object Adapter vs Class Adapter** (multiple inheritance) in C++. When would you use class adapter?
   - Class adapter when you need to override adaptee behavior; object adapter for runtime flexibility and avoiding MI.

3. **Why is Decorator preferred over inheritance for cross-cutting concerns?**
   - Avoids class explosion, allows runtime combination, single responsibility per decorator.

4. **How do middleware chains in web frameworks use the Decorator pattern?**
   - Each middleware wraps the next handler, adding behavior (auth, logging, compression) before/after.

5. **Real-world: Design IO streams** (Java's `BufferedReader(new InputStreamReader(new FileReader(...)))`)
   - Each stream is a decorator adding buffering/decoding/reading capabilities.

6. **What happens if you need to adapt AND decorate?** Can you combine both patterns?
   - Yes. Adapt first (fix interface), then decorate (add behavior). Or use a single class that does both if justified.

7. **When does Decorator become an anti-pattern?**
   - When decorator chains get too deep (hard to debug), when ordering constraints exist, or when the component interface is too large.

---

## Daily Assignment

1. **Adapter**: Wrap a `LegacyXmlParser` (returns raw XML `string`) into a modern `IDataParser` interface returning `JsonObject`. The adapter should parse XML and convert to JSON format.

2. **Decorator**: Implement `ICoffee` interface with `Espresso` base. Add `MilkDecorator`, `SugarDecorator`, `WhipDecorator`. Each adds its own price and description. Stack them: `Whip(Milk(Sugar(Espresso)))`.

3. **Stream Pipeline**: Build `RawStream -> CompressDecorator -> EncryptDecorator`. Each decorator transforms the data, and the pipeline should work in both directions (write: compress then encrypt; read: decrypt then decompress).

4. **Two-Way Adapter Challenge**: Create a `MediaPlayer` that can play audio and a `VideoPlayer` that can play video. Build a two-way adapter so either system can request playback from the other using its own interface.
