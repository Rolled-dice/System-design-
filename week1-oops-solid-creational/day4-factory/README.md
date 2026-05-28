# Day 4 - Factory Method Pattern

## Overview

The Factory Method pattern is one of the most widely used creational patterns in software engineering. It addresses a fundamental tension: client code needs to create objects, but it should not be coupled to the specific classes it creates. By delegating instantiation to subclasses or specialized methods, Factory Method enables code that is both extensible and decoupled from concrete implementations.

Understanding Factory Method deeply requires distinguishing it from two related but distinct concepts: Simple Factory (a function that creates objects) and Abstract Factory (a factory of factories). This day covers all three, with emphasis on the true GoF Factory Method pattern.

---

## GoF Documentation

### Intent

Define an interface for creating an object, but let subclasses decide which class to instantiate. Factory Method lets a class defer instantiation to subclasses.

### Motivation

Consider a framework for building document editors. The framework defines abstract classes for `Application` and `Document`. Clients subclass these to create their specific applications (drawing app, text editor). The framework must create documents, but it cannot know in advance which specific document type to create - that depends on the subclass.

The Factory Method pattern solves this: the `Application` class defines an abstract `createDocument()` method. Each concrete application subclass overrides this method to create the appropriate document type. The framework calls `createDocument()` without knowing the concrete class.

This is also called a "Virtual Constructor" because, like constructors, it creates objects, but unlike constructors, it can be overridden in subclasses to create different types.

### Applicability

Use Factory Method when:
- A class cannot anticipate the class of objects it must create
- A class wants its subclasses to specify the objects it creates
- Classes delegate responsibility to one of several helper subclasses, and you want to localize the knowledge of which helper subclass is the delegate
- You want to provide a hook for subclasses to provide an extended version of an object

**Decision Checklist:**
1. Is the type of object to create determined by context (configuration, runtime input, subclass)?
2. Do you want to avoid tight coupling between creator and product?
3. Is there a family of related creation logic that should be delegable?
4. Do you need to support extension (new product types) without modifying existing creator code?

### Structure

```
+---------------------+              +-------------------+
|      Creator        |              |     Product       |
+---------------------+              +-------------------+
| + someOperation()   |              | + operation()     |
| + factoryMethod():  |----creates-->|                   |
|   Product* [abstract|              +--------+----------+
+----------+----------+                       ^
           ^                                  |
           |                         +--------+----------+
+----------+----------+              |  ConcreteProduct  |
|   ConcreteCreator   |              +-------------------+
+---------------------+              | + operation()     |
| + factoryMethod():  |----creates-->|                   |
|   Product*          |              +-------------------+
+---------------------+

  Creator::someOperation() {
      Product* p = factoryMethod();  // Delegates to subclass
      p->operation();
  }
```

### Participants

- **Product** (Document) - defines the interface of objects the factory method creates
- **ConcreteProduct** (DrawingDocument, TextDocument) - implements the Product interface
- **Creator** (Application) - declares the factory method, which returns a Product. May define a default implementation. Calls the factory method to create Product objects.
- **ConcreteCreator** (DrawingApplication) - overrides the factory method to return a ConcreteProduct instance

### Collaborations

Creator relies on its subclasses to define the factory method so that it returns an instance of the appropriate ConcreteProduct. The creator's other methods can then use the product without knowing its concrete type.

### Consequences

**Benefits:**
1. Eliminates the need to bind application-specific classes into your code. The code only deals with the Product interface.
2. Provides hooks for subclasses - creating objects inside a class with a factory method is always more flexible than creating directly. Subclasses can provide extended versions.
3. Connects parallel class hierarchies - when a class delegates some responsibilities to a separate hierarchy, factory method can create the appropriate delegate.
4. Supports the Open/Closed Principle - new product types can be added without modifying existing creator code.

**Costs:**
1. Requires subclassing the Creator just to create a particular ConcreteProduct. This is acceptable when subclassing is already happening for other reasons, but can be overkill if the Creator subclass has no other purpose.
2. Can lead to parallel hierarchies (one creator per product) which increases class count.

### Implementation Details (C++ Specific)

**Parameterized Factory Method:**
```cpp
Product* Creator::create(const std::string& type) {
    if (type == "circle") return new Circle();
    if (type == "square") return new Square();
    return nullptr;  // or throw
}
```

