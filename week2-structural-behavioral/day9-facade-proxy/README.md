# Day 9 - Facade + Proxy Patterns

## Table of Contents
- [Facade Pattern](#facade-pattern)
- [Proxy Pattern](#proxy-pattern)
- [Facade vs Proxy vs Adapter vs Mediator Comparison](#facade-vs-proxy-vs-adapter-vs-mediator-comparison)
- [Code Examples](#code-examples)
- [Interview Questions](#interview-questions)
- [Daily Assignment](#daily-assignment)

---

## Facade Pattern

### Intent (GoF)
Provide a unified interface to a set of interfaces in a subsystem. Facade defines a higher-level interface that makes the subsystem easier to use.

### Real-World Analogy
Consider a home theater system. You have a DVD player, amplifier, projector, screen, lights, and subwoofer. To watch a movie, you need to: dim lights, lower screen, turn on projector, set input to HDMI, turn on amplifier, set volume, insert disc, press play. That is 8 steps involving 6 different devices.

A universal remote with a "Watch Movie" button is a Facade. One button press orchestrates all the complex subsystem interactions. The subsystem components still exist and can be used directly, but most of the time you just press one button.

Another analogy: a hotel concierge. You tell the concierge "I need a restaurant reservation, a taxi, and theater tickets." They coordinate all the separate services. You interact with one simple interface (the concierge) instead of three separate services.

### Motivation
A compiler has many subsystems: Scanner, Parser, ProgramNodeBuilder, CodeGenerator, etc. Most clients just want to compile code; they do not care about the internal pipeline. A `Compiler` facade class exposes a simple `compile(istream&, BytecodeStream&)` method that orchestrates all the subsystems internally.

Without the facade, clients couple to every subsystem class. Changes to subsystem internals break client code. The facade provides a stable, simple interface while allowing power users to bypass it and access subsystems directly.

### Applicability
Use the Facade pattern when:
- You want to provide a simple interface to a complex subsystem
- There are many dependencies between clients and implementation classes
- You want to layer your subsystems (each layer gets a facade as its entry point)
- You want to decouple clients from subsystem components to promote subsystem independence

### Structure

```
  +---------+
  | Client  |
  +---------+
       |
       v
  +---------+          +-----+-----+-----+-----+
  | Facade  |--------->| Sub | Sub | Sub | Sub |
  +---------+          |sys A|sys B|sys C|sys D|
  |+simple  |          +-----+-----+-----+-----+
  | Method()|          (internal collaborations)
  +---------+
       |
       | Client can still access subsystems directly if needed
       v
  (optional direct subsystem access)
```

### Participants
- **Facade** - knows which subsystem classes are responsible for a request; delegates client requests to appropriate subsystem objects
- **Subsystem classes** - implement subsystem functionality; handle work assigned by the Facade; have no knowledge of the facade (no reference to it)

### Connection to the Law of Demeter
The Law of Demeter (LoD) states: "Only talk to your immediate friends." A Facade embodies this principle by giving clients a single "friend" to talk to, rather than reaching deep into subsystem internals.

Without facade (violates LoD):
```cpp
order.getCustomer().getAddress().getCity().getZipCode();
```

With facade:
```cpp
orderFacade.getShippingZip(orderId);
```

### Layered Architecture and Facades
In layered architectures, facades serve as entry points to each layer:

```
  +-----------------------------------+
  |         Presentation Layer         |
  |  (Controllers use Service Facade)  |
  +-----------------------------------+
                   |
                   v
  +-----------------------------------+
  |         Service Layer (Facade)     |
  |  OrderService, PaymentService      |
  +-----------------------------------+
                   |
                   v
  +-----------------------------------+
  |         Domain Layer               |
  |  Entities, Value Objects, Rules    |
  +-----------------------------------+
                   |
                   v
  +-----------------------------------+
  |         Infrastructure Layer       |
  |  Database, Messaging, External API |
  +-----------------------------------+
```

Each layer communicates only through the facade of the layer below it.

### Consequences

**Benefits:**
1. Shields clients from subsystem components, reducing the number of objects clients deal with
2. Promotes weak coupling between the subsystem and its clients
3. Does not prevent clients from using subsystem classes directly when they need fine-grained control
4. Simplifies porting to other platforms (only the facade needs changing)

**Liabilities:**
1. Facade can become a "god object" if it tries to do too much
2. May hide important complexity that clients should be aware of
3. Adding new subsystem features means updating the facade
4. Facade does not add new functionality - it only simplifies existing interfaces

### Implementation Notes (C++ Specific)
- Make subsystem classes private to a namespace or module; expose only the Facade
- Consider using the Pimpl idiom (pointer to implementation) to hide subsystem headers from clients
- Facade methods should be thin coordination layers, not contain business logic themselves
- A facade can be a singleton if only one instance is ever needed

### Known Uses
- **Compiler design**: `Compiler::compile()` hides scanner, parser, optimizer, code generator
- **JDBC/ODBC**: `DriverManager.getConnection()` hides protocol negotiation, pooling, auth
- **Home automation**: "Scenes" in smart home apps (one tap does many things)
- **Game engines**: `Engine::initialize()` sets up rendering, physics, audio, input subsystems
- **SDL/SFML**: Simple wrappers over complex OS-level windowing, audio, and input APIs

### Common Misconceptions
1. **"Facade replaces subsystem classes"** - No. It provides an alternative simplified interface. Subsystems remain accessible.
2. **"Facade adds behavior"** - No. Facade only delegates and coordinates. For added behavior, use Decorator.
3. **"One facade per subsystem only"** - You can have multiple facades for different use cases of the same subsystem.
4. **"Facade pattern is just a class with a bunch of methods"** - The intent matters: it specifically simplifies complex subsystem interaction.

### Related Patterns
- **Abstract Factory**: can be used with Facade to create subsystem objects independently of their concrete classes
- **Mediator**: similar to Facade but mediates between colleague objects (bidirectional vs unidirectional)
- **Singleton**: Facade is often implemented as a Singleton since only one is usually needed
- **Adapter**: Adapter wraps one object and changes its interface; Facade wraps an entire subsystem with a new interface

---

## Proxy Pattern

### Intent (GoF)
Provide a surrogate or placeholder for another object to control access to it.

### Also Known As
- Surrogate

### Real-World Analogy
A credit card is a proxy for a bank account. Both implement the same "payment" interface. The credit card controls access to the real bank account: it verifies limits, adds transaction records, and can decline purchases. The merchant interacts with the card (proxy) the same way they would with cash (real subject) from the customer's perspective.

Another analogy: a secretary who screens phone calls for an executive. Callers interact with the secretary (proxy) using the same interface (phone call), but the secretary controls access to the executive (real subject).

### Motivation
Consider displaying a document that contains high-resolution images. Loading all images immediately would be slow and waste memory. A `VirtualProxy` stands in for each image. It displays a placeholder and loads the real image only when the user scrolls to it. The document editor code treats the proxy exactly like a real image (same interface), but the expensive object creation is deferred.

### Types of Proxy

#### 1. Virtual Proxy (Lazy Initialization)
Delays expensive object creation until first use.

```
  Client -----> ImageProxy -----> [loads on first draw()] -----> RealImage
                 (lightweight)                                    (heavy, loads file)
```

**Use cases**: Large images, database connections, expensive computations

#### 2. Protection Proxy (Access Control)
Controls who can do what with the real object.

```
  Client -----> ProtectionProxy -----> [checks permissions] -----> RealSubject
                                       [deny if unauthorized]
```

**Use cases**: RBAC, read-only wrappers, admin vs user views

#### 3. Remote Proxy (Network Transparency)
Represents an object in a different address space. Handles serialization, network calls, and response deserialization.

```
  Client -----> RemoteProxy -----> [serialize, send over network] -----> RealObject
  (local)       (local stub)       [deserialize response]                (remote server)
```

**Use cases**: RPC stubs (gRPC, CORBA), distributed objects, microservice clients

#### 4. Caching Proxy
Stores results of expensive operations and returns cached results for repeated requests.

```
  Client -----> CachingProxy -----> [check cache] ---hit---> return cached
                                         |
                                         +---miss---> RealService -> cache result -> return
```

**Use cases**: HTTP caching, database query caching, API response caching

#### 5. Smart Reference Proxy
Performs additional actions when an object is accessed: reference counting, logging, locking, etc.

```
  Client -----> SmartPtr -----> [increment refcount] -----> RawObject
                                [decrement on destroy]
                                [delete when count == 0]
```

**Use cases**: `std::shared_ptr` (reference counting), `std::unique_ptr` (exclusive ownership), thread-safe wrappers

### Structure

```
  +------------------+
  |     Subject      |<----------------------------------+
  +------------------+                                   |
  | + request()      |                                   |
  +------------------+                                   |
       ^         ^                                       |
       |         |                                       |
+-----------+ +------------------+                       |
|RealSubject| |      Proxy       |--- realSubject: Subject*
+-----------+ +------------------+
| +request()| | + request()      |
|           | |   { // control   |
|           | |     // access    |
|           | |     realSubject  |
|           | |     ->request(); }|
+-----------+ +------------------+
```

### Participants
- **Subject** - defines the common interface for RealSubject and Proxy so that a Proxy can be used anywhere a RealSubject is expected
- **RealSubject** - defines the real object that the proxy represents
- **Proxy** - maintains a reference to the real subject; controls access to it; may be responsible for creating/deleting it

### Lazy Initialization Explained
Virtual proxy implements on-demand loading:

```cpp
class ImageProxy : public Image {
    string filename;
    Image* realImage = nullptr;  // NOT loaded yet
public:
    ImageProxy(string file) : filename(file) {}
    
    void draw() override {
        if (!realImage) {
            realImage = new RealImage(filename);  // Load on first use
        }
        realImage->draw();
    }
};
```

The real object is only created when `draw()` is first called. Until then, the proxy is a lightweight placeholder. This saves time at initialization and memory if the object is never actually used.

### Consequences

**Benefits:**
1. Controls access to the real object without clients knowing
2. Manages lifecycle of the real object (create on demand, destroy when unused)
3. Works even if the real object is not ready or available (remote proxy)
4. Open/Closed Principle: add new proxies without changing the real subject or client

**Liabilities:**
1. Response might be delayed (first call triggers lazy loading)
2. Code complexity increases with another indirection layer
3. Smart reference proxies may introduce contention in multithreaded scenarios

### Implementation Notes (C++ Specific)
- `std::shared_ptr` and `std::unique_ptr` are smart reference proxies built into the language
- Overload `operator->` and `operator*` to create transparent proxies in C++
- Use `std::mutex` in proxy for thread-safe lazy initialization
- Consider `std::call_once` and `std::once_flag` for one-time initialization
- For remote proxies, use gRPC or similar frameworks that auto-generate proxy stubs

### Known Uses
- **C++ Smart Pointers**: `shared_ptr` = smart reference proxy, `unique_ptr` = exclusive ownership proxy
- **ORM Lazy Loading**: Hibernate's lazy-loaded entity references are virtual proxies
- **gRPC/Thrift Stubs**: Client stubs are remote proxies for server-side services
- **Copy-on-Write (COW)**: `std::string` in some implementations uses a COW proxy
- **Virtual Memory**: The OS uses page tables as proxies for physical memory

### Common Misconceptions
1. **"Proxy and Decorator are the same"** - Proxy controls access (and usually creates the real object itself). Decorator adds behavior (and receives the component from outside).
2. **"Virtual proxy is always needed"** - Only use when object creation is genuinely expensive. For lightweight objects, the proxy overhead is counterproductive.
3. **"Remote proxy hides all network issues"** - It cannot hide latency and failure modes; clients must still handle timeouts and errors.

### Related Patterns
- **Adapter**: provides a different interface to an object; Proxy provides the same interface
- **Decorator**: similar structure but different intent; Decorator adds responsibilities, Proxy controls access
- **Facade**: simplifies a subsystem; Proxy represents a single object

---

## Facade vs Proxy vs Adapter vs Mediator Comparison

| Aspect | Facade | Proxy | Adapter | Mediator |
|--------|--------|-------|---------|----------|
| **Wraps** | Entire subsystem | Single object | Single object | Multiple colleagues |
| **Interface** | New simplified interface | Same as subject | Different (target) | New coordination interface |
| **Purpose** | Simplification | Access control | Compatibility | Decoupling communication |
| **Direction** | Unidirectional (client -> subsystem) | Unidirectional (client -> subject) | Unidirectional | Bidirectional |
| **Knowledge** | Subsystem unaware of facade | Subject unaware of proxy | Adaptee unaware | Colleagues aware of mediator |
| **Multiplicity** | One facade, many subsystem classes | One proxy per subject | One adapter per adaptee | One mediator, many colleagues |

---

## Code Examples

### Files
- [facade.cpp](facade.cpp) - Home theater facade simplifying complex subsystem
- [proxy_virtual.cpp](proxy_virtual.cpp) - Lazy image loading with virtual proxy
- [proxy_protection.cpp](proxy_protection.cpp) - RBAC access control with protection proxy

---

## Interview Questions

1. **Facade vs Adapter** - both wrap. What fundamentally differs?
   - Facade provides a NEW simplified interface to a complex subsystem. Adapter converts an EXISTING interface to match what a client expects. Facade simplifies; Adapter translates.

2. **Facade vs Mediator** - both coordinate multiple objects.
   - Facade provides one-way simplified access (subsystem does not know about facade). Mediator handles two-way communication between colleagues who know about the mediator.

3. **Proxy vs Decorator** - both wrap a single object with the same interface.
   - Proxy controls access/lifecycle (often creates the real object itself). Decorator adds behavior (receives the component as a constructor parameter from outside).

4. **How would you cache a remote service call with Proxy?**
   - Create a CachingProxy that stores results keyed by request parameters. On each call, check cache first; on miss, delegate to real service and cache the result.

5. **Real-world: Design an API gateway as Facade.**
   - Gateway aggregates multiple microservice calls into a single client-facing endpoint. Client calls `GET /dashboard`, gateway calls User Service + Order Service + Analytics Service internally.

6. **Explain lazy initialization with Proxy. When is it NOT worth it?**
   - Virtual proxy defers creation until first use. Not worth it when the object is always used, is cheap to create, or when the first-call latency spike is unacceptable.

7. **What is the Pimpl idiom and how does it relate to Facade?**
   - Pimpl (Pointer to Implementation) hides implementation details behind a pointer, reducing header dependencies. It is a compile-time facade for a single class.

---

## Daily Assignment

1. **Facade**: Build a `HomeTheaterFacade` controlling Amplifier, DVDPlayer, Projector, Lights, Screen with `watchMovie()` and `endMovie()` methods that orchestrate all subsystems.

2. **Virtual Proxy**: Implement an `ImageProxy` for a high-resolution image that displays a placeholder until `draw()` is first called, then loads the real image.

3. **Protection Proxy**: Implement a `DocumentProxy` that checks user roles before allowing `read()`, `write()`, and `delete()` operations. Admin can do all; Editor can read/write; Viewer can only read.

4. **Caching Proxy Challenge**: Build a `WeatherServiceProxy` that caches API responses for 5 minutes. If the same city is queried within 5 minutes, return cached data; otherwise fetch fresh data from the real service.
