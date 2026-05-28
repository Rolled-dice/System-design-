# Day 6 - Builder Pattern

## Intent
Separate the **construction** of a complex object from its **representation**, so the same construction process can create different representations.

## When to Use
- Object has many optional parameters (telescoping constructor anti-pattern)
- Step-by-step construction
- Build different representations (JSON, XML, HTML from same data)

## Variants
- **Classic GoF Builder** with Director
- **Fluent Builder** (modern, method chaining)

## Files
- [pizza_builder.cpp](pizza_builder.cpp) - Fluent Builder
- [http_request_builder.cpp](http_request_builder.cpp) - Real-world HTTP request

## Interview Questions
1. Builder vs Factory - when to choose which?
2. Why does immutable object construction benefit from Builder?
3. How does Builder solve the telescoping constructor problem?
4. Director's role in classic Builder.
5. How would you build an SQL query builder?

## Daily Assignment
1. Implement a `Burger` builder with optional toppings (bun type, patty, cheese, sauces).
2. Build a `SQLQueryBuilder`: `select(...).from(...).where(...).orderBy(...).build()`.
3. Implement immutable `User` with required (id, email) and optional (name, age, address) fields.
