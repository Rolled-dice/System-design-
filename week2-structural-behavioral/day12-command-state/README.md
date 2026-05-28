# Day 12 - Command + State

## Command
**Intent**: Encapsulate a request as an object, parameterizing clients with different requests, queue, log, and undo.

**Use cases**: Undo/redo, task queues, macro recording, transactional ops.

## State
**Intent**: Allow an object to alter its behavior when its internal state changes; appears as if it changed class.

**Use cases**: Workflow engines, vending machines, TCP connection states, traffic lights, order lifecycle.

## Files
- [command_undo.cpp](command_undo.cpp) - Text editor with undo
- [command_remote.cpp](command_remote.cpp) - Remote control mapping
- [state_vending.cpp](state_vending.cpp) - Vending machine

## Interview Questions
1. Command vs Strategy.
2. How to implement undo/redo using Command stack?
3. State vs Strategy - both polymorphic.
4. State pattern vs giant if/switch on enum - trade-offs.
5. Real-world: model an order's lifecycle (placed -> paid -> shipped -> delivered) using State.

## Daily Assignment
1. Command: text editor with `WriteCommand`, `DeleteCommand`, undo stack of size 100.
2. Command: macro recorder - bind keys to commands, replay sequence.
3. State: build a `Document` with `Draft / Moderation / Published / Archived` states; transitions controlled by state classes.
