# Day 7 - Prototype Pattern

## Overview

The Prototype pattern creates new objects by copying existing instances rather than constructing from scratch. When object creation is expensive (complex initialization, database lookups, network calls) or when you want to avoid a parallel hierarchy of factory classes, cloning a pre-configured prototype is both simpler and faster.

This pattern is particularly relevant in C++ because the language has built-in copy semantics (copy constructors, assignment operators) that directly support it. However, C++ copy semantics also introduce subtle pitfalls (shallow vs deep copy, slicing) that you must understand to use Prototype correctly.

---

## GoF Documentation

### Intent

Specify the kinds of objects to create using a prototypical instance, and create new objects by copying this prototype.

### Motivation

Consider a music editor framework that provides tools like notes, rests, and staves. The framework defines a `Graphic` class and subclasses for each musical symbol. But the framework cannot know in advance which specific musical symbols an application will use - users might define custom note styles, ornaments, or articulation marks.

Rather than creating a factory class for each symbol type, we can clone a prototypical instance. The editor maintains a palette of prototypical symbols. When the user places a note, the editor clones the prototype from the palette. This way, new symbol types can be added simply by adding a new prototype to the palette, without any factory code.

### Applicability

Use Prototype when:
- A system should be independent of how its products are created, composed, and represented AND
- The classes to instantiate are specified at runtime (e.g., by dynamic loading) OR
- You want to avoid building a class hierarchy of factories that parallels the class hierarchy of products OR
- Instances of a class can have only a few different combinations of state, and it is more convenient to install prototypes and clone them rather than instantiating the class manually each time

**Decision Checklist:**
1. Is object creation expensive (complex initialization, IO, computation)?
2. Do you have many "template" objects that users configure once and then stamp out copies?
3. Would a factory hierarchy mirror the product hierarchy pointlessly?
4. Are objects distinguished mainly by their state (not their class)?

### Structure

```
+-------------------+          +-----------------------+
|      Client       |          |      Prototype        |
+-------------------+          +-----------------------+
| - prototype:      |--------->| + clone(): Prototype* |
|   Prototype*      |          +-----------+-----------+
+-------------------+                      ^
| + operation() {   |                      |
|   p = prototype   |           +----------+----------+
|       ->clone();  |           |                     |
| }                 |           |                     |
+-------------------+  +--------+-------+  +----------+--------+
                       | ConcreteProto1 |  | ConcreteProto2    |
                       +----------------+  +-------------------+
                       | + clone():     |  | + clone():        |
                       |   return new   |  |   return new      |
                       |   ConcretePro  |  |   ConcreteProto2  |
                       |   to1(*this)   |  |   (*this)         |
                       +----------------+  +-------------------+

  // Usage:
  Prototype* original = registry.get("warrior");
  Prototype* copy = original->clone();  // New object, same state
  copy->customize(...);
```

### Participants

- **Prototype** (Graphic) - declares an interface for cloning itself
- **ConcretePrototype** (Staff, WholeNote, HalfNote) - implements the clone operation
- **Client** (GraphicTool) - creates a new object by asking a prototype to clone itself
- **Registry** (optional) - maintains a collection of named prototypes that can be cloned on demand

### Collaborations

A client asks a prototype to clone itself. The clone is a new, independent object with the same state as the original. The client can then customize the clone without affecting the original.

### Consequences

**Benefits:**
1. **Adding and removing products at runtime** - you can add new prototypes to the system simply by registering new prototype instances. This is more flexible than other creational patterns because the client can install and remove prototypes at runtime.
2. **Specifying new objects by varying values** - you can define new behavior through object composition by specifying values for an object's variables. This reduces the number of classes you need.
3. **Specifying new objects by varying structure** - complex objects that differ in composition (e.g., circuits made of subcircuits) can be cloned from prototypes that represent the desired structure.
4. **Reduced subclassing** - Factory Method often produces a hierarchy of Creator classes that parallels the product hierarchy. Prototype eliminates this need - you clone a prototype instead of asking a factory method to make a new object.

**Costs:**
1. **Deep copy complexity** - implementing `clone()` correctly can be difficult, especially when objects contain circular references, shared sub-objects, or non-copyable resources (file handles, mutexes).
2. **Every ConcretePrototype must implement clone** - this can be burdensome if classes already exist and adding clone support requires modification.
3. **Shallow vs deep ambiguity** - the semantics of clone (what gets shared vs what gets duplicated) must be clearly defined and documented.

