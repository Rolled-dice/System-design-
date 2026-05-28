# Day 2 - SOLID Principles (C++)

## Overview

SOLID is an acronym for five design principles that, when followed together, make software systems easier to maintain, extend, and understand. Coined by Robert C. Martin (Uncle Bob), these principles represent decades of collective wisdom about what makes object-oriented code robust over time.

However, SOLID is not a religion. These principles are guidelines, not laws. Blindly applying them can lead to over-engineered systems with dozens of tiny classes that are harder to understand than a simpler, slightly "impure" design. The key is understanding WHEN and WHY to apply each principle, and knowing when the cost of compliance exceeds the benefit.

| # | Principle | One-line |
|---|-----------|----------|
| **S** | Single Responsibility | A class should have one reason to change |
| **O** | Open/Closed | Open for extension, closed for modification |
| **L** | Liskov Substitution | Subtypes must be substitutable for their base types |
| **I** | Interface Segregation | Many client-specific interfaces > one fat interface |
| **D** | Dependency Inversion | Depend on abstractions, not concretions |

---

## S - Single Responsibility Principle (SRP)

### Explanation

The Single Responsibility Principle states that a class should have only one reason to change. "Reason to change" means one axis of requirements evolution. If a class handles both business logic and persistence, it has two reasons to change: business rules might change, OR the database schema might change.

The deeper insight is about *coupling to stakeholders*. Different stakeholders (the accountant, the DBA, the UI designer) should not be able to break each other's code through changes to a shared class. SRP ensures that each class is "owned" by exactly one stakeholder's concerns.

This does not mean a class should have only one method. It means all methods in a class should be cohesive - they should all change for the same reason.

### Real-World Analogy

Consider a Swiss Army knife vs a chef's knife. The Swiss Army knife violates SRP: it is a knife, a screwdriver, a bottle opener, and a saw. If the blade needs resharpening, you must handle the entire tool. A chef's knife does ONE thing excellently. A restaurant kitchen has specialized tools (bread knife, paring knife, cleaver) because each can be maintained, replaced, and optimized independently.

### Bad Design Scenario

Imagine an `Employee` class that:
- Calculates salary (business logic - owned by Finance)
- Generates PDF reports (presentation - owned by Reporting team)
- Saves to database (persistence - owned by DBA)
- Sends notification emails (infrastructure - owned by DevOps)

When the Finance team changes the salary formula, the developer touches the same file that handles email sending. A typo could break email notifications. When the DBA migrates to a new database, they must modify a class containing salary business logic. Every change is risky because unrelated concerns are entangled.

### Good Design Scenario

Split into focused classes:
- `SalaryCalculator` - only salary computation logic
- `EmployeeReportGenerator` - only report formatting
- `EmployeeRepository` - only database access
- `NotificationService` - only email/SMS delivery

Now each class has a single reason to change, a single team responsible for it, and changes to one cannot accidentally break another.

### When to Violate SRP

- **Prototyping and MVPs:** When speed matters more than maintainability, a God class that gets the feature shipped is acceptable. Refactor later when the design stabilizes.
- **Performance-critical paths:** Sometimes combining responsibilities in a single class improves cache locality or reduces allocations. Profiler data justifies this.
- **Simple CRUD:** A class that maps 1:1 to a database table and does nothing else may reasonably combine validation and persistence.

### Real-World Violations

- **Java's `Thread` class** combines thread lifecycle management with the `Runnable` task. This was recognized as a design flaw; modern code prefers `ExecutorService` + `Callable`.
- **ActiveRecord pattern** (Rails) intentionally violates SRP by combining business logic with persistence. It works for simple models but struggles with complex domains.
- **Early Android `Activity`** classes handled UI, lifecycle, data loading, and navigation - leading to "Massive Activity" anti-pattern.

---

## O - Open/Closed Principle (OCP)

### Explanation

Software entities (classes, modules, functions) should be open for extension but closed for modification. You should be able to add new behavior without changing existing, tested code.

This sounds paradoxical until you realize it is achieved through abstraction. By depending on interfaces rather than concrete classes, you can extend a system by adding new implementations of those interfaces, without touching the code that uses them.

OCP is what makes plugin architectures possible. A text editor that supports new file formats via plugins follows OCP: the editor code is closed for modification, but open for extension through the plugin interface.

### Real-World Analogy

