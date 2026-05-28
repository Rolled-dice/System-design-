# Day 6 - Builder Pattern

## Overview

The Builder pattern separates the construction of a complex object from its representation. When an object has many constituent parts or configuration options, constructing it in one step becomes unwieldy. Builder provides a step-by-step construction process that can produce different representations of the same construction logic.

Modern software development has embraced Builder extensively. Fluent APIs, query builders, configuration objects, and protocol buffer message construction all use variations of this pattern. Understanding both the classical GoF formulation and modern fluent variations is essential.

---

## GoF Documentation

### Intent

Separate the construction of a complex object from its representation so that the same construction process can create different representations.

### Motivation

Consider a document converter that reads RTF (Rich Text Format) and converts it to multiple output formats (plain ASCII text, TeX, PDF). The reading algorithm is the same regardless of output format: parse the RTF tokens and produce output. But the output generation differs dramatically between formats.

The Builder pattern solves this by separating the parsing (Director) from the output generation (Builder). The parser (Director) calls builder methods like `convertCharacter()`, `convertParagraph()`, `convertFont()`. Different builders produce different outputs from the same sequence of calls. The Director does not know what kind of output is being generated.

### Applicability

Use Builder when:
- The algorithm for creating a complex object should be independent of the parts that make up the object and how they are assembled
- The construction process must allow different representations for the object that is constructed
- You need to construct objects step by step, with the ability to control each step
- Object construction involves many optional parameters and you want to avoid telescoping constructors

**Decision Checklist:**
1. Does the object have many parts or configuration options?
2. Is the construction process multi-step (not just "set all fields")?
3. Do you need multiple representations from the same construction logic?
4. Would a constructor with 10+ parameters be confusing?

### Structure

```
+--------------------+           +--------------------+
|     Director       |           |     Builder        |
+--------------------+           +--------------------+
| - builder: Builder*|---------->| + buildPartA()     |
+--------------------+           | + buildPartB()     |
| + construct()      |           | + buildPartC()     |
+--------------------+           | + getResult():     |
                                 |   Product          |
  Director::construct() {        +--------+-----------+
      builder->buildPartA();              ^
      builder->buildPartB();              |
      builder->buildPartC();     +--------+-----------+
  }                              |  ConcreteBuilder   |
                                 +--------------------+
                                 | - product: Product |
                                 +--------------------+
                                 | + buildPartA()     |
                                 | + buildPartB()     |
                                 | + buildPartC()     |
                                 | + getResult()      |
                                 +--------------------+

  // Usage:
  ConcreteBuilder builder;
  Director director(&builder);
  director.construct();
  Product p = builder.getResult();
```

### Participants