### Implementation Details (C++ Specific)

**Virtual clone using covariant return types:**
```cpp
class Shape {
public:
    virtual ~Shape() = default;
    virtual std::unique_ptr<Shape> clone() const = 0;
};

class Circle : public Shape {
    double radius_;
    Point center_;
public:
    Circle(double r, Point c) : radius_(r), center_(c) {}
    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Circle>(*this);  // Uses copy constructor
    }
};
```

**Why `unique_ptr` for clone:**
- Communicates ownership transfer clearly
- Prevents memory leaks (no raw `new` visible to caller)
- Enables polymorphic return (base pointer to derived object)

**Copy constructor as the clone mechanism:**
The copy constructor is C++'s built-in clone mechanism. `clone()` typically delegates to the copy constructor:
```cpp
std::unique_ptr<Base> clone() const override {
    return std::make_unique<Derived>(*this);
}
```

This means the copy constructor must perform a correct deep copy. If it does not, clone will produce a broken copy.

### Known Uses

- **JavaScript prototype chain** - every JS object has a prototype; new objects inherit from existing ones
- **Game engines** - enemy templates, particle effect prefabs, item databases with cloneable templates
- **Document editors** - copy/paste creates clones of complex objects (formatted text, embedded images, tables)
- **CAD systems** - stamp out copies of component templates
- **Prototype-based languages** (Self, Lua, JavaScript) - the entire object system is based on cloning rather than class instantiation
- **Object pools** - pre-created prototype objects are cloned (or reused) to avoid allocation overhead

### Related Patterns

- **Abstract Factory** - can be implemented using prototypes: the factory stores prototype instances and clones them instead of creating with `new`
- **Composite** and **Decorator** - designs that use these patterns can benefit from Prototype for copying complex structures
- **Singleton** - a prototype registry might be a Singleton
- **Factory Method** - alternative when you want creation logic in subclasses rather than cloning

---

## Clone vs Constructor: Trade-offs

### When Cloning Beats Constructing

| Scenario | Constructor | Clone |
|----------|-------------|-------|
| Complex initialization (DB load, file parse) | Expensive every time | Expensive once, cheap copies |
| Many similar objects with slight variations | Repetitive setup code | Clone + tweak |
| Runtime type unknown (polymorphic) | Cannot call `new ???` | `prototype->clone()` works |
| Deep object graphs | Must rebuild graph | Deep copy preserves structure |
| Pre-configured templates | Pass all config params | Store template, clone it |

### When Constructors Win

- Simple objects with few parameters
- Objects with unique, non-derivable state (timestamps, UUIDs)
- When the "template" concept does not apply
- When copy semantics are expensive or impossible (mutex, file handle, thread)

---

## Shallow vs Deep Copy in C++ (Pitfalls)

### The Problem

```cpp
class Document {
    std::string title;
    std::vector<Section*> sections;  // Raw pointers!
};

Document original;
Document copy = original;  // DEFAULT COPY: shallow!
```

With the default (compiler-generated) copy constructor:
- `title` is deep-copied (std::string manages its own memory)
- `sections` vector is copied, but the POINTERS are copied, not the pointed-to Sections
- Both `original` and `copy` now point to the same Section objects
- Modifying a section through `copy` also modifies `original`
- Deleting `original` leaves `copy` with dangling pointers

```
  SHALLOW COPY:
  
  original.sections[0] ----+
                            |----> [Section A]  (SHARED!)
  copy.sections[0] --------+
  
  // Danger: delete original -> copy has dangling pointer
  // Danger: modify through copy -> original also changes


  DEEP COPY:
  
  original.sections[0] --------> [Section A original]
  
  copy.sections[0] ------------> [Section A copy]  (INDEPENDENT)
  
  // Safe: each object owns its own subgraph
```

### Implementing Deep Copy Correctly

```cpp
class Document {
    std::string title_;
    std::vector<std::unique_ptr<Section>> sections_;
public:
    // Deep copy constructor
    Document(const Document& other) : title_(other.title_) {
        for (const auto& section : other.sections_) {
            sections_.push_back(section->clone());  // Polymorphic deep copy
        }
    }
    
    std::unique_ptr<Document> clone() const {
        return std::make_unique<Document>(*this);
    }
};
```