Consider electrical outlets. The outlet standard (interface) is fixed - closed for modification. But you can plug in any device that conforms to the standard: a lamp, a charger, a blender. The house's electrical system is extended (new devices) without modifying the wiring. If every new appliance required rewiring the house, the system would be unmaintainable.

### Bad Design Scenario

A `PaymentProcessor` class with a giant switch statement:
```
if (type == "credit") { ... }
else if (type == "debit") { ... }
else if (type == "paypal") { ... }
// Adding crypto requires modifying this class
```

Every new payment method requires opening the existing file, adding another branch, and retesting everything. Each modification risks breaking existing payment flows.

### Good Design Scenario

Define a `PaymentMethod` interface with a `process()` method. Each payment type is a separate class implementing the interface. Adding cryptocurrency support means creating a new class `CryptoPayment : PaymentMethod`. The processor iterates over `PaymentMethod*` pointers and calls `process()` without knowing or caring about the concrete type.

### When to Violate OCP

- **When the abstraction is premature:** If you have exactly one implementation and no realistic prospect of a second, creating an interface adds complexity without benefit. Wait for the second use case before abstracting.
- **When the variation axis is wrong:** If you guessed wrong about what would change, the abstraction boundary is in the wrong place. It is better to refactor when you have real data than to maintain a useless abstraction.
- **Stable leaf code:** Code that does one specific thing and will never change does not need extension points.

### Real-World Violations

- **Linux kernel** violates OCP in many places for performance. Instead of virtual dispatch for syscalls, it uses compile-time configuration and direct function calls.
- **Game engines** often use data-driven approaches (Entity-Component-System) instead of inheritance hierarchies, because the OCP-style class explosion becomes unmanageable with hundreds of entity types.

---

## L - Liskov Substitution Principle (LSP)

### Explanation

If class S is a subtype of class T, then objects of type T may be replaced with objects of type S without altering any of the desirable properties of the program (correctness, task performed, etc.). In practical terms: derived classes must honor the contracts established by their base classes.

This is more subtle than it first appears. It is not just about type compatibility (the compiler will enforce that). It is about *behavioral compatibility*. If `Base::withdraw(amount)` promises to never throw when balance is sufficient, then `Derived::withdraw(amount)` must also not throw in that situation. If `Base::getArea()` returns a positive number, `Derived::getArea()` must also return a positive number.

LSP violations typically manifest as code that checks `instanceof` or `dynamic_cast` to handle specific subtypes differently. If you find yourself writing `if (dynamic_cast<Square*>(shape))`, you likely have an LSP violation.

### Real-World Analogy

Consider a car rental company that promises "any car" for your reservation. You expect: it has an engine, it has four wheels, it drives forward when you press the accelerator. If they give you a "car" whose accelerator makes it go backward, that violates your expectations (the contract). Even though it IS technically a car, it cannot substitute for a normal car in your usage.

### Bad Design Scenario: The Classic Square/Rectangle Problem

A `Rectangle` class has `setWidth(w)` and `setHeight(h)`. A `Square` inherits from `Rectangle`. But a square has a constraint: width must equal height. So `Square::setWidth(w)` also sets the height.

Now consider code that uses `Rectangle*`:
```
Rectangle* r = getShape();  // might return Square
r->setWidth(5);
r->setHeight(3);
assert(r->area() == 15);  // FAILS if r is a Square (area is 9)
```

The Square violates the contract of Rectangle. Callers of Rectangle expect that setting width does not change height. Square breaks this expectation.

### Good Design Scenario

Do not make Square inherit from Rectangle. Instead, have both inherit from an abstract `Shape` with `area()` and `perimeter()`. Alternatively, make Rectangle immutable (no setters), which eliminates the contract violation since you cannot observe inconsistent state.

### When to Violate LSP

- **Rarely.** LSP violations are bugs waiting to happen. However, in practice, some frameworks use inheritance hierarchies that technically violate LSP for convenience (e.g., `NotImplementedException` in interface methods). This is a pragmatic compromise when full compliance would require dozens of tiny interfaces.

### Real-World Violations

- **Java's `Stack extends Vector`** - Stack is not substitutable for Vector because Stack has LIFO semantics while Vector allows random access. This is a famous LSP violation in the Java standard library.
- **`Properties extends Hashtable`** in Java - Properties should only contain String key-value pairs, but inheriting from Hashtable allows any Object.

