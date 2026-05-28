# Day 5 - Abstract Factory Pattern

## Overview

The Abstract Factory pattern takes the factory concept one level higher. While Factory Method creates one product, Abstract Factory creates entire families of related products. Its primary purpose is to ensure that products from the same family are used together, preventing incompatible combinations that would lead to runtime errors or visual inconsistencies.

This pattern is most often encountered in cross-platform development, where you need to create a consistent set of widgets (buttons, text fields, scrollbars) for a specific platform without hard-coding platform knowledge throughout your application.

---

## GoF Documentation

### Intent

Provide an interface for creating families of related or dependent objects without specifying their concrete classes.

### Motivation

Consider a user interface toolkit that supports multiple look-and-feel standards (Motif, Windows, macOS). Different look-and-feels define different appearances and behaviors for UI widgets (scroll bars, buttons, windows). To be portable, an application should not hard-code its widgets for a particular look-and-feel.

We solve this by defining an abstract `WidgetFactory` class that declares an interface for creating each basic widget. There is also an abstract class for each widget type, and concrete subclasses that implement widgets for a specific look-and-feel. The abstract factory produces a complete set of widgets for one platform.

The client creates widgets through the factory interface. It never knows which concrete classes it uses. The factory ensures that all widgets come from the same family (you never get a Windows button paired with a macOS scrollbar).

### Applicability

Use Abstract Factory when:
- A system should be independent of how its products are created, composed, and represented
- A system should be configured with one of multiple families of products
- A family of related product objects is designed to be used together, and you need to enforce this constraint
- You want to provide a class library of products, and you want to reveal just their interfaces, not their implementations

**Decision Checklist:**
1. Do you have multiple families/variants of related objects?
2. Must objects within a family be used together (mixing families would be wrong)?
3. Do you want to switch entire families at once (e.g., platform change, theme change)?
4. Is there a realistic chance you will add new families in the future?

### Structure

```
+---------------------------+        +--------------------+
|    AbstractFactory        |        |  AbstractProductA  |
+---------------------------+        +--------------------+
| + createProductA():       |        | + operationA()     |
|   AbstractProductA*       |        +---------+----------+
| + createProductB():       |                  ^
|   AbstractProductB*       |                  |
+------------+--------------+        +---------+----------+
             ^                       | ConcreteProductA1  |
             |                       +--------------------+
    +--------+--------+              | ConcreteProductA2  |
    |                 |              +--------------------+
+---+---+       +-----+-----+
|Factory1|       |  Factory2  |      +--------------------+
+--------+       +-----------+       |  AbstractProductB  |
|createA |       |  createA  |       +--------------------+
|createB |       |  createB  |       | + operationB()     |
+--------+       +-----------+       +---------+----------+
                                               ^
                                               |
                                     +---------+----------+
                                     | ConcreteProductB1  |
                                     +--------------------+
                                     | ConcreteProductB2  |
                                     +--------------------+

  Factory1::createProductA() -> ConcreteProductA1
  Factory1::createProductB() -> ConcreteProductB1  (same family!)
  
  Factory2::createProductA() -> ConcreteProductA2
  Factory2::createProductB() -> ConcreteProductB2  (same family!)
```

### Participants

- **AbstractFactory** (WidgetFactory) - declares an interface for operations that create abstract product objects
- **ConcreteFactory** (MotifWidgetFactory, WindowsWidgetFactory) - implements the operations to create concrete product objects
- **AbstractProduct** (Window, ScrollBar) - declares an interface for a type of product object
- **ConcreteProduct** (MotifWindow, WindowsScrollBar) - defines a product object created by the corresponding concrete factory, implements the AbstractProduct interface
- **Client** - uses only interfaces declared by AbstractFactory and AbstractProduct classes

### Collaborations

- Normally a single instance of a ConcreteFactory class is created at runtime. This concrete factory creates products having a particular implementation. To create different product objects, clients should use a different concrete factory.
- AbstractFactory defers creation of product objects to its ConcreteFactory subclass.
- Client code works only with AbstractFactory and AbstractProduct, never with concrete classes directly.

### Consequences

**Benefits:**
1. **Isolates concrete classes** - clients manipulate instances through abstract interfaces. Product class names are isolated from client code.
2. **Exchanging product families is easy** - changing the concrete factory in one place switches the entire product family. The application can change look-and-feel by simply switching the factory object.
3. **Promotes consistency among products** - when products in a family are designed to work together, the factory ensures the application uses only one family at a time.

**Costs:**
1. **Supporting new kinds of products is difficult** - adding a new product (e.g., adding `createSpinner()` to the factory interface) requires changing the AbstractFactory interface and ALL of its concrete implementations. This is the pattern's main limitation.
2. **Complexity** - the pattern introduces many classes. For simple scenarios with few products and few families, it may be over-engineered.
3. **Tight coupling within families** - all products in a family must be created together. If some products are optional, the pattern becomes awkward.

