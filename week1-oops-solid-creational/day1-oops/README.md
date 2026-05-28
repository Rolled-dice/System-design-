# Day 1 - Object-Oriented Programming Concepts (C++)

## Overview

Object-Oriented Programming (OOP) is the foundational paradigm upon which nearly all modern system design is built. Whether you are designing a microservice, a game engine, or an operating system kernel module, you will encounter OOP concepts at every level. This day provides an exhaustive treatment of the four pillars of OOP, the critical C++ mechanisms that implement them, and the deeper memory-level details that separate a surface-level understanding from true mastery.

The four pillars of OOP are not just academic constructs. They are engineering tools that, when used correctly, produce systems that can evolve over years without collapsing under their own complexity. When used poorly, they produce tangled hierarchies that are worse than procedural code. Understanding the *why* behind each pillar is just as important as understanding the *how*.

---

## The Four Pillars of OOP

### 1. Encapsulation

**Definition:** Encapsulation is the bundling of data (attributes) and the methods (functions) that operate on that data into a single unit (class), while restricting direct access to some of the object's components.

**Real-World Analogy:** Think of a bank vault. The cash, gold, and documents inside (data) are hidden from the outside world. You cannot walk in and grab money directly. Instead, you interact through a controlled interface: the bank teller. The teller (public methods) mediates all access, enforces business rules (no overdrafts, identity verification), and logs every transaction. The vault door and walls (access specifiers: `private`, `protected`) enforce the physical boundary.

**Why It Matters:** Encapsulation is the first line of defense against complexity. By hiding internal state, you create a contract: "You can rely on this interface, and I promise the internals can change without breaking your code." This is the foundation of API design, microservice boundaries, and module decomposition. Without encapsulation, every change to an internal variable potentially breaks every piece of code that touches it.

**C++ Implementation Details:**
- Access specifiers (`private`, `protected`, `public`) control visibility
- `friend` declarations allow controlled access-boundary violations for performance-critical or tightly-coupled components
- `const` member functions guarantee no modification of object state
- The Pimpl idiom (Pointer to Implementation) takes encapsulation further by hiding implementation details from header files, reducing compile-time dependencies

**Common Mistake:** Many developers confuse encapsulation with simply making all members private and adding getters/setters. If your getter returns a mutable reference to internal state, or your setter has no validation logic, you have not achieved meaningful encapsulation. True encapsulation means the class enforces its own invariants.

---

### 2. Inheritance

**Definition:** Inheritance is a mechanism that allows one class (derived/child) to acquire the properties and behaviors of another class (base/parent), enabling code reuse and the modeling of hierarchical relationships.

**Real-World Analogy:** Consider the biological classification system. All mammals share certain characteristics (warm-blooded, produce milk). A dog IS-A mammal and inherits all mammalian properties while adding dog-specific behaviors (barking, fetching). A poodle IS-A dog and inherits further. This hierarchy lets biologists reason at the appropriate level of abstraction without restating shared properties at every level.

**C++ Supports Multiple Inheritance Modes:**
- **Single inheritance:** One base class (the most common and safest form)
- **Multi-level inheritance:** A chain of inheritance (A -> B -> C)
- **Multiple inheritance:** A class inherits from more than one base (powerful but dangerous, see Diamond Problem below)
- **Hierarchical inheritance:** Multiple classes inherit from the same base
- **Hybrid/Virtual inheritance:** Combines forms, using `virtual` to resolve ambiguity

**The Dark Side of Inheritance:** Inheritance is often overused. The mantra "prefer composition over inheritance" exists because deep inheritance hierarchies become brittle. A change in a base class can cascade through dozens of derived classes in unexpected ways. This is the "fragile base class problem." Use inheritance when there is a genuine IS-A relationship; use composition when the relationship is HAS-A or USES-A.

**Why It Matters in System Design:** Inheritance defines extension points. When you design a plugin architecture, a message handler system, or a protocol implementation, you define base classes that establish contracts. Understanding how inheritance interacts with memory layout, virtual dispatch, and access control is essential for designing these extension points correctly.

---

### 3. Polymorphism