---

## I - Interface Segregation Principle (ISP)

### Explanation

Clients should not be forced to depend on interfaces they do not use. If a class implements an interface with ten methods but only uses three, it is burdened with seven meaningless methods. More importantly, changes to those seven methods can force recompilation or redeployment of code that never uses them.

ISP promotes creating small, focused interfaces (often called "role interfaces") that represent specific capabilities. A class can implement multiple role interfaces, but each client only depends on the one interface it needs.

### Real-World Analogy

Consider a restaurant menu. A vegetarian customer should not be forced to read through 50 meat dishes to find the 10 vegetarian options. A well-designed restaurant provides a separate vegetarian menu (a segregated interface). The kitchen still prepares everything, but each customer interacts with only the relevant subset.

### Bad Design Scenario

A `Machine` interface with methods: `print()`, `scan()`, `fax()`, `staple()`, `email()`. A simple desktop printer must implement this entire interface. It throws `NotImplementedException` for `fax()`, `staple()`, and `email()`. Every time the interface changes (adding `print3D()`), the simple printer class must be updated even though it will never support 3D printing.

### Good Design Scenario

Split into role interfaces:
- `Printable` with `print()`
- `Scannable` with `scan()`
- `Faxable` with `fax()`
- `Stapleable` with `staple()`

A simple printer implements only `Printable`. A multifunction device implements `Printable + Scannable + Faxable`. Each client depends only on the interface it uses: the printing subsystem depends on `Printable`, not on the full `Machine` interface.

### When to Violate ISP

- **When interfaces are already small:** If your interface has 2-3 highly cohesive methods, splitting it further adds complexity without benefit.
- **When all implementations genuinely use all methods:** If every implementor needs every method, segregation adds interface classes with no practical value.
- **Internal APIs with few clients:** When the interface has one or two implementations and one client, the overhead of multiple interfaces is not justified.

### Real-World Violations

- **Java's `Iterator<E>`** includes `remove()`, which most iterators do not support (they throw `UnsupportedOperationException`). This was fixed in Java 8 with a default method.
- **JDBC `ResultSet`** has over 200 methods. Most code uses fewer than 10. This is a classic ISP violation that persists for backward compatibility.

---

## D - Dependency Inversion Principle (DIP)

### Explanation

High-level modules should not depend on low-level modules. Both should depend on abstractions. Abstractions should not depend on details. Details should depend on abstractions.

This inverts the traditional dependency direction. Instead of `BusinessLogic -> MySQLDatabase`, you have `BusinessLogic -> DatabaseInterface <- MySQLDatabase`. The business logic defines what it needs (the interface), and the infrastructure adapts to meet those needs.

DIP is the foundation of dependency injection frameworks, plugin architectures, and testable code. Without DIP, you cannot mock dependencies for unit testing, you cannot swap implementations without changing business logic, and you cannot deploy different configurations of the same codebase.

### Real-World Analogy

Consider a lamp and a wall outlet. The lamp (high-level, provides light) does not depend on the specific power plant generating electricity (low-level detail). Both depend on the abstraction: the standard electrical interface (voltage, frequency, plug shape). You can swap the power source (solar, nuclear, coal) without modifying the lamp.

### Bad Design Scenario

A `NotificationService` class directly creates `SmtpEmailSender` objects internally. To test the notification logic, you must have an SMTP server running. To switch to SendGrid, you must modify `NotificationService`. The high-level policy (send notifications) is coupled to low-level details (SMTP protocol).

### Good Design Scenario

`NotificationService` depends on an `EmailSender` interface. In production, you inject `SmtpEmailSender`. In tests, you inject `MockEmailSender`. To switch providers, you create `SendGridEmailSender` and inject it. The `NotificationService` code never changes.

### When to Violate DIP

- **Stable dependencies:** If you depend on something that never changes (the standard library, a fundamental data structure), abstracting it adds no value. Do not wrap `std::vector` in an interface.
- **Simple scripts and utilities:** A 100-line script that reads a file and processes it does not need dependency injection.
- **Performance-critical inner loops:** Virtual dispatch through DIP interfaces has overhead. In hot paths, direct calls may be justified.

### Real-World Violations

