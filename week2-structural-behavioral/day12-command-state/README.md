# Day 12 - Command + State Patterns

## Table of Contents
- [Command Pattern](#command-pattern)
- [State Pattern](#state-pattern)
- [Command vs Strategy Comparison](#command-vs-strategy-comparison)
- [Code Examples](#code-examples)
- [Interview Questions](#interview-questions)
- [Daily Assignment](#daily-assignment)

---

## Command Pattern

### Intent (GoF)
Encapsulate a request as an object, thereby letting you parameterize clients with different requests, queue or log requests, and support undoable operations.

### Also Known As
- Action
- Transaction

### Real-World Analogy
Think of ordering at a restaurant. You (Client) tell the Waiter (Invoker) what you want. The Waiter writes it on an order slip (Command object) and places it in the kitchen queue. The Cook (Receiver) eventually picks up the order and prepares the food. The order slip encapsulates everything needed: what dish, how to cook it, any modifications. The Waiter does not cook; the Cook does not interact with you directly. And if you change your mind before cooking starts, you can cancel the order (undo).

Another analogy: a universal remote control. Each button press creates a command object. The remote does not know what the TV, stereo, or lights actually do; it just invokes the command. You can program any button to do anything.

### Motivation
Consider a GUI toolkit with buttons, menu items, and keyboard shortcuts. A "Save" action might be triggered from a toolbar button, File menu, or Ctrl+S. Without Command, each trigger would contain save logic, creating duplication. With Command, you create a `SaveCommand` object and associate it with all three triggers. The triggers simply call `command->execute()`.

This also enables undo: each command stores enough state to reverse itself.

### Applicability
Use the Command pattern when:
- You want to parameterize objects with an action to perform (callbacks on steroids)
- You want to specify, queue, and execute requests at different times
- You need to support undo/redo operations
- You need to support logging changes so they can be reapplied (crash recovery)
- You want to structure a system around high-level operations built on primitive operations (transactions)
- You need macro recording (sequence of commands replayed)

### Structure

```
  +----------+        +-------------------+        +-----------+
  |  Client  |------->|     Invoker       |        | Receiver  |
  +----------+        +-------------------+        +-----------+
  | creates   |       | - command: Command*|       | + action()|
  | commands  |       | + executeCommand()|        +-----------+
  +----------+        |   { command->      |              ^
       |              |     execute(); }   |              |
       |              +-------------------+              |
       |                       |                         |
       v                       v                         |
  +-------------------+                                  |
  |     Command       |                                  |
  +-------------------+                                  |
  | + execute()       |                                  |
  | + undo()          |                                  |
  +-------------------+                                  |
         ^                                               |
         |                                               |
  +-------------------+                                  |
  | ConcreteCommand   |----------------------------------+
  +-------------------+
  | - receiver: Recv* |
  | - savedState      |
  | + execute()       |
  |   { receiver->    |
  |     action(); }   |
  | + undo()          |
  |   { // reverse }  |
  +-------------------+
```

### Participants
- **Command** - declares an interface for executing an operation (and optionally undoing it)
- **ConcreteCommand** - defines a binding between a Receiver and an action; implements `execute()` by invoking the corresponding operation(s) on Receiver; stores state for undo
- **Client** - creates a ConcreteCommand and sets its receiver
- **Invoker** - asks the command to carry out the request
- **Receiver** - knows how to perform the operations; any class can be a Receiver

### Undo/Redo Implementation with Command History Stack

```
  +-------------------+
  |   CommandHistory   |
  +-------------------+
  | - undoStack: []    |     execute(cmd)         undo()             redo()
  | - redoStack: []    |     +---------+          +---------+       +---------+
  | + execute(Command) |     |1. cmd->  |         |1. pop from|     |1. pop from|
  | + undo()           |     |  execute()|        |   undoStack|    |   redoStack|
  | + redo()           |     |2. push to |        |2. cmd->   |     |2. cmd->   |
  +-------------------+     |  undoStack|         |   undo()  |     |   execute()|
                            |3. clear   |         |3. push to |     |3. push to |
                            |  redoStack|         |   redoStack|    |   undoStack|
                            +---------+          +---------+       +---------+
```

```
  Example: Text Editor
  
  Action sequence: type "Hello", type " World", undo, undo, redo
  
  After "Hello":    undoStack: [WriteCmd("Hello")]      redoStack: []
  After " World":   undoStack: [WriteCmd("Hello"),      redoStack: []
                                WriteCmd(" World")]
  After undo:       undoStack: [WriteCmd("Hello")]      redoStack: [WriteCmd(" World")]
                    (text is now "Hello")
  After undo:       undoStack: []                       redoStack: [WriteCmd(" World"),
                    (text is now "")                                 WriteCmd("Hello")]
  After redo:       undoStack: [WriteCmd("Hello")]      redoStack: [WriteCmd(" World")]
                    (text is now "Hello")
```

### Macro Commands (Composite Command)
A Macro Command is a command that contains a list of sub-commands and executes them all in sequence:

```cpp
class MacroCommand : public Command {
    vector<shared_ptr<Command>> commands;
public:
    void add(shared_ptr<Command> cmd) { commands.push_back(cmd); }
    
    void execute() override {
        for (auto& cmd : commands) cmd->execute();
    }
    
    void undo() override {
        // Undo in REVERSE order!
        for (auto it = commands.rbegin(); it != commands.rend(); ++it)
            (*it)->undo();
    }
};
```

This is Command + Composite patterns combined.

### Command Queues and Scheduling
Commands can be serialized and executed later or on different threads:

```
  Producer Thread          Command Queue           Consumer Thread
  +-------------+         +---+---+---+          +---------------+
  | creates cmds|-------->| C | C | C |--------->| executes cmds |
  +-------------+         +---+---+---+          +---------------+
                          (thread-safe queue)
```

Use cases:
- Task schedulers (execute commands at specific times)
- Worker pools (distribute commands across threads)
- Transaction logs (persist commands for crash recovery)
- Batch processing (collect commands, execute in batch)

### Consequences

**Benefits:**
1. Decouples the object that invokes the operation from the one that knows how to perform it
2. Commands are first-class objects - can be manipulated and extended like any other
3. Easy to add new commands (Open/Closed Principle)
4. Can assemble commands into composite (macro) commands
5. Supports undo/redo, deferred execution, logging, and transaction semantics

**Liabilities:**
1. Increases number of classes (one per command)
2. Commands that save state for undo can consume significant memory
3. For simple operations, the pattern adds unnecessary indirection
4. Undo can be complex when commands have side effects (network calls, file I/O)

### Implementation Notes (C++ Specific)
- Use `std::function<void()>` for simple commands without undo (lightweight alternative)
- Use `std::stack<unique_ptr<Command>>` for undo/redo history
- For thread-safe command queues, use `std::queue` protected by `std::mutex`
- Consider `std::packaged_task` for commands that produce future results
- Lambda captures can serve as lightweight command objects in modern C++

### Known Uses
- **Text Editors**: Every keystroke is a command with undo capability (Vim, VS Code)
- **Database Transactions**: Each SQL statement is a command; ROLLBACK is undo
- **Game Input Systems**: Player actions queued as commands, enabling replay/undo
- **CI/CD Pipelines**: Each pipeline step is a command with rollback on failure
- **std::function**: C++ standard library's callable wrapper is a generalized command
- **QAction in Qt**: Menu items and toolbar buttons backed by command objects

### Common Misconceptions
1. **"Command and Strategy are the same"** - Strategy selects among algorithms for the same task. Command encapsulates an entire request/action as an object. See comparison below.
2. **"Every command must support undo"** - Not necessarily. Some commands are fire-and-forget (logging, analytics events). Undo is optional.
3. **"Command pattern requires a Receiver"** - "Smart commands" can contain the logic directly without delegating to a receiver.

### Related Patterns
- **Composite**: for macro commands (command containing sub-commands)
- **Memento**: can store state for undo (command stores memento before executing)
- **Prototype**: command objects can be cloned for history/logging
- **Strategy**: encapsulates an algorithm; Command encapsulates a request
- **Chain of Responsibility**: commands can be passed along a chain of handlers

---

## State Pattern

### Intent (GoF)
Allow an object to alter its behavior when its internal state changes. The object will appear to change its class.

### Also Known As
- Objects for States
- State Machine

### Real-World Analogy
Think of a gumball machine. It has distinct states: No Quarter (waiting), Has Quarter (ready), Gumball Sold (dispensing), and Sold Out (empty). The same action (turning the crank) produces completely different behavior depending on the current state:
- No Quarter state: "Please insert a quarter first"
- Has Quarter state: dispenses gumball, transitions to No Quarter (or Sold Out)
- Sold Out state: "Sorry, no gumballs available"

Each state is like a completely different "personality" of the same machine.

### Motivation: Eliminating Complex Conditionals
Consider a TCP connection that can be in states: Established, Listening, or Closed. Each state handles operations (`open`, `close`, `acknowledge`) differently.

**Without State pattern (conditionals everywhere):**
```cpp
void TCPConnection::open() {
    if (state == CLOSED) { /* send SYN, go to LISTENING */ }
    else if (state == LISTENING) { /* error: already open */ }
    else if (state == ESTABLISHED) { /* error: already connected */ }
}
void TCPConnection::close() {
    if (state == CLOSED) { /* error: already closed */ }
    else if (state == LISTENING) { /* cleanup, go to CLOSED */ }
    else if (state == ESTABLISHED) { /* send FIN, go to CLOSED */ }
}
// Every method has N branches for N states. Adding a state = modifying every method!
```

**With State pattern:**
```cpp
void TCPConnection::open() { currentState->open(this); }
void TCPConnection::close() { currentState->close(this); }
// Each state class handles its own behavior. Adding a state = adding one class.
```

### State Transition Table
A formal way to define state machines:

```
  +------------------+----------+------------------+----------+
  | Current State    | Event    | Next State       | Action   |
  +------------------+----------+------------------+----------+
  | NoQuarter        | insertQ  | HasQuarter       | accept   |
  | NoQuarter        | turnCrank| NoQuarter        | reject   |
  | HasQuarter       | insertQ  | HasQuarter       | refund   |
  | HasQuarter       | turnCrank| GumballSold      | dispense |
  | HasQuarter       | ejectQ   | NoQuarter        | return   |
  | GumballSold     | dispense | NoQuarter/SoldOut| release  |
  | SoldOut          | insertQ  | SoldOut          | reject   |
  +------------------+----------+------------------+----------+
```

### ASCII State Machine Diagram

```
                  insertQuarter()
  +------------+  ------------->  +-------------+
  | NoQuarter  |                  | HasQuarter  |
  |   State    |  <-------------  |    State    |
  +------------+  ejectQuarter()  +-------------+
       ^                                |
       |                                | turnCrank()
       |                                v
       |                          +-------------+
       |    dispense()            | GumballSold |
       +<-------------------------+    State    |
       |  (if gumballs > 0)       +-------------+
       |                                |
       |    dispense()                  |
       |  (if gumballs == 0)            v
       |                          +-------------+
       +------------------------->|  Sold Out   |
                                  |    State    |
                                  +-------------+
```

### Applicability
Use the State pattern when:
- An object's behavior depends on its state, and it must change behavior at runtime
- Operations have large multipart conditional statements that depend on the object's state
- State-specific behavior should be localized in separate classes
- Transitions between states follow defined rules

### Structure

```
  +-------------------+              +-------------------+
  |     Context       |              |      State        |
  +-------------------+  has-a       +-------------------+
  | - state: State*   |------------->| + handle(Context) |
  | + request()       |              +-------------------+
  |   { state->       |                    ^       ^
  |     handle(this); }|                   |       |
  | + setState(State) |              +-----+--+ +--+-------+
  +-------------------+              |ConcreteA| |ConcreteB |
                                     |State    | |State     |
                                     +----------+ +----------+
                                     | +handle(){  | +handle(){
                                     |   // do A   |   // do B
                                     |   context-> |   context->
                                     |   setState  |   setState
                                     |   (stateB); |   (stateC);
                                     |  }          |  }
                                     +----------+  +----------+
```

### Participants
- **Context** - defines the interface of interest to clients; maintains a reference to the current State object; delegates state-specific work to current State
- **State** - defines an interface for encapsulating the behavior associated with a particular state of the Context
- **ConcreteState** - each subclass implements behavior associated with a state of the Context; handles transitions by calling `context->setState()`

### Who Controls Transitions?

**Option 1: State controls transitions (most common)**
```cpp
void HasQuarterState::turnCrank(VendingMachine* ctx) {
    cout << "Dispensing..." << endl;
    ctx->setState(make_unique<GumballSoldState>());  // State decides next state
}
```

**Option 2: Context controls transitions (centralized)**
```cpp
void VendingMachine::turnCrank() {
    state->turnCrank(this);
    if (/* condition */) setState(make_unique<NextState>());  // Context decides
}
```

Option 1 is decentralized (each state knows its successors). Option 2 is centralized (Context owns the transition table). Choose based on complexity of transition logic.

### Consequences

**Benefits:**
1. Localizes state-specific behavior - each state is in its own class (Single Responsibility)
2. Makes state transitions explicit - clear which states can transition to which
3. State objects can be shared (if they have no instance variables - Flyweight)
4. Eliminates large conditional statements
5. Easy to add new states without modifying existing state classes (Open/Closed)

**Liabilities:**
1. Increases the number of classes (one per state)
2. State transitions are scattered across state classes (unless using a table-driven approach)
3. Context exposes internal details to State classes (for transition purposes)
4. For simple state machines with 2-3 states, a switch statement may be clearer

### Implementation Notes (C++ Specific)
- Use `std::unique_ptr<State>` in Context for exclusive state ownership
- State objects without instance variables can be shared (Flyweight/Singleton per state)
- Consider using `std::variant` with `std::visit` for compile-time state machines
- For complex state machines, consider table-driven approaches with `std::function`
- `std::optional<State>` can represent "no state" (initial or terminal)
- Use `enum class` + switch for trivial 2-3 state machines (YAGNI applies)

### Known Uses
- **TCP Connection States**: LISTEN, SYN_SENT, SYN_RECEIVED, ESTABLISHED, FIN_WAIT, CLOSED
- **Game Characters**: Idle, Walking, Running, Jumping, Attacking, Dead
- **Order Processing**: Placed, Paid, Shipped, Delivered, Returned, Cancelled
- **Document Workflow**: Draft, InReview, Approved, Published, Archived
- **UI Components**: Normal, Hover, Pressed, Disabled, Focused
- **Vending Machines**: Classic textbook example

### Common Misconceptions
1. **"State pattern is for any enum-based state"** - If state does not change behavior, you do not need the pattern. A color enum on a widget does not warrant State pattern.
2. **"State and Strategy are interchangeable"** - State transitions internally; Strategy is selected externally. States know about each other; Strategies do not.
3. **"State pattern replaces all state machines"** - For simple state machines, an enum + switch is cleaner. Use the pattern when state-specific behavior is complex (multiple methods behave differently per state).

### Related Patterns
- **Strategy**: same structure but different intent (see comparison in Day 11)
- **Singleton/Flyweight**: state objects without instance data can be shared
- **Command**: can trigger state transitions

---

## Command vs Strategy Comparison

Both encapsulate behavior in objects, but for different reasons:

| Aspect | Command | Strategy |
|--------|---------|----------|
| **Encapsulates** | A request/action (WHAT to do) | An algorithm (HOW to do something) |
| **When created** | Per action/event (many instances) | Per algorithm family (few instances) |
| **State** | Often stores state for undo | Usually stateless or minimal state |
| **Lifetime** | Short-lived (execute once, then maybe undo) | Long-lived (used repeatedly) |
| **Undo** | Core feature | Not applicable |
| **History** | Often stored in stack/queue | Not typically stored |
| **Example** | "Delete row 5", "Copy selection" | "Sort with quicksort", "Compress with gzip" |
| **Receiver** | Delegates to a receiver object | IS the algorithm implementation |
| **Composability** | Macro commands (composite) | Usually one at a time |

---

## Code Examples

### Files
- [command_remote.cpp](command_remote.cpp) - Universal remote control with programmable buttons
- [command_undo.cpp](command_undo.cpp) - Text editor with undo/redo stack
- [state_vending.cpp](state_vending.cpp) - Vending machine with state transitions

---

## Interview Questions

1. **Command vs Strategy** - both encapsulate behavior. What fundamentally differs?
   - Command encapsulates a request (what to do + undo capability + history). Strategy encapsulates an algorithm (how to do something, swappable). Commands are created per action; strategies are reused.

2. **How to implement undo/redo using Command stack?**
   - Maintain undoStack and redoStack. On execute: push to undoStack, clear redoStack. On undo: pop from undoStack, call undo(), push to redoStack. On redo: pop from redoStack, call execute(), push to undoStack.

3. **State vs Strategy** - both use polymorphism with identical structure.
   - State transitions happen internally (state objects know about each other). Strategy is set externally by the client (strategies are independent). State represents "what I am"; Strategy represents "how I do something".

4. **State pattern vs giant if/switch on enum** - trade-offs?
   - Switch: simpler for 2-3 states, all logic visible in one place. State pattern: better for 5+ states, each state is isolated, easy to add new states, but more classes and dispersed transition logic.

5. **Real-world: Model an order's lifecycle using State.**
   - States: Placed, Paid, Shipped, Delivered, Cancelled, Returned. Each state handles events (pay, ship, deliver, cancel, return) and only allows valid transitions. E.g., cannot ship before paying; cannot return before delivery.

6. **How would you persist a Command queue for crash recovery?**
   - Serialize each command to a log file (write-ahead log). On recovery, replay the log. This is similar to how databases use transaction logs and event-sourced systems replay events.

7. **What is a "smart command" vs a "dumb command"?**
   - Dumb command: thin wrapper that just delegates to a Receiver. Smart command: contains the logic itself (no separate Receiver). Smart commands are simpler but violate SRP for complex operations.

---

## Daily Assignment

1. **Command with Undo**: Build a text editor with `WriteCommand`, `DeleteCommand`, and `ReplaceCommand`. Implement undo stack of size 100 (drop oldest when full). Implement redo.

2. **Macro Command**: Build a macro recorder that binds key sequences to compound commands. Record a sequence (select all, copy, paste, format), then replay it with one keystroke.

3. **State - Document Workflow**: Build a `Document` with states: `Draft`, `Moderation`, `Published`, `Archived`. Implement transitions controlled by state classes:
   - Draft -> Moderation (on submit)
   - Moderation -> Published (on approve) or Draft (on reject)
   - Published -> Archived (on archive)
   - Only admins can transition from Moderation.

4. **Command Queue**: Implement a thread-safe command queue where producer threads enqueue commands and a worker thread dequeues and executes them in order.
