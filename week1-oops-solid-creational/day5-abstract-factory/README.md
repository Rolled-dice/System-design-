# Day 5 - Abstract Factory Pattern

## Intent
Provide an interface for creating **families of related objects** without specifying their concrete classes.

## Difference from Factory Method
| Factory Method | Abstract Factory |
|----------------|------------------|
| Creates ONE product | Creates a FAMILY of products |
| Uses inheritance | Uses object composition |
| `createDocument()` | `createButton()` + `createCheckbox()` |

## When to Use
- Cross-platform UI toolkits (Mac vs Windows widgets)
- DB driver families (MySQL connection + query builder + transaction)
- Theme systems (Dark family vs Light family components)

## Files
- [gui_factory.cpp](gui_factory.cpp) - Mac vs Windows widgets
- [db_factory.cpp](db_factory.cpp) - MySQL vs Postgres family

## Interview Questions
1. When to choose Abstract Factory over Factory Method?
2. How to add a new product type to an Abstract Factory? (Limitation)
3. How to add a new family/variant? (Strength)
4. Real-world: design cross-platform UI library using Abstract Factory.
5. Combine Abstract Factory with Singleton - good or bad?

## Daily Assignment
1. Design a cross-DB Abstract Factory: `Connection`, `Command`, `Transaction` for MySQL & Postgres.
2. Add a new family `MongoDB` - which classes change?
3. Add a new product `Cursor` - which classes change?
