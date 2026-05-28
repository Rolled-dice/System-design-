# Day 8 - Adapter + Decorator

## Adapter

### Intent
Convert the interface of a class into another interface clients expect. Lets incompatible interfaces work together.

### When to Use
- Integrating a 3rd-party legacy library with your code
- Wrapping a C library in a C++ interface
- Different vendors' SDKs with similar functionality

### Files
- [adapter.cpp](adapter.cpp) - Old logger to new ILogger interface

## Decorator

### Intent
Attach additional responsibilities to an object **dynamically** without modifying its class. Alternative to subclassing.

### When to Use
- Adding cross-cutting concerns (logging, caching, authentication)
- Stream processing (encryption, compression layers)
- UI: scrollbars, borders on widgets

### Files
- [decorator_coffee.cpp](decorator_coffee.cpp) - Classic coffee with milk/sugar
- [decorator_stream.cpp](decorator_stream.cpp) - Real: data stream with encryption + compression

## Interview Questions
1. Adapter vs Decorator vs Proxy - all wrap; what differs?
2. Object Adapter vs Class Adapter (multiple inheritance) in C++.
3. Why is Decorator preferred over inheritance for cross-cutting concerns?
4. How do middleware chains in web frameworks use Decorator?
5. Real-world: design IO streams (Java's `BufferedReader(new FileReader(...))`).

## Daily Assignment
1. Adapter: wrap a `LegacyXmlParser` (returns `string`) into a modern `IDataParser` returning `JsonObject`.
2. Decorator: implement `ICoffee` with `Espresso` base, add `MilkDecorator`, `SugarDecorator`, `WhipDecorator`. Each adds price + description.
3. Build a stream pipeline: `RawStream -> CompressDecorator -> EncryptDecorator`.