**Definition:** Polymorphism means "many forms." It allows a single interface to represent different underlying types, enabling code to work with objects without knowing their exact concrete type at compile time.

**Real-World Analogy:** Consider a universal remote control. It has a "power" button. When pointed at a TV, pressing "power" turns on the TV. When pointed at an air conditioner, the same button turns on the AC. The interface (button press) is the same, but the behavior depends on the underlying device. The remote does not need to know the internal circuitry of each device.

**Two Forms in C++:**

| Aspect | Compile-Time (Static) | Runtime (Dynamic) |
|--------|----------------------|-------------------|
| Mechanism | Function overloading, operator overloading, templates | Virtual functions, vtable dispatch |
| Binding | Resolved at compile time | Resolved at runtime |
| Performance | Zero overhead (inlined) | Indirect function call (vtable lookup) |
| Flexibility | Types must be known at compile time | New types can be added without recompiling |
| Use case | Performance-critical generic code | Plugin architectures, polymorphic containers |

**Performance Tradeoffs:** Runtime polymorphism (virtual dispatch) incurs a cost: an indirection through the vtable pointer, which can cause cache misses on modern CPUs. For hot loops processing millions of objects, this matters. Techniques like CRTP (Curiously Recurring Template Pattern) can achieve polymorphism at compile time, eliminating the vtable overhead while preserving the polymorphic interface.

**Why It Matters in System Design:** Polymorphism is what makes the Open/Closed Principle possible. It enables you to write code that processes a `vector<unique_ptr<Shape>>` without knowing or caring whether the shapes are circles, rectangles, or something invented next year. This is the essence of extensible system design.

---

### 4. Abstraction

**Definition:** Abstraction means exposing only the essential characteristics of an object while hiding the complex implementation details. It answers "what does this do?" without revealing "how does it do it?"

**Real-World Analogy:** When you drive a car, you interact with a steering wheel, pedals, and a gear shift. You do not need to understand the combustion cycle, the electronic fuel injection system, or the differential gear mechanism. The car presents an abstraction: "turn this wheel to change direction, press this pedal to accelerate." The enormous complexity under the hood is irrelevant to the driver.

**C++ Implementation:**
- **Abstract classes** with at least one pure virtual function (`= 0`) define interfaces
- **Pure interfaces** (classes with only pure virtual functions and a virtual destructor) establish contracts without any implementation commitment
- The `override` keyword ensures you are actually overriding a virtual function (catches typos and signature mismatches)
- The `final` keyword prevents further overriding or inheritance

**Abstraction vs Encapsulation:** These are often confused. Encapsulation is a *mechanism* (hiding data behind access specifiers). Abstraction is a *design philosophy* (presenting simplified interfaces). Encapsulation supports abstraction, but they are not synonymous. You can have encapsulation without meaningful abstraction (a class with private members but a confusing public interface), and you can have abstraction without strict encapsulation (a well-designed C module with a clean header file).

**Why It Matters in System Design:** Every layer in a system architecture represents an abstraction boundary. The network stack abstracts TCP/IP details from application code. A database driver abstracts SQL dialect differences. A cache layer abstracts storage latency. Designing the right abstractions is the most important skill in system design.

---

## Memory Layout: vtable and vptr

Understanding how C++ implements runtime polymorphism at the memory level is essential for performance analysis and debugging.

### How Virtual Dispatch Works

When a class declares or inherits a virtual function, the compiler generates a **vtable** (virtual function table) for that class. Each object of that class contains a hidden **vptr** (virtual table pointer) that points to the class's vtable.

```
                    MEMORY LAYOUT
                    
  Object (Base*)         vtable for Derived
  +-------------+       +-------------------+
  | vptr --------+----->| &Derived::func1() |
  +-------------+       +-------------------+
  | base members |       | &Base::func2()    |  (not overridden)
  +-------------+       +-------------------+
  | derived      |       | &Derived::func3() |
  | members      |       +-------------------+
  +-------------+
  
  When you call: basePtr->func1()
  
  1. Load vptr from object      (memory access #1)
  2. Index into vtable          (memory access #2)
  3. Call function pointer       (indirect branch)
  
  Cost: ~2 pointer dereferences + potential cache miss
        vs direct call: 0 dereferences, branch predictor friendly
```