### Rules for Safe Cloning in C++

1. **Use smart pointers** (`unique_ptr`, `shared_ptr`) instead of raw pointers. `shared_ptr` gives you automatic sharing (shallow copy with reference counting). `unique_ptr` forces you to implement explicit deep copy.
2. **Implement clone() on polymorphic sub-objects.** If a member is a pointer-to-base, the copy constructor cannot know the dynamic type. Use `clone()` recursively.
3. **Handle circular references** carefully. If object A references B and B references A, naive deep copy creates infinite recursion. Use a "copy map" that tracks already-cloned objects.
4. **Delete or explicitly define copy operations.** The Rule of 3/5 (from Day 1) applies. If your class has pointer members, define copy/move operations explicitly or delete them.
5. **Test clone correctness.** Verify that modifying the clone does not affect the original (deep copy) and that destroying either does not corrupt the other.

---

## Prototype Registry Pattern

### Concept

A Prototype Registry (also called a Prototype Manager) maintains a dictionary of named prototypes. Clients request clones by name:

```cpp
class PrototypeRegistry {
    std::unordered_map<std::string, std::unique_ptr<Shape>> prototypes_;
public:
    void registerPrototype(const std::string& name, std::unique_ptr<Shape> proto) {
        prototypes_[name] = std::move(proto);
    }
    
    std::unique_ptr<Shape> create(const std::string& name) const {
        auto it = prototypes_.find(name);
        if (it == prototypes_.end()) return nullptr;
        return it->second->clone();
    }
};

// Setup:
registry.registerPrototype("red_circle", std::make_unique<Circle>(Color::Red, 5.0));
registry.registerPrototype("blue_square", std::make_unique<Square>(Color::Blue, 3.0));

// Usage:
auto shape = registry.create("red_circle");  // Clone of the red circle prototype
shape->setPosition(10, 20);  // Customize the clone
```

### Benefits of Registry

- **Centralized management** of all available prototypes
- **Dynamic addition/removal** at runtime (load from config, plugins, user definitions)
- **Decouples clients from concrete types** - clients only know the registry name, not the class
- **Enables data-driven creation** - prototype definitions can come from files, databases, or network

### Game Engine Application

Game engines commonly use prototype registries for spawning entities:

```
Registry contents:
  "goblin_warrior"  -> Goblin(hp=100, atk=15, def=5, weapon="sword")
  "goblin_archer"   -> Goblin(hp=80, atk=20, def=3, weapon="bow")
  "dragon_boss"     -> Dragon(hp=5000, atk=100, phase=1)

Spawning:
  auto enemy = registry.create("goblin_warrior");
  enemy->setPosition(spawnPoint);
  world.addEntity(std::move(enemy));
```

Each spawn creates an independent clone. The prototype's expensive initialization (loading meshes, computing AI graphs) happens once during registration. Spawning is a cheap copy operation.

---

## Real-World Prototype Implementations

### JavaScript Prototype Chain

JavaScript is a prototype-based language. Every object has a hidden `[[Prototype]]` link to another object. Property lookup traverses this chain:

```
const animal = { speak() { return "..."; } };
const dog = Object.create(animal);  // dog's prototype is animal
dog.speak();  // Found on animal (prototype chain lookup)
dog.fetch = () => "ball";  // Own property on dog
```

This is Prototype pattern at the language level. Objects are created by cloning (or linking to) existing objects, not by instantiating classes.

### Game Object Cloning (Unity Prefabs)

Unity's `Instantiate()` function is essentially Prototype:
```
GameObject enemyPrefab;  // Configured in editor (prototype)
GameObject enemy = Instantiate(enemyPrefab, position, rotation);  // Clone
```

The prefab is the prototype. Instantiation performs a deep clone of the entire game object hierarchy (components, children, scripts). This is faster than constructing from scratch because materials, meshes, and behaviors are already configured.

### Document Templates

Word processors use prototypes for document templates. A "Business Letter" template is a pre-configured document. Creating "New from Template" clones the template, giving you a new document with the template's formatting, headers, and layout.

### Object Pools

Object pools pre-allocate prototype objects and "clone" them by resetting state rather than constructing new instances. This avoids allocation overhead in performance-critical systems (game engines, real-time systems):