**Template-based Factory Method:**
```cpp
template<typename ProductType>
std::unique_ptr<Product> createProduct() {
    return std::make_unique<ProductType>();
}
```

**Return Type: Raw vs Smart Pointer:**
Modern C++ factory methods should return `std::unique_ptr<Product>`. This clearly communicates ownership transfer and ensures no memory leaks:
```cpp
virtual std::unique_ptr<Product> createProduct() = 0;
```

### Known Uses

- **Qt Framework** - `QWidget::createWindowContainer()`, `QApplication::createWidget()`
- **Game Engines** (Unity, Unreal) - Entity/Actor factories that create game objects from type identifiers
- **Plugin systems** - `dlopen()` + factory function that creates plugin instances through a common interface
- **GUI frameworks** - creating platform-specific widgets through a common API
- **Logging frameworks** - `LoggerFactory::getLogger(name)` creates appropriate logger instances

### Related Patterns

- **Abstract Factory** - often implemented with Factory Methods
- **Template Method** - factory methods are often called from template methods
- **Prototype** - alternative where you clone a prototype instead of calling a factory method
- **Singleton** - a ConcreteCreator is often a Singleton

---

## Simple Factory vs Factory Method vs Abstract Factory

This is one of the most commonly confused topics in design patterns. Here is the definitive comparison:

### Simple Factory (Not a GoF Pattern)

A single function or class that creates objects based on parameters. No inheritance involved.

```
+-------------------+         +-------------------+
|   SimpleFactory   |         |     Product       |
+-------------------+         +-------------------+
| + create(type):   |-------->| (interface)       |
|   Product*        |         +-------------------+
+-------------------+                  ^
                                       |
                              +--------+--------+
                              |        |        |
                           ProductA ProductB ProductC
```

**When to use:** Object creation logic is complex but you have no need for subclass extensibility. The factory is a utility, not a framework extension point.

### Factory Method (GoF)

Uses inheritance - the creator delegates to subclasses. The creation decision is spread across a class hierarchy.

**When to use:** A framework needs to create objects but cannot know which concrete types until subclassed by application code.

### Abstract Factory (GoF)

A factory of related products (families). Creates multiple related objects that must be consistent.

**When to use:** You need to create families of related objects (a Windows button + Windows scrollbar, OR a Mac button + Mac scrollbar, but never a Windows button + Mac scrollbar).

| Aspect | Simple Factory | Factory Method | Abstract Factory |
|--------|---------------|----------------|------------------|
| Pattern type | Not GoF | GoF Creational | GoF Creational |
| Mechanism | Function/static method | Inheritance (override) | Object composition |
| Products | One product type | One product type | Family of products |
| Extension | Modify factory | Add new Creator subclass | Add new factory |
| OCP violation | Yes (modify switch) | No | No |
| Complexity | Low | Medium | High |

---

## The Virtual Constructor Concept

C++ constructors cannot be virtual. You cannot call `new Base()` and get a Derived object back. However, Factory Method achieves the effect of a "virtual constructor":

```
// Without Factory Method: caller must know exact type
Document* doc = new DrawingDocument();  // Coupled to concrete class

// With Factory Method: creation is polymorphic
Document* doc = app->createDocument();  // Could be any document type
```

The factory method acts as a "virtual constructor" because:
1. It returns a pointer/reference to a base class
2. The concrete type created depends on the runtime type of the creator
3. Client code does not know or care what specific type is instantiated

---

## Self-Registering Factory Pattern

One limitation of Factory Method is that adding a new product type requires modifying the factory (violating OCP in parameterized versions). The self-registering factory solves this:

```
// Registry: maps type names to creation functions
class ProductFactory {
    static std::map<std::string, std::function<Product*()>> registry;
public:
    static void registerProduct(const std::string& name, 
                                std::function<Product*()> creator) {
        registry[name] = creator;
    }
    static Product* create(const std::string& name) {
        return registry[name]();
    }
};

// Each product registers itself (e.g., in a .cpp file)
static bool registered = [] {
    ProductFactory::registerProduct("circle", []{ return new Circle(); });
    return true;
}();
```

