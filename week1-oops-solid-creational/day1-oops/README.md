# Day 1 - OOPS Concepts (C++)

## 4 Pillars of OOPS

### 1. Encapsulation
Bundling data + methods that operate on them; hide internal state via `private`.

### 2. Inheritance
Reuse + extend behavior. C++ supports single, multi-level, multiple, hierarchical, and hybrid inheritance.

### 3. Polymorphism
- **Compile-time** (function/operator overloading, templates)
- **Runtime** (virtual functions, vtable)

### 4. Abstraction
Show *what*, hide *how*. Achieved via abstract classes (pure virtual) and interfaces.

---

## Key C++ Concepts You Must Know

| Concept | Use |
|---------|-----|
| `virtual` / `override` / `final` | Runtime polymorphism |
| Pure virtual `= 0` | Abstract class |
| `virtual ~Base()` | Avoid memory leaks via base pointer |
| `friend` | Controlled access break |
| `static` member | Class-level state |
| `const` correctness | Immutable methods/data |
| Smart pointers (`unique_ptr`, `shared_ptr`) | Avoid raw `new`/`delete` |
| Rule of 3 / 5 / 0 | Resource management |
| `explicit` | Prevent unwanted implicit conversions |

---

## Code Walkthroughs

- [encapsulation.cpp](encapsulation.cpp)
- [inheritance.cpp](inheritance.cpp)
- [polymorphism.cpp](polymorphism.cpp)
- [abstraction.cpp](abstraction.cpp)

---

## Interview Questions

**LLD-style**
1. Difference between abstract class and interface in C++?
2. Why does C++ need a virtual destructor?
3. What is the diamond problem? How does virtual inheritance solve it?
4. Difference between `override` and `final`.
5. When does the compiler create a vtable?
6. Can a constructor be virtual? Why/why not?
7. Difference between `static` and `dynamic` polymorphism with cost trade-offs.
8. Explain RAII with an example.

**Conceptual**
9. Encapsulation vs Abstraction - draw boundaries clearly.
10. How does C++ implement multiple inheritance memory layout?

---

## Daily Assignment

1. Implement a `Shape` hierarchy: `Shape -> Circle/Rectangle/Triangle` with `area()` and `perimeter()`. Use a `vector<unique_ptr<Shape>>` and compute total area polymorphically.
2. Add a `Drawable` interface (pure abstract) and make `Circle` inherit both `Shape` and `Drawable`. Demonstrate diamond resolution if you also add `Printable -> Drawable`.
3. Write a class `BankAccount` showing encapsulation: private balance, public deposit/withdraw with validation, throw on overdraft.