```
Bullet* pool[MAX_BULLETS];  // Pre-allocated
Bullet* fire() {
    Bullet* b = getFromPool();
    b->reset(playerPosition, playerDirection);  // "Clone" by reset
    return b;
}
```

---

## Connection to Other Creational Patterns

| Pattern | Relationship to Prototype |
|---------|--------------------------|
| Factory Method | Alternative: Factory creates new, Prototype clones existing |
| Abstract Factory | Can use prototypes internally (factory stores prototypes, clones on request) |
| Builder | Builder constructs step-by-step; Prototype copies all-at-once |
| Singleton | Registry is often a Singleton; prototypes themselves are NOT singletons |

### Prototype vs Factory Method

- **Factory Method:** "I know HOW to create objects of various types"
- **Prototype:** "I have EXAMPLES of objects; copy them"

Factory Method requires a creator class hierarchy. Prototype requires only a clone interface. If you have many types but simple state differences, Prototype avoids class explosion.

---

## Common Misconceptions

1. **"Prototype = copy constructor"** - The copy constructor is the MECHANISM that often implements clone in C++. The Prototype PATTERN is the design concept of creating objects by copying existing instances, including the registry, the virtual clone interface, and the decoupling from concrete types.

2. **"Prototype always means deep copy"** - Not necessarily. Some prototypes use shallow copy intentionally (shared sub-objects). The depth of copy depends on the use case and must be documented.

3. **"Prototype is only useful for expensive objects"** - While performance is a common motivation, decoupling (clients do not need to know concrete classes) and data-driven creation (prototypes loaded from config) are equally valid motivations.

4. **"Default copy constructor is sufficient for clone"** - Only for simple objects with value semantics. Any class with pointer members, polymorphic sub-objects, or non-copyable resources needs explicit copy logic.

5. **"Prototype replaces Factory"** - They serve different purposes. Prototype is best when objects differ mainly in state. Factory is best when objects differ in type (different class hierarchies with different interfaces).

6. **"Cloning is always cheaper than construction"** - Not always. If the prototype has a large state (huge vectors, large strings), deep copying is expensive. Prototype works best when initialization is expensive but the final state is moderate in size.

---

## Why Does This Matter in System Design?

Prototype patterns appear in system architecture as:
- **Configuration templates** - base configurations cloned and customized per environment (dev, staging, prod)
- **Caching** - cached objects are prototypes; serving a request clones the cached object rather than recomputing
- **Object pooling** - database connection pools, thread pools reuse/clone pre-initialized objects
- **A/B testing** - request handlers are cloned with different feature flag configurations
- **Container orchestration** - pod templates in Kubernetes are prototypes that are cloned to create running containers

Understanding Prototype helps you design systems where:
- Object creation cost matters (games, real-time systems)
- Configuration is data-driven (loading from files, databases)
- Types are not known at compile time (plugin systems, scripting engines)

---

## Files

- [shape_prototype.cpp](shape_prototype.cpp)
- [game_unit_clone.cpp](game_unit_clone.cpp) - Real-world: game unit registry

---

## Interview Questions

1. When does cloning beat constructing?
2. How does C++ copy constructor relate to Prototype? Is it the same?
3. Deep vs shallow clone - when is each appropriate?
4. How to clone polymorphically through a base pointer? (`virtual clone()`)
5. Combine Prototype with Registry - design a registry that returns copies.

**Advanced**
6. How would you handle circular references in a deep clone operation?
7. Design a serialization-based clone (serialize to bytes, deserialize to new object). When is this appropriate?
8. How does Prototype interact with move semantics in C++11+?
9. Compare Prototype-based object creation with reflection-based creation (Java `Class.newInstance()`).
10. How would you implement Prototype in a system with shared sub-objects (flyweight elements that should NOT be cloned)?

---

## Daily Assignment

1. Build a `Document` with sections, each section with paragraphs. Implement deep clone.
2. Build a `GameUnitRegistry` storing prototype warriors/archers/mages. `spawn(name)` returns clones.
3. Use `unique_ptr<Base> clone() const` on a polymorphic hierarchy.
4. Implement a `ConfigTemplate` system: base configurations are stored as prototypes. `createEnvironment("production")` clones the base config and applies production-specific overrides.
5. Create a prototype-based particle system: a `ParticleEmitter` stores a prototype `Particle`. Each frame, it clones the prototype and randomizes position/velocity within defined ranges.
