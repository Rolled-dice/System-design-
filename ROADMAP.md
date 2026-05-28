# 12-Week System Design Roadmap (Original)

The full 12-week schedule. The repo's hands-on C++ track in [README.md](README.md) compresses this into 4 weeks; this doc maps each week to that track for progress tracking.

| Month | Week | Topics | Maps to (4-week C++ track) | Done |
|-------|------|--------|----------------------------|------|
| **M1** | **W1 - CS Basics** | OS (Processes, Threads, Memory), Networking (TCP/IP, HTTP, DNS), DS, Algos (Big O, Sort, Search) | _Prereq - skip if known_ | [ ] |
| **M1** | **W2 - Networking Essentials** | Load Balancing, HTTP methods/codes, Caching, CDN | [Day 15](week3-hld-foundation/day15-networking-loadbalancing/), [Day 16](week3-hld-foundation/day16-caching-cdn/) | [ ] |
| **M1** | **W3 - DBs & Distributed Systems** | RDBMS vs NoSQL, CAP, Sharding, Replication | [Day 17](week3-hld-foundation/day17-databases-sql-nosql/), [Day 18](week3-hld-foundation/day18-sharding-replication-cap/) | [ ] |
| **M1** | **W4 - OS & Storage** | File systems, Virtualization, Containers, Disk I/O, DFS | _Prereq - covered in Day 17_ | [ ] |
| **M2** | **W5 - LB & Scaling** | H/V scaling, Auto-scaling, Health checks, Rate limiting | [Day 15](week3-hld-foundation/day15-networking-loadbalancing/), [Day 20](week3-hld-foundation/day20-rate-limiting/) | [ ] |
| **M2** | **W6 - Caching Systems** | Cache types, Eviction, Redis, Memcached, CDN | [Day 16](week3-hld-foundation/day16-caching-cdn/) | [ ] |
| **M2** | **W7 - DBs Deep Dive** | ACID vs BASE, SQL/NoSQL, Indexing, CAP, Consistency | [Day 17](week3-hld-foundation/day17-databases-sql-nosql/), [Day 18](week3-hld-foundation/day18-sharding-replication-cap/) | [ ] |
| **M2** | **W8 - Queues & Messaging** | Kafka, RabbitMQ, Pub/Sub, DLQ, Event-Driven | [Day 19](week3-hld-foundation/day19-messaging-queues/) | [ ] |
| **M3** | **W9 - Design Thinking & Resilience** | 4-step approach, bottlenecks, stateless/stateful, circuit breakers | [Day 21](week3-hld-foundation/day21-consistent-hashing/), Week 4 case studies | [ ] |
| **M3** | **W10 - Patterns & Operations** | Token/Leaky Bucket, Circuit Breaker, Leader Election, CQRS, Health Checks | [Day 20](week3-hld-foundation/day20-rate-limiting/), [Day 21](week3-hld-foundation/day21-consistent-hashing/) | [ ] |
| **M3** | **W11 - Practice Questions** | Twitter, Uber, etc. - whiteboard + mock | [Week 4](week4-case-studies/) | [ ] |
| **M3** | **W12 - Mock Interviews & Review** | Peer/expert mocks, frameworks review | [interview-bank/](interview-bank/) | [ ] |

## How to Track Progress

1. Fork this repository.
2. Tick `[ ]` -> `[x]` in this file and in each `dayXX/README.md` as you finish.
3. Compile and run the C++ examples for each day.
4. Solve the daily assignment in your own file alongside `README.md`.
5. After each week, write a short reflection in `weekN-notes.md` (template below).

## Weekly Notes Template

Create a file `weekN-notes.md` at the start of each week:

```markdown
# Week N - <topic> Notes

## Concepts I learned
- ...

## Patterns I implemented
- ...

## Interview questions I can now answer
- ...

## Things I still need to revisit
- ...

## Mini-project / assignment outcome
- ...
```
