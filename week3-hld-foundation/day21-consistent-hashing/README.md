# Day 21 - Consistent Hashing + Circuit Breaker

## Table of Contents
- [Consistent Hashing Theory](#consistent-hashing-theory)
- [Hash Ring Concept](#hash-ring-concept)
- [Proof: K/N Keys Remapped](#proof-kn-keys-remapped)
- [Virtual Nodes](#virtual-nodes)
- [Bounded-Load Consistent Hashing](#bounded-load-consistent-hashing)
- [Real-World Implementations](#real-world-implementations)
- [Circuit Breaker Deep Dive](#circuit-breaker-deep-dive)
- [Circuit Breaker State Machine](#circuit-breaker-state-machine)
- [Sliding Window Failure Rate](#sliding-window-failure-rate)
- [Bulkhead Pattern](#bulkhead-pattern)
- [Real Implementations](#real-implementations)
- [Cascading Failure Prevention](#cascading-failure-prevention)
- [Files](#files)
- [Interview Questions](#interview-questions)
- [Daily Assignment](#daily-assignment)

---

## Consistent Hashing Theory

### The Problem with Naive Hashing

```
Simple approach: server = hash(key) % N

With 4 servers (N=4):
  hash("user:1") % 4 = 2 -> Server 2
  hash("user:2") % 4 = 0 -> Server 0
  hash("user:3") % 4 = 1 -> Server 1

Now remove Server 2 (N becomes 3):
  hash("user:1") % 3 = 0 -> Server 0  (MOVED! was Server 2)
  hash("user:2") % 3 = 2 -> Server 2  (MOVED! was Server 0)
  hash("user:3") % 3 = 1 -> Server 1  (unchanged)

Result: ~75% of keys remapped when 1 of 4 servers removed!
For a cache: 75% cache miss = thundering herd to database

General formula for naive hashing:
  Keys remapped ~ (N-1)/N of all keys when adding/removing 1 node
  With N=100 servers: ~99% of keys disrupted!
```

---

## Hash Ring Concept

Consistent hashing places both nodes and keys on a circular hash space (ring).

```
                         0 / 2^32
                           |
                     Node A (pos: 100)
                        *
                    .       .
                 .             .
               .    key "x"      .
              .    (hash: 250)    .
             .     -> Node B       .
  Node D    *                       * Node B (pos: 400)
  (pos: 900).                      .
             .                    .
              .       key "y"    .
               .    (hash: 750) .
                 .   -> Node D .
                    .       .
                        *
                     Node C (pos: 600)
                           |
                    (wraps around)


Assignment rule: key is assigned to the FIRST node encountered
                 when walking CLOCKWISE from the key's hash position.

  key "x" (hash: 250): walk clockwise -> Node B (pos: 400)
  key "y" (hash: 750): walk clockwise -> Node D (pos: 900)
  key "z" (hash: 950): walk clockwise -> Node A (pos: 100, wraps!)
```

### Detailed Ring with Multiple Keys

```
Position:  0    100   200   250   300   400   500   600   700   750   900   950
           |     |     |     |     |     |     |     |     |     |     |     |
Nodes:     .     A     .     .     .     B     .     C     .     .     D     .
Keys:      .     .    k1    k2    k3     .    k4     .    k5    k6     .    k7

Key assignments (clockwise to nearest node):
  k1 (200) -> B (400)   (closest clockwise)
  k2 (250) -> B (400)
  k3 (300) -> B (400)
  k4 (500) -> C (600)
  k5 (700) -> D (900)
  k6 (750) -> D (900)
  k7 (950) -> A (100)   (wraps around ring)

Node A owns: keys in range (900, 100] -> k7
Node B owns: keys in range (100, 400] -> k1, k2, k3
Node C owns: keys in range (400, 600] -> k4
Node D owns: keys in range (600, 900] -> k5, k6
```

---

## Proof: K/N Keys Remapped

**Claim**: When adding or removing a node in a consistent hash ring with N nodes and K total keys, only K/N keys on average need to be remapped.

```
ADDING a new node E between existing nodes:

Before (4 nodes):
  ... --[Node C at 600]-------[Node D at 900]-- ...
  Keys 601-900 all belong to Node D (300 units of range)

After adding Node E at position 750:
  ... --[Node C at 600]--[Node E at 750]--[Node D at 900]-- ...
  Keys 601-750 now belong to Node E (moved from D)
  Keys 751-900 still belong to Node D (unchanged)

Keys moved: only those in the arc between E and E's predecessor (C)
  = 150 / 1000 of total ring space (in this example)
  
General formula:
  Expected keys moved = K / (N + 1)
  Where K = total keys, N = original number of nodes

  With K=1000 keys and N=4 nodes:
    Adding 1 node: ~1000/5 = 200 keys moved (20%)
    vs naive hashing: ~800 keys moved (80%)!

REMOVING a node (similar analysis):
  All keys owned by removed node move to its clockwise successor
  Expected keys moved = K / N
  With K=1000 and N=4: ~250 keys move to one successor
```

---

## Virtual Nodes

### The Problem with Physical Nodes Only

With few nodes, hash positions may be uneven:

```
BAD distribution (3 physical nodes, random placement):

     Node A       Node B                    Node C
       *            *                          *
  pos: 100      pos: 200                   pos: 800

  Node A owns: (800, 100] = 300 units = 30% of ring
  Node B owns: (100, 200] = 100 units = 10% of ring
  Node C owns: (200, 800] = 600 units = 60% of ring!

  Node C gets 6x more keys than Node B! Massive imbalance.
```

### Virtual Nodes Solution

Each physical node creates many virtual positions on the ring:

```
Physical nodes: A, B, C
Virtual nodes per physical: 4 (in practice: 100-200)

Ring with virtual nodes:
  A_0(50) A_1(350) A_2(550) A_3(850)
  B_0(150) B_1(400) B_2(650) B_3(950)
  C_0(250) C_1(450) C_2(750) C_3(100)

Positions sorted:
  50(A) 100(C) 150(B) 250(C) 350(A) 400(B) 450(C) 550(A) 650(B) 750(C) 850(A) 950(B)

Each physical node now owns multiple arcs:
  Node A: 4 arcs scattered around ring
  Node B: 4 arcs scattered around ring
  Node C: 4 arcs scattered around ring

With enough virtual nodes, load approaches uniform distribution.
```

### Variance Analysis

```
Load per node = (keys assigned to node) / (total keys)
Expected load per node = 1/N (uniform)

Standard deviation of load:

WITHOUT virtual nodes (V=1 vnode per physical):
  StdDev = O(1/sqrt(N))
  
  With N=3 nodes: StdDev ~ 0.577 (high variance!)
  Some nodes can get 3x average load.

WITH virtual nodes (V vnodes per physical):
  StdDev = O(1/sqrt(N*V))
  
  With N=3, V=100: StdDev ~ 0.058 (much lower!)
  Nodes get within ~6% of average load.
  
  With N=3, V=200: StdDev ~ 0.041
  Within ~4% of average.

RECOMMENDATION: 100-200 virtual nodes per physical node
  - Diminishing returns beyond 200
  - Memory cost: must store all virtual node positions in ring
  - Lookup cost: O(log(N*V)) binary search on sorted ring
```

### How Many Virtual Nodes?

```
Target: coefficient of variation (CV) < 5%
  CV = StdDev / Mean = sqrt(N*V) / 1 ideally -> need N*V > 400

  N=3 nodes:  V = 400/3 ~ 134 vnodes each
  N=10 nodes: V = 400/10 = 40 vnodes each
  N=50 nodes: V = 400/50 = 8 vnodes each (more nodes = fewer vnodes needed)

Memory per vnode:
  - 4 bytes (ring position, uint32)
  - 4 bytes (physical node ID)
  = 8 bytes per vnode

  N=100 nodes, V=150 vnodes: 100*150*8 = 120KB (trivial)
```

---

## Bounded-Load Consistent Hashing

### Google's Approach (2017 paper)

Standard consistent hashing can still have imbalanced load due to key popularity (not just ring position). Bounded-load adds a capacity constraint:

```
Rule: No node may receive more than (1 + epsilon) * (average_load) keys

Algorithm:
  For key K:
    target = consistent_hash(K)  // normal ring lookup
    if target.load < (1 + epsilon) * average_load:
      assign K to target
    else:
      walk clockwise to next node with capacity
      assign K to that node

Example (epsilon = 0.25, average = 100 keys/node):
  Max per node = 125 keys
  
  Node A: 125 keys (at capacity)
  Key "hot" hashes to Node A
  -> Node A full, walk to Node B (has capacity)
  -> Assign "hot" to Node B

Benefits:
  - Guarantees bounded imbalance (max 1+epsilon overload)
  - Still mostly consistent (keys stay put until their node is overloaded)
  - Handles hot-key scenarios gracefully
```

---

## Real-World Implementations

### Cassandra Token Ring

```
Cassandra: 2^63 to 2^63-1 token range, divided among nodes

Murmur3Partitioner: hash(partition_key) -> 64-bit token

Node assignment:
  CREATE KEYSPACE WITH replication = {'class': 'SimpleStrategy', 'replication_factor': 3};
  
  Token Ring (RF=3):
    Node A: tokens 0-1000      (primary for 0-1000)
    Node B: tokens 1001-2000   (primary for 1001-2000)
    Node C: tokens 2001-3000   (primary for 2001-3000)
    Node D: tokens 3001-4000   (primary for 3001-4000)
    
    With RF=3, each key stored on primary + 2 clockwise successors:
    Key at token 500 -> stored on A (primary), B (replica 1), C (replica 2)

  vnode setting: num_tokens = 256 per node (default)
  Each node owns 256 random token ranges (uniform distribution)
```

### Amazon DynamoDB (Dynamo Paper)

```
DynamoDB key concepts:
  - Consistent hashing for partition assignment
  - Virtual nodes (called "tokens") for balance
  - Preference list: N nodes clockwise from key position (for replication)
  - Coordinator: first node in preference list handles request
  - Sloppy quorum: if preferred nodes down, use "next available" (hinted handoff)
```

### Memcached Ketama

```
Client-side consistent hashing (no central coordinator):

  1. Client library computes ring from server list
  2. Each server name hashed to 160 points on ring (virtual nodes)
     points[i] = MD5(server_name + "-" + i) for i in 0..159
  3. Key lookup: hash(key), binary search ring for next point clockwise
  4. All clients compute SAME ring -> route to same server

  Adding/removing server:
    - Client config updated (new server list)
    - Ring recomputed
    - Only K/(N+1) keys miss (remapped to new server)
    - Self-healing: missed keys re-fetched from DB, cached on new server
```

---

## Circuit Breaker Deep Dive

### The Problem

A failing downstream service can cascade failures upward:

```
Without Circuit Breaker:
  User -> Service A -> Service B -> Service C (FAILING)
  
  Service C: 10s timeout per request
  Service B: waiting for C, threads exhausted
  Service A: waiting for B, threads exhausted
  User: sees timeout after 30+ seconds
  
  Result: ONE failing service takes down ENTIRE chain
          Threads, connections, memory all consumed by waiting
```

---

## Circuit Breaker State Machine

```
+------------------------------------------------------------------+
|                                                                    |
|                    CIRCUIT BREAKER STATE MACHINE                   |
|                                                                    |
|  +----------+         failure_rate         +--------+             |
|  |          |       > threshold            |        |             |
|  |  CLOSED  |----------------------------->|  OPEN  |             |
|  |          |                              |        |             |
|  |(requests |                              |(fast   |             |
|  | pass     |                              | fail,  |             |
|  | through, |      success in              | no     |             |
|  | monitor  |      half-open               | calls  |             |
|  | failures)|<--------+                    | to     |             |
|  +----------+         |                    | down-  |             |
|       ^               |                    | stream)|             |
|       |               |                    +---+----+             |
|       |          +----+------+                 |                  |
|       |          |           |                 |                  |
|       +----------|HALF_OPEN  |<----------------+                  |
|       success    |           |    timeout expires                 |
|       streak     |(allow     |    (e.g., 30 seconds)             |
|       met        | limited   |                                   |
|                  | trial     |                                   |
|                  | requests) |---+                                |
|                  +-----------+   | trial request                  |
|                                  | fails -> back to OPEN          |
|                                  +-----> OPEN                     |
+------------------------------------------------------------------+
```

### Detailed Transitions

```
CLOSED -> OPEN:
  Trigger: failure_rate > threshold within monitoring window
  Example: >50% of last 20 requests failed
  Action: start open_duration timer, reject all subsequent requests

OPEN -> HALF_OPEN:
  Trigger: open_duration timer expires (e.g., after 30 seconds)
  Action: allow limited trial requests through (e.g., 1-3 requests)

HALF_OPEN -> CLOSED:
  Trigger: N consecutive trial requests succeed (e.g., 3 successes)
  Action: reset failure counter, resume normal traffic

HALF_OPEN -> OPEN:
  Trigger: ANY trial request fails
  Action: restart open_duration timer (back to full isolation)
```

---

## Sliding Window Failure Rate

### Implementation

```
Ring buffer approach (sliding window of last N calls):

  Window size: 20 calls
  Failure threshold: 50%
  
  Ring buffer: [S, S, F, S, F, F, S, S, F, F, S, S, S, F, F, S, S, F, S, S]
                                                                          ^
                                                                    newest call

  Count failures: 8 / 20 = 40% -> below threshold -> STAY CLOSED
  
  Next call fails:
  Ring buffer: [S, F, S, F, F, S, S, F, F, S, S, S, F, F, S, S, F, S, S, F]
                                                                          ^
  Count failures: 9 / 20 = 45% -> still below threshold
  
  Two more failures:
  Count: 11 / 20 = 55% -> ABOVE threshold -> TRIP TO OPEN!

Alternative: time-based window
  Track failures in last 60 seconds
  Use sliding window counter (same technique as rate limiting)
  failure_rate = failures_in_window / total_calls_in_window
```

### Minimum Call Threshold

```
Problem: if circuit just started, first failure = 100% failure rate -> trips immediately

Solution: require minimum number of calls before evaluating:

  if total_calls_in_window < minimum_calls (e.g., 10):
    // Not enough data, stay CLOSED regardless of failure rate
    return CLOSED
  else:
    evaluate failure_rate normally
```

---

## Bulkhead Pattern

### Thread Pool Isolation

Named after ship bulkheads (compartments that prevent entire ship from sinking):

```
WITHOUT Bulkhead:
  Service has shared thread pool (100 threads)
  Dependency A: slow (consuming 90 threads waiting)
  Dependency B: healthy but STARVED (only 10 threads available)
  Result: healthy dependency B degraded by failing A

WITH Bulkhead (thread pool isolation):
  +--------------------------------------------------+
  | Service                                           |
  |                                                   |
  | +---------------+  +---------------+              |
  | | Pool: Dep A   |  | Pool: Dep B   |              |
  | | Max: 30 threads|  | Max: 30 threads|            |
  | | Timeout: 5s   |  | Timeout: 2s   |              |
  | +---------------+  +---------------+              |
  |                                                   |
  | +---------------+  +------------------+           |
  | | Pool: Dep C   |  | Remaining (other |           |
  | | Max: 20 threads|  | work): 20 threads|          |
  | | Timeout: 3s   |  +------------------+           |
  | +---------------+                                 |
  +--------------------------------------------------+
  
  If Dep A exhausts its 30 threads:
    - Dep A callers get rejected (fail fast)
    - Dep B, Dep C, other work UNAFFECTED (separate pools)
```

### Semaphore-Based Bulkhead

```
Lighter weight than thread pools (no thread creation overhead):

  Semaphore for Dep A: max_concurrent = 30
  
  call_dep_a():
    if semaphore.tryAcquire():
      try:
        result = dep_a.call()
      finally:
        semaphore.release()
    else:
      throw BulkheadFullException("Dep A at capacity")

Difference from thread pool:
  - Thread pool: dedicated threads, calls execute in pool
  - Semaphore: caller's thread executes, just limits concurrency
  - Semaphore is lighter but cannot timeout waiting calls
```

---

## Real Implementations

### Netflix Hystrix (Legacy, Influential)

```
@HystrixCommand(
    commandGroupKey = "UserService",
    commandKey = "GetUser",
    threadPoolKey = "UserPool",
    threadPoolProperties = {
        @HystrixProperty(name = "coreSize", value = "30"),
        @HystrixProperty(name = "maxQueueSize", value = "100")
    },
    commandProperties = {
        @HystrixProperty(name = "circuitBreaker.requestVolumeThreshold", value = "20"),
        @HystrixProperty(name = "circuitBreaker.errorThresholdPercentage", value = "50"),
        @HystrixProperty(name = "circuitBreaker.sleepWindowInMilliseconds", value = "5000"),
        @HystrixProperty(name = "execution.isolation.thread.timeoutInMilliseconds", value = "3000")
    },
    fallbackMethod = "getUserFallback"
)
public User getUser(String id) {
    return userService.getUser(id);
}

Key concepts:
  - Thread pool per dependency (bulkhead)
  - Circuit breaker per command
  - Fallback method when circuit open or call fails
  - Request collapsing (batch multiple calls into one)
  - Metrics stream (real-time dashboard)
```

### Resilience4j (Modern Java)

```
CircuitBreaker circuitBreaker = CircuitBreaker.ofDefaults("backendService");

// Configuration
CircuitBreakerConfig config = CircuitBreakerConfig.custom()
    .failureRateThreshold(50)              // 50% failure rate trips
    .slowCallRateThreshold(80)             // 80% slow calls also trips
    .slowCallDurationThreshold(Duration.ofSeconds(2))
    .waitDurationInOpenState(Duration.ofSeconds(30))
    .permittedNumberOfCallsInHalfOpenState(3)
    .minimumNumberOfCalls(10)
    .slidingWindowType(SlidingWindowType.COUNT_BASED)
    .slidingWindowSize(20)
    .build();

Advantages over Hystrix:
  - Lightweight (no thread pool overhead, uses decorators)
  - Composable (stack CircuitBreaker + RateLimiter + Retry + Bulkhead)
  - Functional style (Java lambdas)
  - Active maintenance (Hystrix is in maintenance mode)
```

### Polly (.NET)

```csharp
var circuitBreaker = Policy
    .Handle<HttpRequestException>()
    .Or<TimeoutException>()
    .CircuitBreaker(
        exceptionsAllowedBeforeBreaking: 5,
        durationOfBreak: TimeSpan.FromSeconds(30),
        onBreak: (ex, breakDuration) => Log("Circuit opened"),
        onReset: () => Log("Circuit closed"),
        onHalfOpen: () => Log("Circuit half-open, testing...")
    );

// Compose with retry and timeout
var policy = Policy.Wrap(
    circuitBreaker,
    Policy.Handle<Exception>().Retry(3),
    Policy.Timeout(TimeSpan.FromSeconds(5))
);
```

---

## Cascading Failure Prevention

### Architecture for Resilience

```
+---------------------------------------------------------------+
|                    RESILIENT SERVICE ARCHITECTURE               |
+---------------------------------------------------------------+
|                                                                 |
|   Client Request                                                |
|        |                                                        |
|        v                                                        |
|   [Rate Limiter] -- reject if over limit                       |
|        |                                                        |
|        v                                                        |
|   [Circuit Breaker] -- fast-fail if circuit open               |
|        |                                                        |
|        v                                                        |
|   [Bulkhead] -- reject if pool exhausted                       |
|        |                                                        |
|        v                                                        |
|   [Timeout] -- cancel if too slow                              |
|        |                                                        |
|        v                                                        |
|   [Retry w/ backoff] -- retry transient failures               |
|        |                                                        |
|        v                                                        |
|   [Downstream Call]                                             |
|        |                                                        |
|     success / failure feeds back to circuit breaker metrics     |
|                                                                 |
+---------------------------------------------------------------+

Order matters! Rate limit first (cheapest check), then circuit
breaker (no need to acquire bulkhead slot if circuit is open),
then bulkhead, then timeout around the actual call with retry.
```

### Load Shedding

```
When system is overloaded, intentionally drop requests to protect core functionality:

Priority-based shedding:
  P0 (critical): user authentication, payments -> NEVER shed
  P1 (important): feed generation, search -> shed at 90% load
  P2 (best-effort): recommendations, analytics -> shed at 70% load
  P3 (background): batch jobs, non-urgent notifications -> shed at 50% load

Implementation:
  on_request(request):
    load = get_system_load()
    
    if load > 0.9 AND request.priority >= P2:
      return 503 Service Unavailable
    elif load > 0.7 AND request.priority >= P3:
      return 503 Service Unavailable
    else:
      process(request)
```

### Graceful Degradation

```
Instead of failing completely, provide reduced functionality:

Example: E-commerce product page
  Normal mode:
    - Product details (DB)
    - Personalized recommendations (ML service)
    - Real-time inventory (Inventory service)
    - User reviews (Reviews service)
    
  Degraded mode (recommendations service down):
    - Product details (DB) -> still works
    - Static popular products -> fallback
    - Real-time inventory -> still works
    - User reviews -> still works
    
  Heavily degraded (multiple services down):
    - Product details (from cache, possibly stale)
    - No recommendations
    - "Check availability in store" (static message)
    - No reviews
    
  Still better than showing a 500 error page!
```

---

## Files
- [consistent_hash.cpp](consistent_hash.cpp) - ring with virtual nodes
- [circuit_breaker.cpp](circuit_breaker.cpp)

## Interview Questions
1. Why is `hash % N` bad in dynamic clusters? Quantify the disruption.
2. How do virtual nodes help in consistent hashing? How many are needed?
3. Real-world: how does Cassandra route writes? (consistent hashing + replication factor)
4. What is data skew? How does virtual nodes reduce it? What is the variance formula?
5. Circuit breaker - state transitions on success and failure? Draw the state machine.
6. Difference between Circuit Breaker and Retry? When to combine them?
7. When to use Bulkhead pattern with Circuit Breaker? Thread pool vs semaphore?
8. How to implement circuit breaker in distributed system without shared state?
9. Prove that only K/N keys are remapped when adding a node to consistent hash ring.
10. What is bounded-load consistent hashing? What problem does it solve?
11. Explain the cascading failure problem. How does the combination of circuit breaker + bulkhead + timeout prevent it?
12. What is load shedding? How do you decide which requests to shed?
13. Design a graceful degradation strategy for a social media feed. What are the fallbacks?
14. How does Memcached Ketama work? Why is it client-side consistent hashing?
15. If you have 5 physical nodes and 150 virtual nodes each, what is the expected load standard deviation?

## Daily Assignment
1. Implement consistent hashing with virtual nodes; add/remove a node and count key migrations. Verify K/N property.
2. Implement circuit breaker with three states (CLOSED, OPEN, HALF_OPEN). Wrap a flaky function and observe state transitions.
3. Combine: route requests to N backends via consistent hash ring; if a backend trips its circuit breaker, reroute to next node on ring.
4. Implement bounded-load consistent hashing: set max load per node, overflow to next node clockwise.
5. Bonus: add a bulkhead (semaphore-based) per backend. When bulkhead is full, circuit breaker should count it as a failure.
6. Advanced: measure load distribution standard deviation with V=1, 10, 50, 100, 200 virtual nodes. Plot the convergence.