**How it works:** Each concrete product registers its own creation function at static initialization time. The factory never needs to know about specific products. Adding a new product means writing a new class and adding a registration line. No existing code changes.

**Real-world usage:**
- Plugin architectures (DLLs register their factories on load)
- Test frameworks (tests self-register with a test runner)
- Serialization systems (each serializable type registers its deserializer)

---

## Real-World Usage Examples

### Qt Framework
Qt uses factories extensively. `QStyleFactory::create("fusion")` returns a platform-appropriate style object. The factory encapsulates platform detection and returns the correct implementation.

### Game Engines
Entity Component Systems use factories to spawn entities from prefab names:
```
Entity* enemy = EntityFactory::create("goblin_archer");
// Creates entity with correct components, stats, AI behavior
```

### Plugin Systems
A plugin architecture loads shared libraries at runtime. Each library exports a factory function:
```
// Plugin interface
extern "C" Plugin* createPlugin();

// Host application
void* handle = dlopen("my_plugin.so", RTLD_LAZY);
auto factory = (Plugin*(*)()) dlsym(handle, "createPlugin");
Plugin* p = factory();
```

### Database Drivers
ODBC/JDBC use factories to create connections without knowing the specific database:
```
Connection* conn = DriverManager::getConnection("mysql://localhost/mydb");
// Returns MySQLConnection, but caller only sees Connection interface
```

---

## Common Misconceptions

1. **"Factory Method = any method that creates objects"** - Not every creation method is a Factory Method pattern. The GoF pattern specifically involves inheritance and subclass delegation. A static `create()` function is a Simple Factory.

2. **"Factory is always better than `new`"** - For simple, unambiguous object creation with no variation, direct construction is clearer and has less indirection.

3. **"Abstract Factory is just multiple Factory Methods"** - While Abstract Factory often uses Factory Methods internally, its purpose is different: ensuring consistency across a family of products.

4. **"Factory Method requires a separate factory class"** - In the GoF pattern, the factory method is defined on the Creator class itself. The creator IS the factory.

5. **"Parameterized factory with switch/if is Factory Method"** - This is Simple Factory. Factory Method uses polymorphism (override), not conditionals.

---

## Why Does This Matter in System Design?

Factory patterns appear everywhere in system architecture:
- **Microservice communication:** Message serializers are created by factories based on content type
- **Cloud resource provisioning:** VM, container, and serverless creators implement a common factory interface
- **API versioning:** Request handlers are created by version-specific factories
- **Database migration tools:** Schema migration steps are created by factories based on database type

Understanding factories helps you design systems where new types can be added without modifying existing, deployed code - a critical requirement for systems that must evolve without downtime.

---

## Files

- [factory_simple.cpp](factory_simple.cpp) - Simple factory (not a true GoF pattern but common)
- [factory_method.cpp](factory_method.cpp) - True Factory Method
- [logistics_factory.cpp](logistics_factory.cpp) - Real-world: shipping logistics

---

## Interview Questions

1. Difference between Simple Factory, Factory Method, and Abstract Factory.
2. How does Factory Method support OCP?
3. Where would you use Factory Method in a payment system?
4. Factory Method vs Strategy - both use polymorphism. What differs?
5. How to register factories dynamically (factory registry / self-registration)?

**Advanced**
6. Explain the Virtual Constructor idiom and how Factory Method implements it.
7. How would you design a plugin system using Factory Method + dynamic loading?
8. What are the thread-safety concerns with a self-registering factory during static initialization?
9. How does Factory Method interact with dependency injection containers?
10. Compare Factory Method with template-based creation (e.g., `make_unique<T>()`) - when is each appropriate?

---

## Daily Assignment

1. Build a `ShapeFactory` that creates `Circle/Square/Triangle` from a string input.
2. Build a `NotificationFactory` that returns `Email/SMS/Push` notifiers based on user preference.
3. Implement a self-registering factory using a static registry map.
4. Design a `DocumentApp` framework where subclasses (`TextApp`, `SpreadsheetApp`, `DrawingApp`) each override `createDocument()` to produce their specific document types. Demonstrate that the framework's `openDocument()` logic works without knowing concrete types.
5. Implement a parameterized factory with proper error handling (what happens when an unknown type is requested?).
