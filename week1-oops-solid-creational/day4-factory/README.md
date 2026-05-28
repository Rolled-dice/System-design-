# Day 4 - Factory Method Pattern

## Intent
Define an interface for creating an object, but let subclasses decide which class to instantiate.

## When to Use
- Object creation logic is complex or varies by input
- You want to decouple client code from concrete classes
- Plugin-style architecture

## Files
- [factory_simple.cpp](factory_simple.cpp) - Simple factory (not a true GoF pattern but common)
- [factory_method.cpp](factory_method.cpp) - True Factory Method
- [logistics_factory.cpp](logistics_factory.cpp) - Real-world: shipping logistics

## Interview Questions
1. Difference between Simple Factory, Factory Method, and Abstract Factory.
2. How does Factory Method support OCP?
3. Where would you use Factory Method in a payment system?
4. Factory Method vs Strategy - both use polymorphism. What differs?
5. How to register factories dynamically (factory registry / self-registration)?

## Daily Assignment
1. Build a `ShapeFactory` that creates `Circle/Square/Triangle` from a string input.
2. Build a `NotificationFactory` that returns `Email/SMS/Push` notifiers based on user preference.
3. Implement a self-registering factory using a static registry map.
