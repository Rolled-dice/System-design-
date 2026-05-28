# Day 10 - Composite, Bridge, Flyweight

## Composite
**Intent**: Treat individual objects and groups of objects uniformly via a tree structure.

**Use cases**: File systems (file/folder), UI hierarchies (Panel containing Button + Panel), org charts.

## Bridge
**Intent**: Decouple **abstraction** from its **implementation** so they can vary independently. Avoids cartesian explosion of classes.

**Use cases**: Cross-platform rendering (Shape x Renderer), DB drivers (Repo x DBImpl), remote control x device.

## Flyweight
**Intent**: Use sharing to support large numbers of fine-grained objects efficiently.

**Use cases**: Text editor characters (font/style shared), game sprites, particle systems.

## Files
- [composite_filesystem.cpp](composite_filesystem.cpp)
- [bridge_renderer.cpp](bridge_renderer.cpp)
- [flyweight_text.cpp](flyweight_text.cpp)

## Interview Questions
1. Composite vs Decorator - both recursive. What differs?
2. Bridge vs Adapter - both connect interfaces.
3. Bridge vs Strategy - both compose. What differs? (Hint: lifetime + abstraction layer)
4. How does Flyweight reduce memory? Intrinsic vs extrinsic state.
5. Real-world: design a text editor's rendering using Flyweight.

## Daily Assignment
1. Composite: build `FileSystemNode` -> `File` and `Directory`. Compute total size recursively.
2. Bridge: `Shape` (Circle/Square) x `Renderer` (Vector/Raster) - 4 combinations, no class explosion.
3. Flyweight: text editor where 1M characters share ~50 unique glyphs (font, size, color).