- **Builder** (TextConverter) - specifies an abstract interface for creating parts of a Product object
- **ConcreteBuilder** (ASCIIConverter, TeXConverter) - constructs and assembles parts of the product by implementing the Builder interface. Defines and keeps track of the representation it creates. Provides an interface to retrieve the product.
- **Director** (RTFReader) - constructs an object using the Builder interface. Knows the WHAT and WHEN of construction (the algorithm) but not the HOW (that is the Builder's job).
- **Product** (ASCIIText, TeXText) - represents the complex object under construction. Includes classes that define the constituent parts.

### Collaborations

1. The client creates the Director object and configures it with the desired Builder object
2. Director notifies the builder whenever a part of the product should be built
3. Builder handles requests from the director and adds parts to the product
4. The client retrieves the product from the builder

### Consequences

**Benefits:**
1. **Lets you vary a product's internal representation** - the Builder interface hides the product's structure. To change representation, just define a new Builder.
2. **Isolates code for construction and representation** - improves modularity. Clients do not need to know anything about the classes that define the product's internal structure.
3. **Gives finer control over the construction process** - unlike creational patterns that construct products in one shot, Builder constructs step by step. Only when the product is finished does the director retrieve it.
4. **Enforces construction constraints** - the builder can validate each step, reject invalid configurations, and ensure the product is in a consistent state when returned.

**Costs:**
1. **More complex than direct construction** - for simple objects, a constructor is clearer
2. **Tight coupling between Director and Builder interface** - if the construction process changes, the Builder interface may need to change
3. **Product is not type-safe until complete** - partial products may be in invalid states during construction

### Implementation Details (C++ Specific)

**Builder with method chaining (modern C++):**
```cpp
class HttpRequest {
    std::string method_, url_, body_;
    std::map<std::string, std::string> headers_;
    friend class HttpRequestBuilder;
    HttpRequest() = default;  // Only Builder can construct
};

class HttpRequestBuilder {
    HttpRequest request_;
public:
    HttpRequestBuilder& method(std::string m) { request_.method_ = std::move(m); return *this; }
    HttpRequestBuilder& url(std::string u) { request_.url_ = std::move(u); return *this; }
    HttpRequestBuilder& header(std::string k, std::string v) { 
        request_.headers_[std::move(k)] = std::move(v); return *this; 
    }
    HttpRequestBuilder& body(std::string b) { request_.body_ = std::move(b); return *this; }
    HttpRequest build() { return std::move(request_); }
};
```

**Returning `unique_ptr` for heap products:**
```cpp
std::unique_ptr<Product> ConcreteBuilder::getResult() {
    return std::move(product_);
}
```

### Known Uses

- **StringBuilder** (Java, C#) - builds strings incrementally, avoids O(n^2) concatenation
- **Protobuf message builders** - `Message::Builder` in Protocol Buffers
- **HTTP client libraries** - OkHttp RequestBuilder, libcurl easy handle configuration
- **SQL query builders** - build queries step by step without string concatenation
- **gRPC channel builders** - configure channel with credentials, interceptors, load balancing
- **Test data builders** - create complex test fixtures with sensible defaults

### Related Patterns

- **Abstract Factory** - similar in that it creates complex objects. Key difference: Builder focuses on constructing a complex object step by step, returning the product as a final step. Abstract Factory returns the product immediately.
- **Composite** - builders often produce Composite structures
- **Factory Method** - can be used within a Builder to decide which classes to use for building parts
- **Singleton** - builders themselves are rarely Singletons (they maintain state for one product)

---

## The Telescoping Constructor Problem

### What Is It?

When a class has many optional parameters, you end up with many constructor overloads:

```
Pizza(size)
Pizza(size, cheese)
Pizza(size, cheese, pepperoni)
Pizza(size, cheese, pepperoni, onion)
Pizza(size, cheese, pepperoni, onion, bacon)
Pizza(size, cheese, pepperoni, onion, bacon, mushroom)
// ... 2^n combinations
```

**Problems:**
- Exponential number of constructors
- Easy to swap parameters of the same type (pass `onion` in the `bacon` position)
- Cannot skip optional parameters in the middle
- Unreadable call sites: `Pizza(12, true, false, true, false, true)` - what does each `bool` mean?

### How Builder Solves It

```
Pizza pizza = PizzaBuilder(12)
    .addCheese("mozzarella")
    .addTopping("onion")
    .addTopping("mushroom")
    .build();
```

Each step is named, order does not matter for independent options, and you can skip anything you do not want. The call site is self-documenting.

---

## Fluent Interface Pattern

### What Is It?

A fluent interface is an API designed for method chaining, where each method returns `this` (or a reference to the builder) so calls can be chained:

```cpp
auto request = HttpRequest::builder()
    .method("POST")
    .url("https://api.example.com/users")
    .header("Content-Type", "application/json")
    .body("{\"name\": \"Alice\"}")
    .timeout(30)
    .build();
```

### Design Considerations

- **Return type:** Return `Builder&` for chaining. For the final step, return the product.
- **Immutability of builder:** Should `build()` consume the builder (move semantics) or leave it reusable? Both approaches have valid use cases.
- **Validation:** `build()` is the natural place to validate that all required fields are set and that the combination of options is valid.
- **Copy vs move:** Return the product by value (move semantics) for efficiency.

### Advanced: Type-Safe Builders (Phantom Types)

Some languages (Rust, Scala) support builders that enforce required fields at compile time using type states. In C++, you can approximate this with templates:

```cpp
template<bool HasUrl, bool HasMethod>
class RequestBuilder { ... };

// Only RequestBuilder<true, true> has a build() method
// Calling .url() transforms RequestBuilder<false, X> to RequestBuilder<true, X>
```

This prevents forgetting required fields, catching errors at compile time rather than runtime.

---

## The Director: When to Use and When to Skip

### What the Director Does

The Director encapsulates a particular construction algorithm. It calls builder methods in a specific order to produce a specific product configuration:

```cpp
class MealDirector {
public:
    void constructVegMeal(MealBuilder& builder) {
        builder.addBurger("veggie");
        builder.addDrink("water");
        builder.addSide("salad");
    }
    void constructKidsMeal(MealBuilder& builder) {
        builder.addBurger("mini");
        builder.addDrink("juice");
        builder.addSide("fries");
        builder.addToy("car");
    }
};
```

### When to Use a Director

- You have multiple named construction algorithms (build configurations, presets)
- The construction algorithm is complex and should not be duplicated across clients
- The Director can be reused with different Builders (same algorithm, different output)

### When to Skip the Director (Modern Practice)

In modern code, the Director is often omitted. The client code itself acts as the Director, calling builder methods directly. This is appropriate when:
- Each usage site has a unique construction sequence
- The construction is simple enough that encapsulating it adds no value
- You are building test data where each test has different needs
- The fluent interface is self-explanatory without a Director

Most modern Builder usage (HTTP request builders, query builders, configuration builders) operates without a formal Director class.

---

## Immutable Object Construction

### The Problem

Immutable objects (objects whose state cannot change after construction) are desirable for thread safety and correctness. But immutable objects with many fields require all fields to be set at construction time, leading back to the telescoping constructor problem.

### Builder as the Solution

```cpp
class User {
    const std::string id_;      // Required
    const std::string email_;   // Required
    const std::string name_;    // Optional
    const int age_;             // Optional
    const std::string address_; // Optional
    
    friend class UserBuilder;
    User(std::string id, std::string email, std::string name, int age, std::string address)
        : id_(std::move(id)), email_(std::move(email)), name_(std::move(name)),
          age_(age), address_(std::move(address)) {}
          
public:
    const std::string& id() const { return id_; }
    const std::string& email() const { return email_; }
    // ... no setters - object is immutable
};

class UserBuilder {
    std::string id_, email_, name_, address_;
    int age_ = 0;
public:
    UserBuilder(std::string id, std::string email)  // Required params in constructor
        : id_(std::move(id)), email_(std::move(email)) {}
    
    UserBuilder& name(std::string n) { name_ = std::move(n); return *this; }
    UserBuilder& age(int a) { age_ = a; return *this; }
    UserBuilder& address(std::string a) { address_ = std::move(a); return *this; }
    
    User build() {
        return User(std::move(id_), std::move(email_), std::move(name_), age_, std::move(address_));
    }
};
```

The Builder is mutable and collects configuration over time. The final `build()` call creates an immutable product. This separates the mutable construction phase from the immutable usage phase.

---

## Real-World Builder Examples

### StringBuilder (Java/C#)

The most common Builder in practice. String concatenation creates a new string each time (O(n) per concatenation, O(n^2) total). StringBuilder accumulates parts and builds the final string in one allocation:

```
StringBuilder sb;
sb.append("Hello").append(", ").append(name).append("!");
std::string result = sb.toString();  // One allocation
```

### HTTP Request Builders

Every modern HTTP library uses builders:
- OkHttp (Java): `new Request.Builder().url(...).header(...).post(body).build()`
- libcurl (C): `curl_easy_setopt(handle, CURLOPT_URL, "...")` (imperative builder)
- Boost.Beast (C++): request objects configured step by step

### SQL Query Builders

Prevent SQL injection and improve readability:
```
auto query = QueryBuilder::select({"name", "email"})
    .from("users")
    .where("age > ?", 18)
    .orderBy("name", ASC)
    .limit(10)
    .build();
```

### Protocol Buffer Message Builders

```
Person person = Person::newBuilder()
    .setName("Alice")
    .setId(123)
    .setEmail("alice@example.com")
    .addPhone(Phone::newBuilder().setNumber("555-1234").setType(MOBILE))
    .build();
```

---

## Builder vs Factory Patterns

| Aspect | Builder | Factory (Method/Abstract) |
|--------|---------|---------------------------|
| Focus | Step-by-step construction | Instantiation decision |
| Complexity | Complex objects with many parts | Objects with variation in type |
| When called | Multiple method calls | Single method call |
| Return | One complex product | One of many product types |
| Typical use | Configuration, query building | Polymorphic creation |
| Product variety | Same type, different config | Different types, same interface |

**Rule of thumb:** Use Factory when the question is "WHICH class to create?" Use Builder when the question is "HOW to configure a complex instance?"

---

## Common Misconceptions

1. **"Builder is only for objects with many parameters"** - While telescoping constructors are a common motivation, Builder's original purpose (GoF) is separating construction algorithm from representation. The two concerns happen to overlap.

2. **"Builder always needs a Director"** - Modern usage often omits the Director. The client code itself drives the construction. This is perfectly valid.

3. **"Fluent interface = Builder pattern"** - Fluent interfaces are a style of API design (method chaining). Builder is a design pattern. They often coincide but are not synonymous. You can have a non-fluent Builder or a fluent non-Builder.

4. **"Builder is always better than a constructor"** - For simple objects with 2-3 required fields and no optional ones, a constructor is simpler and clearer. Builder adds complexity.

5. **"Builder makes objects immutable"** - Builder does not enforce immutability. The product class must be designed as immutable independently. Builder just makes it practical to construct immutable objects with many fields.

---

## Why Does This Matter in System Design?

Builder appears frequently in system design:
- **Configuration management** - building complex configuration objects for services, databases, and caches
- **Request/Response construction** - HTTP requests, gRPC messages, GraphQL queries
- **Pipeline construction** - data processing pipelines built step by step (Apache Beam, Spark)
- **Infrastructure as Code** - Terraform resources, CloudFormation templates built programmatically
- **Test fixtures** - building complex test data with sensible defaults and specific overrides

In LLD interviews, Builder often appears in:
- "Design a URL shortener" (URL builder with optional parameters)
- "Design an email service" (email message builder)
- "Design a query engine" (query builder)

Understanding Builder helps you design APIs that are both powerful (many options) and usable (self-documenting, hard to misuse).

---

## Files

- [pizza_builder.cpp](pizza_builder.cpp) - Fluent Builder
- [http_request_builder.cpp](http_request_builder.cpp) - Real-world HTTP request

---

## Interview Questions

1. Builder vs Factory - when to choose which?
2. Why does immutable object construction benefit from Builder?
3. How does Builder solve the telescoping constructor problem?
4. Director's role in classic Builder.
5. How would you build an SQL query builder?

**Advanced**
6. How would you make a Builder thread-safe for concurrent construction?
7. Design a Builder that validates at compile time (type-state pattern).
8. When should `build()` throw vs return an error type?
9. How does the Builder pattern relate to the concept of "staged initialization"?
10. Compare Builder with named parameters (where the language supports them).

---

## Daily Assignment

1. Implement a `Burger` builder with optional toppings (bun type, patty, cheese, sauces).
2. Build a `SQLQueryBuilder`: `select(...).from(...).where(...).orderBy(...).build()`.
3. Implement immutable `User` with required (id, email) and optional (name, age, address) fields.
4. Create a `PipelineBuilder` that constructs a data processing pipeline: `read(source).filter(predicate).transform(func).write(sink).build()`. The pipeline should be executable.
5. Implement the Director pattern: create a `MealDirector` that can construct predefined meals (vegetarian, kids, standard) using a `MealBuilder` interface.
