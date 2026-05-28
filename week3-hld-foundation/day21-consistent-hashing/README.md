# Day 21 - Consistent Hashing + Circuit Breaker

## Consistent Hashing

### Problem
Naive `hash(key) % N` requires re-hashing nearly all keys when N changes. Bad for:
- Distributed caches (Memcached, Cassandra)
- CDN routing
- Sticky sessions

### Idea
Map nodes and keys onto a circular hash ring (0 ... 2^32). For a key, walk clockwise to the next node.

Adding/removing a node only re-shuffles **K/N** keys on average.

### Virtual Nodes
A single physical node = many virtual node positions on the ring. Improves uniform distribution and reduces variance.

## Circuit Breaker

### Problem
A failing downstream service drags your service down (threads, memory, latency). Cascading failures.

### State Machine
| State | Behavior |
|-------|----------|
| **CLOSED** | All requests pass through; track failures |
| **OPEN** | Fast-fail without calling downstream; trip when failure threshold hit |
| **HALF_OPEN** | After timeout, allow trial requests; if success -> CLOSED, else OPEN |

### Tuning
- Failure threshold (e.g., 50% over last 20 requests)
- Open duration (e.g., 10s)
- Timeout per request

## Files
- [consistent_hash.cpp](consistent_hash.cpp) - ring with virtual nodes
- [circuit_breaker.cpp](circuit_breaker.cpp)

## Interview Questions
1. Why is `hash % N` bad in dynamic clusters?
2. How do virtual nodes help in consistent hashing?
3. Real-world: how does Cassandra route writes? (consistent hashing + replication)
4. What is data skew? How does virtual nodes reduce it?
5. Circuit breaker - state transitions on success and failure?
6. Difference between Circuit Breaker and Retry?
7. When to use Bulkhead pattern with Circuit Breaker?
8. How to implement circuit breaker in distributed system without shared state?

## Daily Assignment
1. Implement consistent hashing with virtual nodes; remove a node, count migrations.
2. Implement circuit breaker with three states. Wrap a flaky function and observe transitions.
3. Combine: route to N backends via consistent hash; if a backend trips its circuit, route to next on ring.