### Implementation Details (C++ Specific)

**Smart pointer returns for clear ownership:**
```cpp
class GUIFactory {
public:
    virtual ~GUIFactory() = default;
    virtual std::unique_ptr<Button> createButton() = 0;
    virtual std::unique_ptr<TextBox> createTextBox() = 0;
    virtual std::unique_ptr<ScrollBar> createScrollBar() = 0;
};
```

**Factory selection at startup:**
```cpp
std::unique_ptr<GUIFactory> createPlatformFactory() {
    #ifdef _WIN32
        return std::make_unique<WindowsFactory>();
    #elif __APPLE__
        return std::make_unique<MacFactory>();
    #else
        return std::make_unique<LinuxFactory>();
    #endif
}
```

**Abstract Factory as Singleton:** It is common to make the concrete factory a Singleton since you typically need only one factory instance per application. However, this reintroduces the testing difficulties of Singleton.

### Known Uses

- **JDBC (Java)** - `DriverManager` returns a `Connection` which then creates `Statement`, `ResultSet`, etc., all from the same database family
- **Qt Framework** - style factories create platform-native widgets
- **OS abstraction layers** - creating file handles, sockets, threads appropriate to the current OS
- **Game engines** - rendering backend factories (DirectX vs Vulkan vs OpenGL) create textures, shaders, buffers from the same API family
- **GUI frameworks** (wxWidgets, GTK) - cross-platform widget creation

### Related Patterns

- **Factory Method** - Abstract Factory is often implemented using Factory Methods (each `createX()` is a factory method)
- **Prototype** - can be used instead when families are defined by a set of prototype objects rather than a class hierarchy
- **Singleton** - ConcreteFactory is often a Singleton
- **Bridge** - Abstract Factory can create the implementation side of a Bridge

---

## Family-of-Products Concept

The key insight of Abstract Factory is the concept of **product families**. Objects within a family are designed to work together, and mixing objects from different families leads to errors.

### Concrete Example: Database Abstraction

```
Family 1: MySQL
  - MySQLConnection (uses MySQL wire protocol)
  - MySQLCommand (uses MySQL SQL dialect)  
  - MySQLTransaction (uses MySQL transaction semantics)

Family 2: PostgreSQL
  - PostgresConnection (uses Postgres wire protocol)
  - PostgresCommand (uses Postgres SQL dialect)
  - PostgresTransaction (uses Postgres transaction semantics)

Mixing: MySQLConnection + PostgresCommand = RUNTIME ERROR
  (Postgres command cannot execute on MySQL connection)
```

The Abstract Factory prevents this mixing. If you have a `MySQLFactory`, ALL products it creates are MySQL-compatible. You cannot accidentally create a Postgres command and try to execute it on a MySQL connection.

### Why Not Just Use Separate Factories?

You could have a `ConnectionFactory`, a `CommandFactory`, and a `TransactionFactory` separately. But then nothing prevents code from using a MySQL connection factory and a Postgres command factory together. The Abstract Factory bundles them, enforcing consistency.

---

## Platform Abstraction: GUI Toolkits

The canonical example of Abstract Factory is cross-platform GUI development:

```
                    +------------------+
                    |   GUIFactory     |
                    +------------------+
                    | + createButton() |
                    | + createCheckbox()|
                    | + createMenu()   |
                    +--------+---------+
                             |
              +--------------+--------------+
              |                             |
    +---------+----------+       +----------+---------+
    |    MacFactory       |       |   WindowsFactory   |
    +--------------------+       +--------------------+
    | + createButton()   |       | + createButton()   |
    |   -> MacButton     |       |   -> WinButton     |
    | + createCheckbox() |       | + createCheckbox() |
    |   -> MacCheckbox   |       |   -> WinCheckbox   |
    | + createMenu()     |       | + createMenu()     |
    |   -> MacMenu       |       |   -> WinMenu       |
    +--------------------+       +--------------------+

Application code:
    void buildUI(GUIFactory& factory) {
        auto btn = factory.createButton();    // Mac or Win, don't care
        auto menu = factory.createMenu();     // Same family guaranteed
        btn->render();
        menu->render();
    }
```

The application code never uses platform-specific types. To port to a new platform (Linux), you add a `LinuxFactory` with `LinuxButton`, `LinuxCheckbox`, `LinuxMenu`. No existing application code changes.

---

## Comparison with Factory Method

| Aspect | Factory Method | Abstract Factory |
|--------|---------------|------------------|
| Purpose | Create ONE product polymorphically | Create FAMILY of products consistently |
| Mechanism | Inheritance (override a method) | Composition (inject a factory object) |
| Extension | Add new Creator subclass | Add new ConcreteFactory |
| Products | Single product hierarchy | Multiple product hierarchies |
| Guarantees | Correct single product | Correct combination of products |
| Complexity | Low-medium | Medium-high |

