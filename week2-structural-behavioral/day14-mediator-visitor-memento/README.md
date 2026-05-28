# Day 14 - Mediator + Visitor + Memento

## Mediator
**Intent**: Define an object that encapsulates how a set of objects interact - reduces coupling.

**Use cases**: Chat rooms, ATC, UI form coordination, microservice orchestration.

## Visitor
**Intent**: Represent an operation to be performed on elements of an object structure - lets you define a new operation without changing the classes.

**Use cases**: AST traversal, compiler passes, file system scanners.

## Memento
**Intent**: Capture and externalize an object's internal state without violating encapsulation, so the object can be restored later.

**Use cases**: Undo/redo, snapshots, save/load game state.

## Files
- [mediator_chat.cpp](mediator_chat.cpp)
- [visitor_ast.cpp](visitor_ast.cpp)
- [memento_editor.cpp](memento_editor.cpp)

## Interview Questions
1. Mediator vs Observer.
2. Mediator vs Facade.
3. Visitor vs Iterator.
4. Visitor double-dispatch - explain.
5. Memento + Command - undo/redo combo.
6. How does Memento preserve encapsulation?

## Daily Assignment
1. Mediator: chat room with private/group messages.
2. Visitor: AST with `Number, Add, Mul` nodes - implement `EvalVisitor` and `PrintVisitor`.
3. Memento: text editor with snapshots, undo last 5 edits.
