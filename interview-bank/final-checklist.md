# Final Readiness Checklist

Tick when you can explain + implement (LLD) or whiteboard (HLD) the topic from memory.

## OOPS in C++

- [ ] Difference between abstract class and interface (in C++ specifically)
- [ ] Why virtual destructors are needed
- [ ] Diamond problem + virtual inheritance
- [ ] vtable + vptr layout
- [ ] Static vs dynamic polymorphism trade-offs
- [ ] RAII + smart pointers (`unique_ptr`, `shared_ptr`, `weak_ptr`)
- [ ] Rule of 0 / 3 / 5
- [ ] Move semantics (`std::move`, rvalue refs)

## SOLID

- [ ] Single Responsibility - find SRP violation in a class
- [ ] Open/Closed via Strategy
- [ ] Liskov - Square/Rectangle redesign
- [ ] Interface Segregation - printer/scanner example
- [ ] Dependency Inversion - constructor injection

## Creational Patterns

- [ ] Singleton (Meyer's, double-checked locking)
- [ ] Singleton drawbacks + alternatives (DI)
- [ ] Factory Method
- [ ] Abstract Factory (UI / DB driver families)
- [ ] Builder (vs telescoping constructor)
- [ ] Prototype (clone + registry)

## Structural Patterns

- [ ] Adapter (legacy library)
- [ ] Decorator (coffee, IO streams)
- [ ] Facade vs Adapter vs Mediator
- [ ] Proxy (virtual, protection, remote, caching)
- [ ] Composite (filesystem)
- [ ] Bridge (Shape x Renderer)
- [ ] Flyweight (text glyphs)

## Behavioral Patterns

- [ ] Observer (with `weak_ptr` to avoid leaks)
- [ ] Strategy (sort, payment)
- [ ] Command (undo/redo)
- [ ] State (vending, order lifecycle)
- [ ] Iterator (custom container)
- [ ] Template Method (data processor)
- [ ] Chain of Responsibility (HTTP middleware)
- [ ] Mediator (chat room)
- [ ] Visitor (AST eval + print)
- [ ] Memento (editor undo)

## HLD Building Blocks

- [ ] L4 vs L7 load balancing
- [ ] LB algorithms: RR, weighted, least-conn, IP-hash, consistent-hash
- [ ] CDN pull vs push
- [ ] Cache strategies: aside, through, behind, refresh-ahead
- [ ] Eviction: LRU, LFU, TTL
- [ ] Cache stampede + mitigations
- [ ] SQL vs NoSQL families (KV, doc, wide-col, graph)
- [ ] Indexes: B-tree, hash, bitmap, inverted, geo, LSM
- [ ] Isolation levels (RU, RC, RR, Serializable)
- [ ] CAP + PACELC
- [ ] Sharding: range, hash, consistent, geo
- [ ] Replication: leader-follower, multi-leader, leaderless, quorum
- [ ] Kafka vs RabbitMQ
- [ ] At-most/least/exactly-once delivery
- [ ] Idempotency, transactional outbox
- [ ] Backpressure
- [ ] Rate limiting: token, leaky, fixed window, sliding window
- [ ] Distributed rate limiter design
- [ ] Consistent hashing + virtual nodes
- [ ] Circuit breaker states + tuning

## HLD Case Studies (be able to whiteboard in 45 min)

- [ ] URL Shortener
- [ ] Twitter / Threads timeline
- [ ] Uber matching + driver index
- [ ] WhatsApp messaging
- [ ] YouTube upload + playback
- [ ] Instagram feed
- [ ] Dropbox
- [ ] Web Crawler
- [ ] Search autocomplete
- [ ] Rate limiter

## LLD Case Studies (be able to code core classes in 45 min)

- [ ] Parking Lot
- [ ] Splitwise
- [ ] BookMyShow
- [ ] Tic-Tac-Toe + Snake & Ladder
- [ ] Vending Machine / ATM
- [ ] Logger
- [ ] LRU + LFU caches
- [ ] Notification system
- [ ] Elevator
- [ ] Online cart with discounts

## Mock Interview Self-Rating

Rate yourself 1-5 after each mock:

| Mock # | Date | Problem | Score | Weakness |
|--------|------|---------|-------|----------|
| 1 | | | | |
| 2 | | | | |
| 3 | | | | |
| 4 | | | | |
| 5 | | | | |

## Day-of Tips

- [ ] Always clarify scope before designing
- [ ] State assumptions explicitly
- [ ] Calculate before designing - don't skip estimation
- [ ] Draw, don't just talk
- [ ] When stuck, name a trade-off and pick a side
- [ ] Don't over-engineer; you can extend if asked
- [ ] Identify single points of failure proactively
- [ ] Discuss monitoring + observability briefly
- [ ] Save 5 min for "what would you do differently with more time?"