### vtable Layout for Inheritance Hierarchy

```
  class Animal { virtual void speak(); virtual void eat(); };
  class Dog : public Animal { void speak() override; void fetch(); };
  class Cat : public Animal { void speak() override; void purr(); };
  
  Animal vtable:          Dog vtable:            Cat vtable:
  +--------------+       +--------------+       +--------------+
  | &Animal::speak|       | &Dog::speak  |       | &Cat::speak  |
  +--------------+       +--------------+       +--------------+
  | &Animal::eat |       | &Animal::eat |       | &Animal::eat |
  +--------------+       +--------------+       +--------------+
  
  Animal* a = new Dog();
  a->speak();  // goes to Dog vtable -> calls Dog::speak()
```

### Key Points About vtable/vptr
- Each class with virtual functions has exactly ONE vtable (shared among all instances)
- Each object has exactly ONE vptr (8 bytes on 64-bit systems)
- The vptr is set in the constructor (which is why calling virtual functions in constructors is dangerous: the vptr is not yet pointing to the derived vtable)
- Multiple inheritance means multiple vptrs per object (one per base class with virtuals)

---

## Compile-Time vs Runtime Polymorphism

### When to Use Each

**Use compile-time polymorphism (templates, CRTP) when:**
- Performance is critical (inner loops, real-time systems, game engines)
- The set of types is known at compile time
- You want zero-overhead abstractions
- You can tolerate longer compile times and larger binary sizes

**Use runtime polymorphism (virtual functions) when:**
- Types are determined at runtime (plugin loading, user input, configuration)
- You need polymorphic containers (`vector<unique_ptr<Base>>`)
- You need binary-stable interfaces (shared libraries, ABI compatibility)
- You want to minimize compile-time dependencies

### CRTP: The Static Polymorphism Pattern

```
template<typename Derived>
class Base {
public:
    void interface() { static_cast<Derived*>(this)->implementation(); }
};

class Concrete : public Base<Concrete> {
public:
    void implementation() { /* ... */ }
};
```

This gives you polymorphic behavior with zero virtual dispatch overhead. The compiler resolves all calls at compile time. The tradeoff: you cannot store different CRTP-derived types in the same container, and the pattern produces larger binaries due to template instantiation.

---

## RAII: Resource Acquisition Is Initialization

### Philosophy

RAII is perhaps the most important C++ idiom. It states that resource acquisition (memory allocation, file opening, mutex locking, network connection) should happen in a constructor, and resource release should happen in the destructor. Because C++ guarantees that destructors run when objects go out of scope (even during exception unwinding), RAII ensures resources are never leaked.

### Why RAII Is Fundamental

In languages with garbage collection (Java, Go, Python), memory is managed automatically but other resources (files, locks, connections) require explicit cleanup. In C++, RAII manages ALL resources uniformly. This eliminates entire categories of bugs:

- Memory leaks (solved by `unique_ptr`, `shared_ptr`)
- File handle leaks (solved by RAII file wrappers)
- Mutex deadlocks from forgotten unlocks (solved by `lock_guard`, `unique_lock`)
- Connection pool exhaustion (solved by RAII connection handles)

### Connection to Smart Pointers

Smart pointers ARE RAII applied to heap memory:

| Smart Pointer | Ownership Model | Use Case |
|---------------|----------------|----------|
| `unique_ptr<T>` | Exclusive ownership | Default choice, zero overhead |
| `shared_ptr<T>` | Shared ownership (ref-counted) | When lifetime is genuinely shared |
| `weak_ptr<T>` | Non-owning observer | Breaking cycles, caches |

### RAII in System Design

RAII principles scale beyond single objects. Connection pools, thread pools, and transaction managers all follow the pattern: acquire on construction, release on destruction. When designing distributed systems in C++, RAII ensures that network connections are returned to pools, database transactions are committed or rolled back, and temporary files are cleaned up, regardless of how control flow exits a function.

---

## The Diamond Problem

### What Is It?

