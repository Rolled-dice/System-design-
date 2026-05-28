# Day 2 - SOLID Principles (C++)

| # | Principle | One-line |
|---|-----------|----------|
| **S** | Single Responsibility | A class should have one reason to change |
| **O** | Open/Closed | Open for extension, closed for modification |
| **L** | Liskov Substitution | Subtypes must be substitutable for their base types |
| **I** | Interface Segregation | Many client-specific interfaces > one fat interface |
| **D** | Dependency Inversion | Depend on abstractions, not concretions |

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

## Daily Assignment

1. Take a `God class` `EmployeeManager` that:
   - Saves employees to DB
   - Sends emails
   - Generates PDF reports
   - Computes salary
   Refactor following all 5 SOLID principles. Provide before/after C++ code.
2. Identify SOLID violations in your last project's code and document fixes.
