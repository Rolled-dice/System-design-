# Day 14 - Mediator + Visitor + Memento Patterns

## Table of Contents
- [Mediator Pattern](#mediator-pattern)
- [Visitor Pattern](#visitor-pattern)
- [Memento Pattern](#memento-pattern)
- [Pattern Comparison Matrix](#pattern-comparison-matrix)
- [Code Examples](#code-examples)
- [Interview Questions](#interview-questions)
- [Daily Assignment](#daily-assignment)

---

## Mediator Pattern

### Intent (GoF)
Define an object that encapsulates how a set of objects interact. Mediator promotes loose coupling by keeping objects from referring to each other explicitly, and it lets you vary their interaction independently.

### Also Known As
- Controller

### Real-World Analogy
Think of an air traffic control tower (ATC). Aircraft do not communicate directly with each other to coordinate landing and takeoff. Instead, all pilots communicate with the control tower (Mediator). The tower knows the positions of all planes and issues instructions. No pilot needs to know about all other pilots; they only talk to the tower. This prevents the O(n^2) communication nightmare of every plane talking to every other plane.

Another analogy: a chat room. Users send messages to the chat room (Mediator), not directly to each other. The chat room decides who receives each message (broadcast, private, group). Users are decoupled from each other.

### Motivation
Consider a dialog box with various UI components: text fields, buttons, checkboxes, and lists. Components have complex dependencies:
- When a checkbox is checked, enable a text field
- When a list item is selected, update a label
- When a button is clicked, validate all fields

Without Mediator, each component references and calls methods on other components directly. This creates a tangled web of N*(N-1)/2 connections. Adding or removing a component requires modifying many other components.

With Mediator (the `DialogBox` class), components notify the mediator of events. The mediator implements all coordination logic in one place.

### Applicability
Use the Mediator pattern when:
- A set of objects communicate in well-defined but complex ways (spaghetti dependencies)
- Reusing an object is difficult because it refers to and communicates with many other objects
- Behavior distributed between several classes should be customizable without subclassing all of them
- You want to centralize control logic that was previously scattered

### Structure

```
  +-------------------+              +-------------------+
  |    Mediator       |<------------>|    Colleague      |
  +-------------------+              +-------------------+
  | + notify(sender,  |              | - mediator: Med*  |
  |   event)          |              | + setMediator()   |
  +-------------------+              +-------------------+
         ^                                ^       ^
         |                                |       |
  +-------------------+            +------+--+ +--+--------+
  | ConcreteMediator  |            |ColleagueA| |ColleagueB|
  +-------------------+            +----------+ +----------+
  | - colleagueA      |            | +changed(){| +doSomething()
  | - colleagueB      |            |   mediator | 
  | - colleagueC      |            |   ->notify |
  | + notify(sender,  |            |   (this,   |
  |   event) {        |            |   "event")}|
  |   if sender==A:   |            +----------+ +----------+
  |     B.doSomething()|
  |     C.doOther()   |
  | }                 |
  +-------------------+
```

### Communication Flow

```
  ColleagueA       Mediator       ColleagueB       ColleagueC
      |               |               |               |
      | changed()     |               |               |
      |----+          |               |               |
      |    |          |               |               |
      | notify(A,"ev")|               |               |
      |-------------->|               |               |
      |               |               |               |
      |               | doSomething() |               |
      |               |-------------->|               |
      |               |               |               |
      |               | doOther()     |               |
      |               |------------------------------>|
      |               |               |               |
      |               |<------------------------------|
      |               |    result     |               |
      |<--------------|               |               |
      |   response    |               |               |
```

### Consequences

**Benefits:**
1. Limits subclassing - behavior changes only require changing the mediator, not the colleagues
2. Decouples colleagues - colleagues can be reused independently since they do not reference each other
3. Simplifies object protocols - replaces many-to-many with one-to-many relationships
4. Abstracts cooperation - mediator encapsulates how objects cooperate, making it easier to understand

**Liabilities:**
1. Centralizes control - mediator can become a "god object" (monolithic, complex)
2. Mediator complexity grows with the number of colleagues and interactions
3. Can be hard to maintain if the mediator handles too many cross-cutting interactions
4. Single point of failure - if the mediator breaks, all communication stops

### Implementation Notes (C++ Specific)
- Colleagues can store a `std::weak_ptr<Mediator>` to avoid circular ownership
- Use event types (enum or string) for flexible notification
- Consider using `std::function` callbacks instead of a formal Mediator interface
- For complex mediators, consider using the Observer pattern internally
- The mediator often has a `std::vector` or `std::map` of registered colleagues

### Known Uses
- **Air Traffic Control**: Classic example of centralizing complex coordination
- **Chat Applications**: Chat rooms mediate between users
- **UI Dialog Boxes**: MFC's `CDialog`, Qt's signal/slot system works as implicit mediator
- **Message Brokers**: RabbitMQ, Kafka mediate between producers and consumers
- **Game Event Systems**: Central event manager dispatching events between game entities
- **Microservice Orchestrators**: Saga pattern uses an orchestrator (mediator) to coordinate service calls

### Common Misconceptions
1. **"Mediator and Facade are the same"** - Facade provides a simple unidirectional interface to a subsystem (clients call facade, subsystem does not call facade back). Mediator enables bidirectional communication between colleagues.
2. **"Mediator and Observer are the same"** - Mediator centralizes communication logic. Observer distributes it (subjects notify observers, but observers decide what to do). Mediator KNOWS the coordination logic; in Observer, the subject does not know what observers will do.
3. **"Every centralized communication is Mediator"** - If the central object just routes messages without coordination logic, it is more of a message bus (Observer variant).

### Related Patterns
- **Facade**: organizes communication in one direction (client -> subsystem); Mediator handles bidirectional
- **Observer**: distributed notification; Mediator can use Observer internally
- **Command**: mediator can use commands to parameterize colleague requests

---

## Visitor Pattern

### Intent (GoF)
Represent an operation to be performed on the elements of an object structure. Visitor lets you define a new operation without changing the classes of the elements on which it operates.

### Also Known As
- Double Dispatch

### Real-World Analogy
Think of a tax inspector visiting different types of businesses: restaurants, tech companies, retail stores. Each business type requires a different audit procedure. The inspector (Visitor) has specialized knowledge for each business type. The businesses (Elements) do not contain tax audit logic themselves; they just "accept" the inspector and let them do their job. If tax laws change, you update the inspector (Visitor), not the businesses.

Another analogy: a document exporter. You have a document with paragraphs, images, tables, and code blocks (Elements). You want to export to PDF, HTML, and Markdown (operations). Instead of each element knowing how to render in every format, you create a Visitor for each format. Adding a new export format means adding one new Visitor, not modifying all element classes.

### Motivation: The Expression Problem
You have a hierarchy of AST (Abstract Syntax Tree) nodes: `NumberExpr`, `AddExpr`, `MultiplyExpr`. You need operations: `evaluate()`, `prettyPrint()`, `typeCheck()`, `optimize()`.

**Without Visitor**: Add virtual methods to each node class. Adding a new operation means modifying ALL element classes.
**With Visitor**: Each operation is a separate Visitor class. Adding a new operation means adding one class. But adding a new element type means modifying all visitors.

Visitor is ideal when: element types are STABLE but operations CHANGE frequently.

### Double Dispatch Mechanism (Step-by-Step)

Most OOP languages support single dispatch (method resolution based on the receiver's type). Visitor achieves double dispatch (resolution based on BOTH the element type AND the visitor type):

```
  Step 1: Client calls element->accept(visitor)
           -> dispatches on element type (single dispatch)
  
  Step 2: Inside accept(), element calls visitor->visitConcreteElement(this)
           -> dispatches on visitor type (second dispatch)
  
  Result: The correct method is called based on BOTH element type AND visitor type.
```

```cpp
// Step 1: Dispatch on element type
void NumberExpr::accept(Visitor& v) {
    v.visitNumber(*this);  // Step 2: Dispatch on visitor type
}

void AddExpr::accept(Visitor& v) {
    v.visitAdd(*this);     // Step 2: Dispatch on visitor type
}

// Result: EvalVisitor::visitNumber vs PrintVisitor::visitAdd
// is selected based on BOTH the element (Number/Add) AND the visitor (Eval/Print)
```

### Structure

```
  +-------------------+              +-------------------+
  |    Element        |              |     Visitor       |
  +-------------------+              +-------------------+
  | + accept(Visitor) |              | +visitConcreteA() |
  +-------------------+              | +visitConcreteB() |
       ^         ^                   | +visitConcreteC() |
       |         |                   +-------------------+
  +----+---+ +---+----+                  ^         ^
  |ConcreteA| |ConcreteB|                |         |
  +---------+ +---------+          +----+---+ +---+----+
  |+accept(v){| |+accept(v){       |Visitor1| |Visitor2|
  |  v.visitA |  | v.visitB        +---------+ +---------+
  |  (*this); |  | (*this);        |visitA(){ | |visitA(){
  | }         |  |}                | // op1 on| | // op2 on
  +---------+ +---------+         | A       }| | A       }
                                   |visitB(){ | |visitB(){
                                   | // op1 on| | // op2 on
                                   | B       }| | B       }
                                   +---------+ +---------+
```

### Participants
- **Visitor** - declares a Visit operation for each class of ConcreteElement in the object structure
- **ConcreteVisitor** - implements each operation declared by Visitor (one per element type)
- **Element** - defines an Accept operation that takes a visitor as argument
- **ConcreteElement** - implements Accept by calling the appropriate visitor method
- **ObjectStructure** - can enumerate its elements; may provide a high-level interface to allow the visitor to visit its elements

### When to Use Visitor

**Good fit (use Visitor):**
- Element hierarchy is STABLE (rarely add new element types)
- Operations on elements CHANGE frequently (new operations added often)
- Elements are diverse and operations differ significantly per element type
- You cannot or should not modify element classes

**Bad fit (avoid Visitor):**
- Element hierarchy changes frequently (adding elements means updating ALL visitors)
- Operations are simple and uniform across element types (polymorphism suffices)
- Few operations are needed and unlikely to grow

### Consequences

**Benefits:**
1. Easy to add new operations - just add a new Visitor class
2. Related operations gathered together in a Visitor (Single Responsibility for operations)
3. Visitor can accumulate state as it visits elements (running totals, collected data)
4. Can visit elements across different class hierarchies (not limited to a single inheritance tree)

**Liabilities:**
1. Adding new ConcreteElement is hard - must update ALL visitor classes
2. Breaks encapsulation - visitors often need access to element internals (friend class or public getters)
3. Double dispatch can be confusing to developers unfamiliar with the pattern
4. Requires element hierarchy to be known upfront

### Implementation Notes (C++ Specific)
- Use forward declarations to break circular dependencies (Visitor declares visit methods for elements)
- Consider `std::variant` + `std::visit` as a modern alternative to classic Visitor (C++17)
- `std::visit` with `overloaded` pattern provides type-safe visitation without virtual:
  ```cpp
  std::visit(overloaded{
      [](const Number& n) { /* handle number */ },
      [](const Add& a) { /* handle add */ },
  }, node);
  ```
- Use `friend` access or public getters for visitor to access element internals
- Consider the Acyclic Visitor variant to avoid updating all visitors when adding elements

### Known Uses
- **Compiler AST Traversal**: Type checking, code generation, optimization passes are all visitors
- **Document Processing**: Export to PDF/HTML/LaTeX as separate visitors on document elements
- **Syntax Highlighting**: Editor visits token types and applies formatting
- **Static Analysis Tools**: Each analysis rule is a visitor over the code AST
- **LLVM**: Uses visitor pattern extensively for IR transformations
- **Serialization**: Visiting object graphs to produce JSON/XML/binary output

### Common Misconceptions
1. **"Visitor is just iteration"** - Iterator provides sequential access. Visitor provides type-specific operations. Visitor does double dispatch; Iterator does not.
2. **"Visitor violates Open/Closed"** - For elements, yes (adding elements requires modifying visitors). For operations, no (adding operations only requires adding visitors). It trades one axis of extensibility for another.
3. **"Visitor requires inheritance"** - In modern C++ (C++17), `std::variant` + `std::visit` provides a non-inheritance Visitor mechanism.

### Related Patterns
- **Composite**: Visitor is often applied across Composite structures
- **Iterator**: used to traverse elements before visiting them
- **Interpreter**: grammar tree can be visited for evaluation

---

## Memento Pattern

### Intent (GoF)
Without violating encapsulation, capture and externalize an object's internal state so that the object can be restored to this state later.

### Also Known As
- Token
- Snapshot

### Real-World Analogy
Think of a save point in a video game. At any moment, you can "save" your game state (position, health, inventory, quest progress) to a save file. If you die, you reload from the save point and the entire game state is restored exactly as it was. The save file (Memento) captures everything without the game needing to expose its internal structure to the save/load system.

Another analogy: the undo feature in a text editor. Before each edit, the editor captures a snapshot of the document state. If you press Ctrl+Z, it restores the previous snapshot. The snapshot (Memento) contains the full document state but the undo system does not need to understand the editor's internals.

### Motivation
Consider a constraint solver in a graphics editor that moves objects to satisfy constraints. Sometimes the solver must try a solution, discover it does not work, and backtrack to a previous state. The solver needs to capture object state before each attempt.

The problem: state is private (encapsulation). The solver should not be granted access to internals. Solution: the object itself creates a memento (opaque snapshot) that the solver can store and later pass back to restore state.

### The Three Roles

```
  +-----------+        +-----------+        +-----------+
  | Caretaker |        | Originator|        |  Memento  |
  +-----------+        +-----------+        +-----------+
  | Requests   |------>| Creates    |------->| Stores    |
  | save/restore|      | mementos   |       | state     |
  | Stores     |       | Restores   |       | (opaque   |
  | mementos   |       | from       |       |  to care- |
  | (without   |       | mementos   |       |  taker)   |
  |  reading)  |       +-----------+        +-----------+
  +-----------+

  Key insight: Caretaker holds mementos but CANNOT read or modify them.
  Only the Originator can create and restore from mementos.
```

### Structure

```
  +-------------------+        +-------------------+
  |    Caretaker      |        |    Originator     |
  +-------------------+        +-------------------+
  | - mementos[]      |        | - state           |
  | + save()          |<------>| + createMemento() |
  | + undo()          |        |   { return new    |
  +-------------------+        |     Memento(state)}|
         |                     | + restore(memento)|
         | stores              |   { state =       |
         v                     |     memento.state}|
  +-------------------+        +-------------------+
  |     Memento       |
  +-------------------+
  | - state (private) |  <- only Originator can access
  | + getState()      |  <- wide interface (for Originator)
  +-------------------+
  Note: Caretaker sees only narrow interface (store/return)
```

### Participants
- **Memento** - stores internal state of the Originator; protects against access by objects other than the originator. Has two interfaces:
  - Wide interface (for Originator): full access to state
  - Narrow interface (for Caretaker): can only pass memento around, not inspect it
- **Originator** - creates a memento containing a snapshot of its current internal state; uses the memento to restore its internal state
- **Caretaker** - responsible for the memento's safekeeping; never operates on or examines the contents of a memento

### Preserving Encapsulation
The key challenge is letting the Originator access Memento internals while preventing the Caretaker from doing so:

**C++ approach: nested class + friend**
```cpp
class Editor {  // Originator
public:
    class Snapshot {  // Memento (nested class)
        friend class Editor;  // Only Editor can access internals
    private:
        string text;
        int cursorPos;
        Snapshot(string t, int c) : text(t), cursorPos(c) {}
    };
    
    Snapshot createSnapshot() {
        return Snapshot(text, cursorPos);  // Editor can access private ctor
    }
    
    void restore(const Snapshot& s) {
        text = s.text;          // Editor can access private members
        cursorPos = s.cursorPos;
    }
    
private:
    string text;
    int cursorPos;
};

// Caretaker only holds Snapshot objects - cannot read text or cursorPos
class UndoManager {  // Caretaker
    stack<Editor::Snapshot> history;
public:
    void save(Editor::Snapshot s) { history.push(move(s)); }
    Editor::Snapshot getLast() { auto s = history.top(); history.pop(); return s; }
};
```

### Undo System with Memento + Command

Memento and Command patterns work together beautifully for undo:

```
  1. Before executing a Command, save Originator state to Memento
  2. Execute the Command (modifies Originator)
  3. Store the (Command, Memento) pair in history
  4. On undo: restore Originator from stored Memento
  
  +---+---+---+---+---+---+
  |Cmd|Mem|Cmd|Mem|Cmd|Mem|  <- History stack (command + saved state pairs)
  +---+---+---+---+---+---+
            ^
            | current position
```

### Consequences

**Benefits:**
1. Preserves encapsulation boundaries - state capture/restore without exposing internals
2. Simplifies Originator - Originator does not need to maintain multiple versions of state
3. Enables undo/redo, transaction rollback, save/load functionality
4. Memento objects are simple data holders - easy to serialize for persistent storage

**Liabilities:**
1. Might be expensive - if Originator state is large, creating frequent mementos consumes memory
2. Defining narrow and wide interfaces can be tricky in some languages (C++ uses friend)
3. Caretaker must manage memento lifecycle (prevent memory leaks from unlimited history)
4. Hidden costs - clients may not realize how expensive memento creation is

### Memory Management for Mementos
Since mementos can be expensive, consider:
- **Limited history**: Keep only the last N mementos (circular buffer)
- **Incremental mementos**: Store only the diff/delta from previous state (like git)
- **Compression**: Compress state before storing in memento
- **Copy-on-write**: Share state until modification (lazy copying)
- **Periodic full + incremental**: Full snapshot every N operations, deltas in between

### Implementation Notes (C++ Specific)
- Use nested class + `friend` for encapsulation (wide interface for Originator only)
- Use `std::unique_ptr<Memento>` or value semantics for memento storage
- Consider `std::deque` for bounded history (efficient front removal)
- For large state: use copy-on-write with `std::shared_ptr` for shared data
- Implement move semantics for efficient memento transfer
- For serializable mementos: consider `std::stringstream` or protobuf

### Known Uses
- **Text Editor Undo**: Every editor uses mementos (document state snapshots)
- **Database Transactions**: SAVEPOINT creates a memento; ROLLBACK TO restores it
- **Game Save Systems**: Serialized game state = persistent memento
- **Browser History**: Each page state is a memento (for back/forward navigation)
- **Version Control**: Each commit is a memento of the entire project state
- **std::filesystem::copy_options::skip_existing**: backup files as mementos before overwrite
- **Serialization Libraries**: Boost.Serialization, cereal capture object state as mementos

### Common Misconceptions
1. **"Memento is just serialization"** - Serialization is a mechanism. Memento is a pattern about responsibilities (who creates, who stores, who restores). You CAN use serialization to implement Memento.
2. **"Caretaker can read the memento"** - No! The caretaker treats it as an opaque token. This is essential for encapsulation. If the caretaker inspects memento content, the pattern is violated.
3. **"Memento always stores the complete state"** - Incremental mementos (storing only what changed) are valid and often preferred for large objects.

### Related Patterns
- **Command**: often uses Memento to store state needed for undo
- **Iterator**: can use Memento to capture iteration state (bookmarks)
- **Prototype**: alternative approach; clone the entire object instead of extracting state into a separate memento

---

## Pattern Comparison Matrix

| Aspect | Mediator | Visitor | Memento |
|--------|----------|---------|---------|
| **Core Problem** | Complex inter-object communication | Adding operations to stable hierarchies | State capture/restore |
| **Key Mechanism** | Centralized coordination | Double dispatch | Opaque state snapshot |
| **Coupling** | Colleagues -> Mediator only | Visitor knows all elements | Originator <-> Memento (encapsulated) |
| **Adding new...** | Easy to add colleagues | Easy to add visitors (operations) | N/A |
| **Weakness** | Mediator becomes god object | Hard to add new element types | Memory cost |
| **Encapsulation** | Hides colleague interactions | May break element encapsulation | PRESERVES encapsulation |

### Mediator vs Observer vs Event Bus

| Aspect | Mediator | Observer | Event Bus |
|--------|----------|----------|-----------|
| **Communication** | Bidirectional, orchestrated | Unidirectional (subject->observers) | Bidirectional via broker |
| **Intelligence** | Mediator contains coordination logic | Subject has no logic about what observers do | Bus just routes, no logic |
| **Coupling** | Colleagues know mediator | Observers know subject (or at least subscribe) | Publishers and subscribers decoupled |
| **Topology** | Star (mediator in center) | Star (subject in center) | Star (bus in center) |
| **Use case** | Complex form validation | Simple notification | System-wide event distribution |

---

## Code Examples

### Files
- [mediator_chat.cpp](mediator_chat.cpp) - Chat room mediator with private and group messaging
- [visitor_ast.cpp](visitor_ast.cpp) - AST visitor with evaluation and printing operations
- [memento_editor.cpp](memento_editor.cpp) - Text editor with snapshot-based undo

---

## Interview Questions

1. **Mediator vs Observer** - both handle object communication. What differs?
   - Mediator CENTRALIZES complex coordination logic in one place (mediator decides what happens). Observer DISTRIBUTES notification (subject broadcasts, observers decide individually what to do). Mediator is bidirectional; Observer is typically unidirectional.

2. **Mediator vs Facade** - both sit in front of multiple objects.
   - Facade provides a SIMPLE unidirectional interface to a subsystem (client->facade->subsystem). Mediator enables COMPLEX bidirectional communication between colleagues (colleague<->mediator<->colleague). Subsystem does not know about Facade; colleagues know about Mediator.

3. **Visitor vs Iterator** - both traverse structures.
   - Iterator provides sequential access (focuses on traversal order and position). Visitor applies type-specific operations (focuses on what to do with each element type). Iterator is simple; Visitor uses double dispatch for type resolution.

4. **Explain Visitor double dispatch step-by-step.**
   - First dispatch: `element->accept(visitor)` resolves based on element's dynamic type. Second dispatch: `visitor->visitX(*this)` resolves based on visitor's dynamic type. Combined: the correct operation for both the element type AND the visitor type is selected.

5. **Memento + Command - how do they combine for undo/redo?**
   - Before Command executes, capture Originator state in a Memento. Store the Memento with the Command in history. On undo: restore Originator from the stored Memento. On redo: re-execute the Command.

6. **How does Memento preserve encapsulation?**
   - The Originator creates and reads the Memento (wide interface via friendship). The Caretaker only stores and passes Mementos (narrow interface - cannot read contents). State is captured without exposing internals to external objects.

7. **When is Visitor a bad choice?**
   - When the element hierarchy changes frequently (every new element requires updating ALL visitors). When operations are uniform across types (regular polymorphism suffices). When the number of element types is very large.

8. **What is the "god object" risk with Mediator?**
   - As more colleagues and interactions are added, the mediator grows increasingly complex. It can become a bloated class that knows too much. Solution: decompose into multiple mediators by interaction group, or use event-based approaches.

---

## Daily Assignment

1. **Mediator - Chat Room**: Build a chat system with `ChatRoom` (mediator) and `User` (colleagues). Support: broadcast messages, private messages (user to user), and group messages (user to group). Users should never reference each other directly.

2. **Visitor - AST**: Create an expression tree with `NumberNode`, `AddNode`, `MultiplyNode`. Implement `EvalVisitor` (computes result), `PrintVisitor` (infix notation), and `PostfixVisitor` (postfix/RPN notation). Demonstrate that adding a new operation requires only a new visitor class.

3. **Memento - Text Editor**: Build an `Editor` with text content and cursor position. Implement `Snapshot` (memento) and `UndoManager` (caretaker) supporting unlimited undo and bounded redo (last 10 operations). The UndoManager must NOT access Snapshot internals.

4. **Combined Challenge**: Build a drawing application where shapes (elements) can be visited by `DrawVisitor`, `ResizeVisitor`, and `ExportVisitor`. Use Memento to snapshot the canvas state before each operation, enabling undo. Use Mediator to coordinate between the toolbar, canvas, and property panel.