When a class inherits from two classes that share a common base, it can end up with two copies of the base class's members, creating ambiguity.

```
        +----------+
        |  Animal  |
        | + name   |
        | + eat()  |
        +----+-----+
             |
     +-------+-------+
     |               |
+----+----+     +----+----+
|  Mammal |     |   Bird  |
| + nurse()|     | + fly() |
+----+----+     +----+----+
     |               |
     +-------+-------+
             |
        +----+----+
        |   Bat   |  // Bat is both Mammal and Bird
        +---------+
        
WITHOUT virtual inheritance:
  Bat has TWO copies of Animal::name and Animal::eat()
  bat.name is ambiguous - which Animal::name?
  
WITH virtual inheritance:
  class Mammal : virtual public Animal { ... };
  class Bird   : virtual public Animal { ... };
  class Bat    : public Mammal, public Bird { ... };
  
  Bat has exactly ONE copy of Animal
```

### How Virtual Inheritance Works (Memory Model)

With virtual inheritance, the base class subobject is shared. The compiler uses a **vbase pointer** (or offset) to locate the single shared base. This adds indirection but eliminates duplication.

```
  Normal inheritance memory:     Virtual inheritance memory:
  +----------+                   +----------+
  | Animal#1 |                   | Mammal   |
  +----------+                   | (vbase*) ---+
  | Mammal   |                   +----------+  |
  +----------+                   | Bird     |  |
  | Animal#2 |                   | (vbase*) ---+
  +----------+                   +----------+  |
  | Bird     |                   | Bat      |  |
  +----------+                   +----------+  |
  | Bat      |                   | Animal   | <+  (shared, at end)
  +----------+                   +----------+
```

### Practical Impact

Virtual inheritance has costs: increased object size (vbase pointers), slightly slower access to base members, and complex construction rules (the most-derived class must initialize the virtual base). Use it only when multiple inheritance genuinely requires a shared base.

---

## Rule of 3 / 5 / 0

### The Rule of Three (C++98/03)

If your class manages a resource (raw pointer, file handle, etc.), you must define ALL THREE of:
1. **Destructor** - to release the resource
2. **Copy constructor** - to duplicate the resource correctly
3. **Copy assignment operator** - to release old + acquire new

If you define one but not the others, the compiler-generated defaults will perform shallow copies, leading to double-free, use-after-free, or resource leaks.

### The Rule of Five (C++11)

With move semantics, add:
4. **Move constructor** - to transfer ownership efficiently
5. **Move assignment operator** - to release old + transfer from source

### The Rule of Zero (Modern C++)

**The preferred approach:** If you use RAII types (`unique_ptr`, `shared_ptr`, `string`, `vector`) for all resource management, you do not need to define ANY special member functions. The compiler-generated defaults will do the right thing because each member's destructor handles its own cleanup.

```
// Rule of Zero: no special members needed
class User {
    std::string name;
    std::vector<std::string> emails;
    std::unique_ptr<Address> address;
};
// Compiler generates correct destructor, move ops, and deletes copy
// (because unique_ptr is non-copyable)
```

This is the modern ideal: compose your class from RAII-managing members, and let the compiler do the rest.

---

## Common Misconceptions

1. **"Virtual functions are slow"** - The overhead is one pointer indirection. On modern CPUs with branch predictors, this is often negligible. Profile before optimizing away virtual functions.

2. **"Inheritance is better than composition"** - This was the OOP hype of the 1990s. Modern consensus: inheritance models IS-A relationships; composition models everything else. Overusing inheritance leads to rigid hierarchies.

3. **"Abstract class = Interface"** - In C++, an abstract class CAN have data members and method implementations. A pure interface has only pure virtual functions and a virtual destructor. Java/C# have a formal `interface` keyword; C++ does not.

4. **"Encapsulation = making things private"** - Encapsulation is about invariant enforcement, not just hiding data. A class with all-private data but public setters that do no validation is not meaningfully encapsulated.

5. **"Multiple inheritance is always bad"** - Multiple inheritance of interfaces (pure abstract classes) is safe and common. Multiple inheritance of implementation (classes with data) is dangerous. The diamond problem arises from the latter.

