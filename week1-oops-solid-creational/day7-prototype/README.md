# Day 7 - Prototype Pattern

## Intent
Create new objects by **cloning** an existing instance instead of creating from scratch.

## When to Use
- Object creation is expensive (DB load, complex computation)
- Avoid subclassing factories - clone existing config instead
- Game engines (clone enemy templates, particle effects)
- Editors (copy/paste of complex objects)

## Deep vs Shallow Clone
- **Shallow**: copies pointers, shared sub-objects
- **Deep**: copies sub-objects too

## Files
- [shape_prototype.cpp](shape_prototype.cpp)
- [game_unit_clone.cpp](game_unit_clone.cpp) - Real-world: game unit registry

## Interview Questions
1. When does cloning beat constructing?
2. How does C++ copy constructor relate to Prototype? Is it the same?
3. Deep vs shallow clone - when is each appropriate?
4. How to clone polymorphically through a base pointer? (`virtual clone()`)
5. Combine Prototype with Registry - design a registry that returns copies.

## Daily Assignment
1. Build a `Document` with sections, each section with paragraphs. Implement deep clone.
2. Build a `GameUnitRegistry` storing prototype warriors/archers/mages. `spawn(name)` returns clones.
3. Use `unique_ptr<Base> clone() const` on a polymorphic hierarchy.
