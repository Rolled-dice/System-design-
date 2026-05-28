# Day 9 - Facade + Proxy

## Facade
**Intent**: Provide a unified, simple interface to a set of complex subsystems.

**Use cases**: Hiding library complexity, simplifying onboarding APIs, microservice gateways.

## Proxy
**Intent**: Provide a surrogate or placeholder for another object to control access to it.

**Variants**:
- **Virtual Proxy** - lazy initialization
- **Protection Proxy** - access control
- **Remote Proxy** - represents object in another address space (RPC stub)
- **Caching Proxy** - cache results
- **Smart Reference** - C++ `shared_ptr` is essentially a smart proxy

## Files
- [facade.cpp](facade.cpp) - Home theater facade
- [proxy_virtual.cpp](proxy_virtual.cpp) - Lazy image loading
- [proxy_protection.cpp](proxy_protection.cpp) - RBAC access proxy

## Interview Questions
1. Facade vs Adapter - both wrap. What differs?
2. Facade vs Mediator.
3. Proxy vs Decorator.
4. How would you cache a remote service call with Proxy?
5. Real-world: design API gateway as Facade.

## Daily Assignment
1. Build a `HomeTheaterFacade` controlling Amplifier, DVDPlayer, Projector, Lights, with `watchMovie()` and `endMovie()`.
2. Implement a virtual proxy for a high-resolution image - load only on first display.
3. Implement a protection proxy that blocks `delete()` for non-admin users.
