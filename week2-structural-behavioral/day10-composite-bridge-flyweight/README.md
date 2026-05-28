# Day 10 - Composite, Bridge, and Flyweight Patterns

## Table of Contents
- [Composite Pattern](#composite-pattern)
- [Bridge Pattern](#bridge-pattern)
- [Flyweight Pattern](#flyweight-pattern)
- [Pattern Comparison Matrix](#pattern-comparison-matrix)
- [Code Examples](#code-examples)
- [Interview Questions](#interview-questions)
- [Daily Assignment](#daily-assignment)

---

## Composite Pattern

### Intent (GoF)
Compose objects into tree structures to represent part-whole hierarchies. Composite lets clients treat individual objects and compositions of objects uniformly.

### Real-World Analogy
A military hierarchy: an Army is composed of Divisions, which are composed of Brigades, which are composed of Platoons, which are composed of Squads, which contain individual Soldiers. When a General gives the order "Attack!", it propagates down the tree. Each level either executes the command (leaf: Soldier) or forwards it to children (composite: Division). The General does not care whether they are commanding one soldier or an entire army; the interface is the same: `execute(order)`.

Another analogy: a file system. A Directory can contain Files and other Directories. When you request `size()`, a file returns its own size; a directory returns the sum of sizes of all its contents recursively.

### Motivation
Consider a graphics editor that has simple shapes (Line, Rectangle, Circle) and groups of shapes. Users should be able to group shapes together, and groups of groups, and treat a group the same way as a single shape (move, resize, draw). Without Composite, you need separate code paths for single shapes vs groups everywhere in your application.

### Applicability
Use the Composite pattern when:
- You want to represent part-whole hierarchies of objects
- You want clients to be able to ignore the difference between compositions and individual objects
- The structure can be nested to any depth
- Operations should propagate recursively through the tree

### Structure

```
  +-------------------+
  |    Component      |<---------------------------------+
  +-------------------+                                  |
  | + operation()     |                                  |
  | + add(Component)  |                                  |
  | + remove(Component)|                                 |
  | + getChild(int)   |                                  |
  +-------------------+                                  |
       ^         ^                                       |
       |         |                                       |
  +---------+ +-------------------+                      |
  |  Leaf   | |    Composite      |--- children: Component*[]
  +---------+ +-------------------+
  |+operation| | + operation()    |
  |  ()      | |   { for child   |
  |          | |     in children: |
  |          | |     child->      |
  |          | |     operation(); }|
  +---------+ | + add(Component)  |
              | + remove(Component)|
              +-------------------+
```

### Participants
- **Component** - declares the interface for objects in the composition; may declare interface for accessing and managing child components; optionally defines default behavior
- **Leaf** - represents leaf objects in the composition (has no children); defines behavior for primitive objects
- **Composite** - defines behavior for components having children; stores child components; implements child-related operations
- **Client** - manipulates objects in the composition through the Component interface

### Safety vs Transparency Trade-off

This is a classic design tension in Composite:

**Transparency approach** (GoF default): put `add()`, `remove()`, `getChild()` in Component.
- Benefit: client treats all components uniformly (no type checking needed)
- Cost: leaves have meaningless child operations (calling `add()` on a Leaf is nonsensical)

**Safety approach**: put child operations ONLY in Composite.
- Benefit: leaves cannot have invalid operations called on them
- Cost: client must distinguish between Leaf and Composite (type checking/casting)

```
  TRANSPARENCY:                    SAFETY:
  +------------+                   +------------+
  | Component  |                   | Component  |
  |------------|                   |------------|
  | +operation |                   | +operation |
  | +add()     |                   +------------+
  | +remove()  |                        ^    ^
  | +getChild()|                        |    |
  +------------+                   +----+    +----+
      ^    ^                       |Leaf|    |Composite|
      |    |                       +----+    |+add()   |
  +----+  +------+                           |+remove()|
  |Leaf|  |Compos.|                          +---------+
  +----+  +------+
```

In C++, the safety approach is generally preferred because:
- Compile-time type checking catches errors early
- Dynamic casting is available when needed
- Prevents accidental misuse of leaf objects

### Consequences

**Benefits:**
1. Defines class hierarchies of primitive and composite objects - can compose recursively
2. Makes client code simple - treats all objects uniformly
3. Easy to add new component types - no client code changes needed
4. Natural for tree-structured data (UI, documents, organizations)

**Liabilities:**
1. Makes design overly general - hard to restrict what children a composite can have
2. Cannot rely on type system to enforce constraints (e.g., "a File cannot contain other items")
3. May need to implement runtime checks for invalid operations on leaves

### Implementation Notes (C++ Specific)
- Use `std::vector<std::shared_ptr<Component>>` for children in Composite
- Consider `std::unique_ptr` if parent owns children exclusively
- Use `virtual ~Component() = default` for proper cleanup
- Implement `begin()`/`end()` iterators on Composite for range-based for loops
- Use RTTI (`dynamic_cast`) sparingly for the safety approach when needed

### Known Uses
- **File Systems**: Files and Directories (Qt `QFileInfo`, Java `java.io.File`)
- **UI Widget Trees**: Every GUI framework (Qt `QWidget`, Java Swing `JComponent`, HTML DOM)
- **Organization Charts**: Departments containing teams containing individuals
- **Arithmetic Expressions**: Operands (leaves) and operators (composites) in expression trees
- **XML/JSON/HTML**: Elements containing other elements and text nodes

### Related Patterns
- **Decorator**: similar recursive structure but different purpose (adding behavior vs part-whole)
- **Iterator**: used to traverse Composite structures
- **Visitor**: can apply operations across a Composite tree without modifying element classes
- **Chain of Responsibility**: often combined (child-parent chain for event handling)

---

## Bridge Pattern

### Intent (GoF)
Decouple an abstraction from its implementation so that the two can vary independently.

### Also Known As
- Handle/Body

### Real-World Analogy
Think of a TV remote control (abstraction) and the TV itself (implementation). You can have different remotes (basic remote, advanced remote with voice control) and different TVs (Sony, Samsung, LG). Any remote can work with any TV. You do not need a SonyBasicRemote, SonyAdvancedRemote, SamsungBasicRemote, SamsungAdvancedRemote - that would be an explosion of classes. Instead, the remote "bridges" to whatever TV implementation it is paired with.

### Motivation: The N x M Class Explosion Problem

Without Bridge, extending in two dimensions creates a multiplicative explosion:

```
  Shapes: Circle, Square, Triangle  (N = 3)
  Renderers: Vector, Raster, SVG    (M = 3)
  
  WITHOUT BRIDGE: N * M = 9 classes required!
  
  VectorCircle    VectorSquare    VectorTriangle
  RasterCircle    RasterSquare    RasterTriangle
  SVGCircle       SVGSquare       SVGTriangle
  
  Adding one new shape OR renderer requires adding 3 more classes!
```

```
  WITH BRIDGE: N + M = 6 classes required!
  
  Abstraction:      Circle, Square, Triangle (each holds a Renderer*)
  Implementation:   VectorRenderer, RasterRenderer, SVGRenderer
  
  Adding a new shape: 1 new class
  Adding a new renderer: 1 new class
  
  SAVINGS: From quadratic growth to linear growth
```

### Applicability
Use the Bridge pattern when:
- You want to avoid a permanent binding between abstraction and implementation
- Both abstractions and implementations should be extensible via subclassing
- Changes in implementation should not affect client code
- You have a class explosion due to a proliferation of classes varying in two or more dimensions
- You want to share an implementation among multiple objects (possibly with reference counting)

### Structure

```
  +-------------------+                  +-------------------+
  |   Abstraction     |                  |  Implementor      |
  +-------------------+  has-a           +-------------------+
  | - impl: Impl*     |---------------->| + operationImpl() |
  | + operation()     |                  +-------------------+
  |   { impl->        |                        ^       ^
  |     operationImpl()}                       |       |
  +-------------------+                  +-----+--+ +--+-------+
       ^        ^                        |ConcreteA| |ConcreteB |
       |        |                        |Impl     | |Impl      |
  +----+---+ +--+------+                +----------+ +----------+
  |Refined | |Refined  |
  |Abstr A | |Abstr B  |
  +---------+ +---------+
```

### Participants
- **Abstraction** - defines the abstraction's interface; maintains a reference to an object of type Implementor
- **RefinedAbstraction** - extends the interface defined by Abstraction
- **Implementor** - defines the interface for implementation classes (can be different from Abstraction's interface)
- **ConcreteImplementor** - implements the Implementor interface

### Key Insight: Abstraction vs Implementation
The "abstraction" is not an abstract class in the C++ sense. It is the high-level control layer that delegates work to the "implementation" layer:

- **Abstraction** = what the client sees (the "what")
- **Implementation** = how it is actually done (the "how")
- **Bridge** = the composition link between them

Example: `Window` (abstraction) and `WindowImpl` (implementation - XWindow, PMWindow). The Window knows WHAT to do (draw shapes, handle resize). The WindowImpl knows HOW to do it on a specific platform.

### Consequences

**Benefits:**
1. Decouples interface from implementation - can change independently
2. Eliminates class explosion from multi-dimensional variation
3. Improved extensibility - extend abstraction and implementation independently
4. Implementation can be changed at runtime (dependency injection)
5. Hides implementation details from clients

**Liabilities:**
1. Increased complexity - more classes and indirection
2. Must identify the two dimensions of variation upfront at design time
3. Performance overhead from double dispatch (though usually negligible)

### Implementation Notes (C++ Specific)
- The implementation object is typically passed via constructor (dependency injection)
- Use `std::unique_ptr<Implementor>` if abstraction owns its implementation exclusively
- Use `std::shared_ptr<Implementor>` if implementation can be shared across abstractions
- Consider `std::function` as a lightweight bridge for single-operation implementations
- Pimpl idiom is a compile-time bridge (hides implementation behind a pointer)

### Known Uses
- **Device Drivers**: OS kernel (abstraction) and hardware-specific drivers (implementation)
- **Pimpl Idiom in C++**: header exposes interface, `.cpp` file contains implementation (compilation firewall)
- **JDBC**: `Connection` (abstraction) and vendor-specific driver (implementation)
- **Cross-platform frameworks**: Qt uses bridge internally for platform-specific widget implementations
- **Graphics APIs**: `Shape` abstraction bridging to `DirectX`/`OpenGL`/`Vulkan` renderers

### Common Misconceptions
1. **"Bridge is the same as Adapter"** - Adapter fixes incompatibility after design; Bridge prevents incompatibility by separating concerns during design.
2. **"Bridge is just Strategy"** - Bridge separates abstraction levels (one calls the other); Strategy provides interchangeable algorithms at the same level. Bridge has refined abstractions; Strategy typically does not.
3. **"You always need Bridge"** - If you only have one dimension of variation, Bridge adds unnecessary complexity. Use it when you observe N * M class proliferation.

### Related Patterns
- **Abstract Factory**: can create and configure a Bridge (provide the right implementation)
- **Adapter**: works with existing incompatible interfaces; Bridge is designed up-front
- **Strategy**: similar structure but different intent; Strategy encapsulates algorithms; Bridge separates abstraction levels

---

## Flyweight Pattern

### Intent (GoF)
Use sharing to support large numbers of fine-grained objects efficiently.

### Real-World Analogy
Think of the letters in a printed book. A typical novel has around 500,000 characters, but there are only about 70 unique characters (letters, digits, punctuation). Instead of creating 500,000 independent character objects (each storing font, size, color, character data), you create 70 shared flyweight objects (one per unique character). Each occurrence in the text stores only its position (extrinsic state) and references the shared character definition (intrinsic state).

Another analogy: a forest in a video game. There might be 10,000 trees on screen, but only 5 unique tree models (oak, pine, birch, maple, spruce). Each rendered tree instance shares the same model/texture (intrinsic) but has its own position, rotation, and scale (extrinsic).

### Motivation: Memory Optimization

Without Flyweight:
```
  500,000 characters x (4 bytes char + 48 bytes font info + 8 bytes color)
  = 500,000 x 60 bytes = 30,000,000 bytes (30 MB)
```

With Flyweight:
```
  70 shared flyweights x 60 bytes = 4,200 bytes (flyweight pool)
  + 500,000 references x 12 bytes (position + flyweight pointer)
  = 6,004,200 bytes (~6 MB)
  
  SAVINGS: 30 MB -> 6 MB = 80% reduction!
```

The key insight is separating what is SHARED (intrinsic state) from what is UNIQUE per instance (extrinsic state).

### Intrinsic vs Extrinsic State

| Intrinsic State | Extrinsic State |
|----------------|-----------------|
| Stored IN the flyweight | Stored OUTSIDE (passed by client) |
| Shared across all contexts | Unique per context |
| Immutable (never changes) | Can change per use |
| Character code, font metrics | Position in document, selection state |
| Tree mesh, texture | Position, rotation, wind offset |
| Particle sprite, base color | Velocity, current position, lifetime |

```
  +------------------+          +-------------------+
  |  FlyweightPool   |          |  Client Context   |
  +------------------+          +-------------------+
  | "A" -> GlyphA    |          | pos: (10, 20)     |
  | "B" -> GlyphB    |          | flyweight: "A" ---+---> GlyphA (shared)
  | "C" -> GlyphC    |          +-------------------+
  | ...              |          | pos: (20, 20)     |
  +------------------+          | flyweight: "B" ---+---> GlyphB (shared)
                                +-------------------+
                                | pos: (30, 20)     |
                                | flyweight: "A" ---+---> GlyphA (same instance!)
                                +-------------------+
```

### Applicability
Use the Flyweight pattern when ALL of the following are true:
- An application uses a large number of objects
- Storage costs are high because of the sheer quantity of objects
- Most object state can be made extrinsic (moved outside the object)
- Many groups of objects may be replaced by relatively few shared objects once extrinsic state is removed
- The application does not depend on object identity (flyweights are shared, so identity checks are meaningless)

### Structure

```
  +------------------+              +-------------------+
  |     Client       |              |  FlyweightFactory |
  +------------------+              +-------------------+
  | uses flyweights  |------------->| + getFlyweight(key)|
  | passes extrinsic |              |   { if exists,    |
  | state            |              |     return it;    |
  +------------------+              |     else create,  |
                                    |     store, return}|
                                    +-------------------+
                                           |
                                           | manages pool of
                                           v
                                    +-------------------+
                                    |    Flyweight      |
                                    +-------------------+
                                    | - intrinsicState  |
                                    | + operation(      |
                                    |   extrinsicState) |
                                    +-------------------+
                                       ^          ^
                                       |          |
                              +--------+--+  +----+-------+
                              |Concrete   |  |Unshared    |
                              |Flyweight  |  |Flyweight   |
                              +-----------+  +------------+
```

### Participants
- **Flyweight** - declares an interface through which flyweights can receive and act on extrinsic state
- **ConcreteFlyweight** - implements the Flyweight interface and stores intrinsic state; must be shareable
- **UnsharedConcreteFlyweight** - not all Flyweight subclasses need be shared; the pattern allows unshared instances when needed
- **FlyweightFactory** - creates and manages flyweight objects; ensures proper sharing (returns existing instances)
- **Client** - maintains references to flyweights; computes or stores extrinsic state

### Memory Optimization Math

To determine if Flyweight is worthwhile, calculate:

```
  WITHOUT Flyweight:
    memory = N * (intrinsic_size + extrinsic_size)
  
  WITH Flyweight:
    memory = U * intrinsic_size + N * (extrinsic_size + pointer_size)
  
  Where:
    N = total number of objects
    U = number of unique objects (U << N for flyweight to help)
    pointer_size = 8 bytes on 64-bit systems
  
  SAVINGS = N * intrinsic_size - U * intrinsic_size - N * pointer_size
          = (N - U) * intrinsic_size - N * pointer_size
  
  Flyweight is worth it when:
    (N - U) * intrinsic_size > N * pointer_size
    i.e., when there are many duplicates and intrinsic state is large
```

### Consequences

**Benefits:**
1. Substantial memory savings when the number of instances is very large
2. Processing savings if extrinsic state is computed rather than stored
3. Reduced object creation time (reuse existing instances)

**Liabilities:**
1. Increased code complexity (separating intrinsic/extrinsic state)
2. Runtime cost of computing/transferring extrinsic state
3. Flyweights cannot be used with object identity (shared objects are same instance)
4. Thread-safety concerns if flyweights are mutable (they should be immutable!)

### Implementation Notes (C++ Specific)
- Use `std::unordered_map<Key, std::shared_ptr<Flyweight>>` for the flyweight pool
- Flyweight objects MUST be immutable (const member variables, no setters)
- Consider `std::string_view` as a lightweight flyweight for string data
- String interning (`std::string` implementations may share buffers for short strings)
- Use `static` factory methods to control instantiation
- In game engines, use flyweight for particle systems, bullet patterns, terrain tiles

### Known Uses
- **Java String Pool**: `String.intern()` ensures identical strings share the same memory
- **Text Editors**: Characters share glyph definitions (font, metrics)
- **Game Engines**: Particle systems share sprite/behavior, each instance has unique position/velocity
- **Python Integer Cache**: Small integers (-5 to 256) are pre-created and shared
- **Database Connection Pools**: Connections are shared flyweights reused across requests
- **CSS in Browsers**: Computed style objects are shared across DOM elements with identical styles

### Common Misconceptions
1. **"Flyweight is just caching"** - Caching stores computed results. Flyweight shares object structure. The flyweight pool never evicts objects; a cache does.
2. **"All objects must be flyweights"** - The pattern allows UnsharedConcreteFlyweight for objects that do not benefit from sharing.
3. **"Flyweight means singleton"** - Singleton has ONE instance total. Flyweight has one instance PER unique key. A flyweight pool contains many objects.

### Related Patterns
- **Composite**: often combined with Flyweight (share leaf nodes in a composite tree)
- **State/Strategy**: can be implemented as flyweights (strategy objects are often stateless and shareable)
- **Singleton**: extreme case of sharing (one instance vs many shared instances)

---

## Pattern Comparison Matrix

| Aspect | Composite | Bridge | Flyweight |
|--------|-----------|--------|-----------|
| **Core Problem** | Part-whole hierarchies | Multi-dimensional variation | Too many similar objects |
| **Key Mechanism** | Recursive tree composition | Composition over inheritance | Object sharing via pool |
| **State** | Each node has its own state | Each side has its own state | Split into intrinsic/extrinsic |
| **New types** | Add new leaf/composite types | Add to either dimension independently | Add new flyweight keys |
| **Performance** | Recursive traversal overhead | Extra indirection | Memory savings, lookup overhead |
| **Typical Scale** | Dozens to thousands of nodes | 2 hierarchies with N and M types | Millions of instances |
| **Identity** | Each node is unique | Each combination is unique | Shared objects lose identity |

### Bridge vs Strategy - Deep Comparison

| Aspect | Bridge | Strategy |
|--------|--------|----------|
| **Level** | Separates abstraction layers | Same abstraction level |
| **Hierarchy** | Has refined abstractions AND concrete implementations | Only concrete strategies |
| **Lifetime** | Implementation typically lives as long as abstraction | Strategy can be swapped frequently |
| **Purpose** | Prevent class explosion from multi-dimensional variation | Allow algorithm selection at runtime |
| **Example** | Shape x Renderer (structural) | Sort using QuickSort or MergeSort (behavioral) |

---

## Code Examples

### Files
- [composite_filesystem.cpp](composite_filesystem.cpp) - File system with Files and Directories demonstrating recursive size computation
- [bridge_renderer.cpp](bridge_renderer.cpp) - Shape x Renderer bridge avoiding class explosion
- [flyweight_text.cpp](flyweight_text.cpp) - Text editor sharing character glyphs for memory optimization

---

## Interview Questions

1. **Composite vs Decorator** - both have recursive structure. What differs?
   - Composite represents part-whole hierarchies (collection of children). Decorator wraps a single component to add behavior. Composite fans out (1-to-many); Decorator chains (1-to-1).

2. **Bridge vs Adapter** - both connect two interfaces.
   - Adapter fixes incompatibility after the fact. Bridge prevents it by separating abstraction from implementation during design. Adapter is reactive; Bridge is proactive.

3. **Bridge vs Strategy** - both use composition and polymorphism. What differs?
   - Bridge separates abstraction levels (Shape uses Renderer). Strategy provides interchangeable algorithms at the same level (Context uses Algorithm). Bridge has a hierarchy on both sides; Strategy typically only on the implementation side.

4. **How does Flyweight reduce memory? Explain intrinsic vs extrinsic state.**
   - Intrinsic state is shared (immutable, context-independent). Extrinsic state is unique per use (passed in by client). By sharing intrinsic state across thousands of instances, you store it only once.

5. **Real-world: Design a text editor's rendering using Flyweight.**
   - Each unique character (glyph) is a flyweight storing font metrics and bitmap. Each position in the document stores only row/column (extrinsic) and a reference to the shared glyph.

6. **Safety vs Transparency in Composite - which do you prefer and why?**
   - In C++, safety (child operations only on Composite) is preferred because the type system catches errors at compile time. Use `dynamic_cast` or the Visitor pattern when you need to distinguish.

7. **When is Flyweight NOT appropriate?**
   - When objects have mostly unique state (little sharing possible), when the number of instances is small, or when object identity matters.

---

## Daily Assignment

1. **Composite**: Build `FileSystemNode` with `File` (leaf) and `Directory` (composite). Implement `size()` that computes total size recursively. Add `ls()` that prints the tree with indentation.

2. **Bridge**: Implement `Shape` (Circle, Square, Triangle) x `Renderer` (VectorRenderer, RasterRenderer, SVGRenderer). Demonstrate that adding a new shape or renderer requires only ONE new class.

3. **Flyweight**: Build a text editor where 1,000,000 characters share approximately 50 unique glyphs (font, size, color). Print memory usage comparison: with flyweight vs without.

4. **Combined Challenge**: Build a UI widget tree (Composite) where leaf widgets use Flyweight for shared icon/style data, and the rendering is pluggable via Bridge (Console vs GUI renderer).