- **Direct database calls in controllers** are ubiquitous in rapid prototypes. This is fine for a proof of concept but becomes painful as the system grows.
- **Tightly coupled microservices** that make direct HTTP calls to specific URLs instead of using service discovery represent DIP violations at the architectural level.

---

## SOLID in Practice

### The Over-Engineering Trap

The most common mistake with SOLID is applying it too eagerly. Creating 15 interfaces, 30 classes, and 5 layers of abstraction for a feature that currently has one implementation and no prospect of change is worse than a simple, direct implementation.

The heuristics for "when to SOLID":
1. **Wait for the second use case.** One implementation does not need an interface.
2. **Separate things that change at different rates.** If business rules change weekly but the database schema changes yearly, they belong in different classes (SRP).
3. **Abstract at boundaries.** The edges of your system (external services, databases, UI) benefit most from DIP. Internal utility classes rarely need it.

### How SOLID Maps to Design Patterns

| Principle | Patterns That Embody It |
|-----------|------------------------|
| SRP | Facade (simplifies interfaces), Mediator (extracts coordination logic) |
| OCP | Strategy, Decorator, Observer (extend behavior without modification) |
| LSP | Template Method (subclasses must honor base contract) |
| ISP | Adapter (adapts fat interface to thin one), Facade |
| DIP | Abstract Factory, Bridge, Strategy, Observer (all depend on abstractions) |

### Pragmatic Guidelines

- Apply SRP when a class starts accumulating unrelated responsibilities (the "and" test: if you describe the class with "and", it might have too many responsibilities)
- Apply OCP when you find yourself repeatedly modifying the same class to add new variants
- Apply LSP always - violations are bugs, not tradeoffs
- Apply ISP when you see `NotImplementedException` or methods that exist only to satisfy an interface
- Apply DIP at system boundaries and for any dependency you want to mock in tests

---

## Dependency Inversion vs Dependency Injection

These are often confused but are distinct concepts:

- **Dependency Inversion** is a PRINCIPLE: depend on abstractions, not concretions
- **Dependency Injection** is a TECHNIQUE: pass dependencies to a class instead of having the class create them

You can do DI without DIP (inject a concrete class), and you can do DIP without DI (use a factory to create abstract instances). In practice, they work together: DIP defines the interface, DI provides the implementation.

| | Dependency Inversion | Dependency Injection |
|---|---|---|
| What | Design principle | Implementation technique |
| How | Create interfaces at boundaries | Pass dependencies via constructor/setter |
| Why | Decoupling, testability | Configurability, testability |
| Tools | Abstract classes, interfaces | Constructors, setters, DI frameworks |

---

## Files

- [srp.cpp](srp.cpp) - Single Responsibility violation + fix
- [ocp.cpp](ocp.cpp) - Open/Closed via polymorphism
- [lsp.cpp](lsp.cpp) - Liskov violation (Square/Rectangle classic)
- [isp.cpp](isp.cpp) - Interface segregation (printer/scanner/fax)
- [dip.cpp](dip.cpp) - Dependency inversion (notification service)

---

## Interview Questions

1. Give a real example where SRP is violated in a typical e-commerce `Order` class.
2. How does OCP relate to the Strategy pattern?
3. Why is `Square : Rectangle` a Liskov violation? How would you redesign?
4. ISP example with hardware multifunction printer.
5. DIP - why constructor injection > service locator?
6. Difference between Dependency Inversion and Dependency Injection.
7. Can SOLID hurt design (over-abstraction)? When to skip it?

**Advanced**
8. How would you refactor a 3000-line God class using SRP without breaking existing clients?
9. Explain how OCP enables hot-swappable plugins in a running system.
10. Give an LSP violation that compiles but produces runtime bugs.
11. How does ISP relate to the concept of "role interfaces" in domain-driven design?
12. In a microservices architecture, how does DIP manifest at the service boundary level?

---

## Daily Assignment

1. Take a `God class` `EmployeeManager` that:
   - Saves employees to DB
   - Sends emails
   - Generates PDF reports
   - Computes salary
   Refactor following all 5 SOLID principles. Provide before/after C++ code.
2. Identify SOLID violations in your last project's code and document fixes.
3. Design a notification system that supports Email, SMS, Push, and Slack. Apply OCP so that adding a new channel requires zero changes to existing code.
4. Create a `Shape` hierarchy that does NOT violate LSP (avoid the Square/Rectangle trap). Justify your design.