6. **"Constructors can be virtual"** - They cannot. The object's type must be known to call the constructor. The Virtual Constructor idiom uses a factory method or clone method to achieve a similar effect.

7. **"RAII only applies to memory"** - RAII applies to any resource with acquire/release semantics: files, locks, sockets, database connections, GPU handles, etc.

---

## Why Does This Matter in System Design?

Object-oriented design is the vocabulary of Low-Level Design (LLD). When an interviewer asks you to design a parking lot, an elevator system, or a chess game, they expect you to:

1. **Identify classes** (abstraction): What are the entities and their responsibilities?
2. **Define interfaces** (polymorphism): What behaviors vary across types?
3. **Hide complexity** (encapsulation): What invariants must be protected?
4. **Model hierarchies** (inheritance): What IS-A relationships exist?

Beyond interviews, these concepts are the building blocks of:
- **Plugin architectures** (virtual functions as extension points)
- **Serialization frameworks** (polymorphic dispatch to serialize different types)
- **Event systems** (observer pattern relies on polymorphic handlers)
- **Testing** (mock objects require virtual functions or templates)
- **API design** (encapsulation determines what can change without breaking clients)

Mastering OOP gives you the language to communicate design decisions. Mastering the underlying mechanisms (vtable, memory layout, RAII) gives you the ability to make those decisions correctly.

---

## Key C++ Concepts Reference

| Concept | Use |
|---------|-----|
| `virtual` / `override` / `final` | Runtime polymorphism |
| Pure virtual `= 0` | Abstract class |
| `virtual ~Base()` | Avoid memory leaks via base pointer |
| `friend` | Controlled access break |
| `static` member | Class-level state |
| `const` correctness | Immutable methods/data |
| Smart pointers (`unique_ptr`, `shared_ptr`) | Avoid raw `new`/`delete` |
| Rule of 3 / 5 / 0 | Resource management |
| `explicit` | Prevent unwanted implicit conversions |

---

## Code Walkthroughs

- [encapsulation.cpp](encapsulation.cpp)
- [inheritance.cpp](inheritance.cpp)
- [polymorphism.cpp](polymorphism.cpp)
- [abstraction.cpp](abstraction.cpp)

---

## Interview Questions

**LLD-style**
1. Difference between abstract class and interface in C++?
2. Why does C++ need a virtual destructor?
3. What is the diamond problem? How does virtual inheritance solve it?
4. Difference between `override` and `final`.
5. When does the compiler create a vtable?
6. Can a constructor be virtual? Why/why not?
7. Difference between `static` and `dynamic` polymorphism with cost trade-offs.
8. Explain RAII with an example.

**Conceptual**
9. Encapsulation vs Abstraction - draw boundaries clearly.
10. How does C++ implement multiple inheritance memory layout?

**Advanced**
11. Explain how the vptr is set during construction of a derived object. What happens if you call a virtual function in a constructor?
12. What is the Curiously Recurring Template Pattern (CRTP) and when would you use it over virtual dispatch?
13. How does `shared_ptr` reference counting work in a multithreaded environment?
14. Explain object slicing. When does it occur and how do you prevent it?
15. What is the difference between `unique_ptr` and `shared_ptr` in terms of overhead and use cases?

---

## Daily Assignment

1. Implement a `Shape` hierarchy: `Shape -> Circle/Rectangle/Triangle` with `area()` and `perimeter()`. Use a `vector<unique_ptr<Shape>>` and compute total area polymorphically.
2. Add a `Drawable` interface (pure abstract) and make `Circle` inherit both `Shape` and `Drawable`. Demonstrate diamond resolution if you also add `Printable -> Drawable`.
3. Write a class `BankAccount` showing encapsulation: private balance, public deposit/withdraw with validation, throw on overdraft.
4. Implement a RAII wrapper class `FileHandle` that opens a file in the constructor and closes it in the destructor. Ensure it is non-copyable but movable (Rule of Five with deleted copy operations).
5. Create a simple CRTP example: a `Counter<T>` base that tracks how many instances of each derived type exist.