**Rule of thumb:** If you create one object, Factory Method is sufficient. If you create multiple related objects that must be consistent, Abstract Factory is appropriate.

### When Factory Method Becomes Abstract Factory

Often a system starts with Factory Method (creating one thing) and evolves into Abstract Factory as related products are added. The pattern boundary is not rigid.

---

## When Is the Complexity Justified?

Abstract Factory introduces significant class count. For a system with 3 product types and 3 families, you need:
- 1 abstract factory + 3 concrete factories = 4 classes
- 3 abstract products + 9 concrete products = 12 classes
- Total: 16 classes minimum

This is justified when:
1. **Families are real and mixing is dangerous** - cross-platform UIs, database drivers, rendering backends
2. **New families are expected** - you know you will add Linux support, or a new database backend
3. **The system is large and long-lived** - the upfront cost is amortized over years of maintenance
4. **Teams work on different families independently** - the Mac team and Windows team can work in parallel without conflicts

This is NOT justified when:
1. You have only one family today and "maybe" another later
2. Products are independent (mixing is fine)
3. The system is small or short-lived
4. You can refactor later without significant cost (no binary compatibility concerns)

---

## Adding New Products vs New Families

This is the key tradeoff of Abstract Factory:

**Adding a new family (e.g., LinuxFactory):**
- Create new ConcreteFactory class
- Create new ConcreteProduct for each product type
- Register new factory
- NO changes to existing code (Open/Closed Principle satisfied)

**Adding a new product type (e.g., createToolbar()):**
- Modify AbstractFactory interface (add new method)
- Modify ALL ConcreteFactory implementations
- Create new AbstractProduct + all ConcreteProducts
- This VIOLATES Open/Closed Principle

This asymmetry is fundamental. If your variation axis is "new families" (platforms, themes, vendors), Abstract Factory works beautifully. If your variation axis is "new product types," the pattern fights you.

---

## Common Misconceptions

1. **"Abstract Factory is just multiple Factory Methods"** - While it often uses Factory Methods internally, its PURPOSE is different: enforcing family consistency. The factory OBJECT (not subclass) is what varies.

2. **"You always need Abstract Factory for cross-platform code"** - For simple cases (one or two platform-specific objects), compile-time #ifdef with Factory Method is simpler and has zero runtime cost.

3. **"Abstract Factory guarantees product compatibility"** - It guarantees that products CREATED TOGETHER are from the same family. It cannot prevent you from mixing products obtained from different factory instances.

4. **"More abstraction is always better"** - Abstract Factory has a real cost in class explosion and conceptual complexity. Use it only when the family consistency guarantee provides genuine value.

---

## Why Does This Matter in System Design?

Abstract Factory patterns appear in large-scale system architecture as:
- **Cloud provider abstraction** - AWS, GCP, Azure families (compute + storage + network from one provider)
- **Database driver families** - connection + cursor + transaction from the same database
- **Serialization format families** - serializer + deserializer + schema validator for JSON, XML, Protobuf
- **Payment processing families** - payment + refund + verification from the same gateway (Stripe, PayPal)

Understanding when to use family-level abstraction vs individual factory methods helps you design systems that are portable without being over-engineered.

---

## Files

- [gui_factory.cpp](gui_factory.cpp) - Mac vs Windows widgets
- [db_factory.cpp](db_factory.cpp) - MySQL vs Postgres family

---

## Interview Questions

1. When to choose Abstract Factory over Factory Method?
2. How to add a new product type to an Abstract Factory? (Limitation)
3. How to add a new family/variant? (Strength)
4. Real-world: design cross-platform UI library using Abstract Factory.
5. Combine Abstract Factory with Singleton - good or bad?

**Advanced**
6. How does Abstract Factory relate to the Bridge pattern?
7. Design a rendering engine abstraction that supports DirectX and Vulkan families.
8. What is the "product matrix" problem and how does it limit Abstract Factory?
9. How would you implement Abstract Factory with template metaprogramming to avoid virtual dispatch overhead?
10. Compare Abstract Factory with a dependency injection container for managing product families.

---

## Daily Assignment

1. Design a cross-DB Abstract Factory: `Connection`, `Command`, `Transaction` for MySQL & Postgres.
2. Add a new family `MongoDB` - which classes change?
3. Add a new product `Cursor` - which classes change?
4. Implement a theme system (DarkTheme, LightTheme) where each theme produces a consistent family of UI elements (background color, font style, border style, icon set).
5. Design a game rendering backend factory that produces Texture, Shader, and Buffer objects for both OpenGL and Vulkan. Ensure the application code never touches rendering-API-specific types.
