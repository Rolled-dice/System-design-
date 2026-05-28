# Day 16 - Caching + CDN

## Table of Contents
- [Cache Theory](#cache-theory)
- [Caching Strategies](#caching-strategies)
- [Eviction Policies](#eviction-policies)
- [Cache Stampede and Thundering Herd](#cache-stampede-and-thundering-herd)
- [Redis Internals](#redis-internals)
- [Memcached vs Redis](#memcached-vs-redis)
- [CDN Deep Dive](#cdn-deep-dive)
- [Files](#files)
- [Interview Questions](#interview-questions)
- [Daily Assignment](#daily-assignment)

---

## Cache Theory

### Locality of Reference

Caching works because of two fundamental properties of real workloads:

**Temporal Locality**: If data was accessed recently, it will likely be accessed again soon.
- Example: A trending tweet is read millions of times in minutes
- Example: User session data accessed on every request

**Spatial Locality**: If data at address X was accessed, nearby data will likely be accessed soon.
- Example: Reading a database row often means reading adjacent rows
- Example: Fetching CSS after HTML (same page resources)

**The 80/20 Rule (Pareto)**: Typically 20% of data serves 80% of requests. A cache holding just 20% of your dataset can serve 80% of reads.

**Cache Hit Ratio Formula**:
```
Hit Ratio = cache_hits / (cache_hits + cache_misses)

Effective Latency = hit_ratio * cache_latency + (1 - hit_ratio) * db_latency

Example: 95% hit ratio, cache=1ms, DB=50ms
  Effective = 0.95 * 1 + 0.05 * 50 = 0.95 + 2.5 = 3.45ms
  (vs 50ms without cache = 14.5x improvement)
```

### Cache Hierarchy

```
+---------------------------------------------------+
| CPU Registers        | ~0.3ns  | 1KB              |
+---------------------------------------------------+
| L1 Cache (per core)  | ~1ns    | 32-64KB          |
+---------------------------------------------------+
| L2 Cache (per core)  | ~4ns    | 256KB-1MB        |
+---------------------------------------------------+
| L3 Cache (shared)    | ~12ns   | 8-64MB           |
+---------------------------------------------------+
| RAM                  | ~100ns  | 16-512GB         |
+---------------------------------------------------+
| Application Cache    | ~1ms    | Bounded by RAM   |
| (in-process HashMap) |         |                  |
+---------------------------------------------------+
| Distributed Cache    | ~1-5ms  | 100s of GB       |
| (Redis, Memcached)   |         | (cluster)        |
+---------------------------------------------------+
| Database             | ~5-50ms | TB-PB            |
+---------------------------------------------------+
| Disk/SSD             | ~0.1ms  | TB per node      |
+---------------------------------------------------+
```

**Key insight**: Each level trades capacity for speed. System design interviews focus on application-level and distributed caching.

---

## Caching Strategies

### 1. Cache-Aside (Lazy Loading)

The application manages cache explicitly. Most common pattern.

```
READ PATH:
  Client        Cache         Database
    |              |              |
    |--get(key)--->|              |
    |              |              |
    |<--MISS-------|              |
    |              |              |
    |----------get(key)---------->|
    |              |              |
    |<---------data--------------|
    |              |              |
    |--set(key,data,TTL)-->|     |
    |              |              |
    |<--OK---------|              |

WRITE PATH:
  Client        Cache         Database
    |              |              |
    |----------write(key,data)--->|
    |              |              |
    |--delete(key)->|             |
    |              |              |
```

**Pros**: Cache only what is actually read; simple; cache failure = graceful degradation (reads hit DB)
**Cons**: Cache miss = 3 operations (check, fetch, fill); first request always slow (cold cache)
**Why delete on write, not update?**: Avoids race condition where stale write overwrites fresh data. Delete is idempotent.

### 2. Read-Through

Cache itself fetches from DB on miss. Application only talks to cache.

```
  Client          Cache              Database
    |               |                   |
    |--get(key)---->|                   |
    |               |---(miss: fetch)-->|
    |               |<---data-----------|
    |               | (stores in cache) |
    |<---data-------|                   |
```

**Pros**: Application code simpler (no miss-handling logic)
**Cons**: Cache must know how to query the data source; initial requests still slow

### 3. Write-Through

Every write goes to cache AND database synchronously.

```
  Client          Cache              Database
    |               |                   |
    |--write(k,v)-->|                   |
    |               |--write(k,v)------>|
    |               |<--ACK-------------|
    |<---ACK--------|                   |
```

**Pros**: Cache always consistent with DB; read-after-write always hits cache
**Cons**: Write latency doubled (cache + DB); writes for data never read waste cache space
**Best paired with**: Read-through (full cache-layer abstraction)

### 4. Write-Behind (Write-Back)

Write to cache immediately, asynchronously flush to database in batches.

```
  Client          Cache              Database
    |               |                   |
    |--write(k,v)-->|                   |
    |<---ACK--------|                   |
    |               |                   |
    |               |...(async batch)...|
    |               |--write(batch)---->|
    |               |<--ACK-------------|
```

**Pros**: Extremely fast writes; batching reduces DB write load; coalesce multiple writes to same key
**Cons**: Data loss risk if cache crashes before flush; complexity; eventual consistency
**Use case**: Write-heavy workloads (analytics events, counters, real-time metrics)

### 5. Write-Around

Write directly to DB, bypassing cache entirely.

```
  Client          Cache              Database
    |               |                   |
    |--write(k,v)---(skip)------------>|
    |<---ACK----------------------------|
    |               |                   |
    (subsequent reads fill cache via cache-aside)
```

**Pros**: Cache not polluted with write-only data; good for write-heavy, read-rarely data
**Cons**: Read-after-write will miss cache; higher read latency until cache fills

---

## Eviction Policies

### LRU (Least Recently Used) - Implementation Deep Dive

LRU evicts the item that has not been accessed for the longest time. The key insight: we need O(1) for both get and put.

**Data Structure: HashMap + Doubly-Linked List**

```
HashMap: key -> Node*
  +-----+     +-----+     +-----+
  | "A" | --> | ptr |     | "B" | --> | ptr |
  +-----+     +-----+     +-----+     +-----+

Doubly-Linked List (most recent at HEAD):
  HEAD                                    TAIL
   |                                       |
   v                                       v
[sentinel] <-> [D] <-> [A] <-> [B] <-> [C] <-> [sentinel]
  (dummy)       ^                        ^       (dummy)
                |                        |
           most recent              least recent
                                    (evict this)
```

**Operations (all O(1))**:
- **get(key)**: HashMap lookup O(1) -> move node to head O(1) -> return value
- **put(key, value)**: If exists, update + move to head. If full, remove tail node + remove from HashMap. Insert new node at head + add to HashMap.
- **evict()**: Remove node at tail (least recently used), delete from HashMap

**Why doubly-linked list?** Need O(1) removal of arbitrary nodes (when accessed, move to head). Single-linked list requires O(n) to find previous node.

### LFU (Least Frequently Used)

Evicts the item with the lowest access count. On ties, evicts the least recently used among tied items.

```
Frequency Map:
  freq=1: [C] -> [E]          (least frequent, evict C first)
  freq=2: [B] -> [D]
  freq=5: [A]                  (most frequent)

Data Structure:
  - HashMap: key -> (value, frequency, node_pointer)
  - FrequencyMap: freq -> DoublyLinkedList of nodes
  - min_freq: tracks minimum frequency for O(1) eviction
```

**O(1) LFU implementation**:
1. get(key): increment freq, move node from freq_list[f] to freq_list[f+1]
2. put(key,val): if full, evict from freq_list[min_freq].tail, insert at freq_list[1].head, min_freq=1

### ARC (Adaptive Replacement Cache)

IBM's ARC combines recency and frequency adaptively. Outperforms both LRU and LFU.

```
+------------------------------------------------------------------+
|                        ARC Structure                               |
|                                                                    |
|  Ghost B1     |        T1        |       T2        |  Ghost B2    |
| (recent evicts)| (recently used) | (frequently used)| (freq evicts)|
|               |                  |                  |               |
|  [tracks keys |  [actual cache]  |  [actual cache]  | [tracks keys |
|   evicted     |   recency-based  |   frequency-based|  evicted     |
|   from T1]    |                  |                  |  from T2]    |
+------------------------------------------------------------------+

Adaptation: 
  - Hit in B1 ghost: T1 was too small -> increase T1 target size
  - Hit in B2 ghost: T2 was too small -> increase T2 target size
```

**Key insight**: ARC automatically adapts to workload. Scan-resistant (unlike LRU) and recency-aware (unlike LFU).

### CLOCK Algorithm (Second-Chance)

Approximates LRU with lower overhead. Used in OS page replacement.

```
Clock Hand
    |
    v
+---+---+---+---+---+---+---+---+
| 1 | 0 | 1 | 1 | 0 | 1 | 0 | 1 |  <- reference bits
| A | B | C | D | E | F | G | H |  <- pages
+---+---+---+---+---+---+---+---+
        ^
        |
  On eviction needed:
  - If ref_bit=1: clear to 0, advance hand (second chance)
  - If ref_bit=0: evict this page
  - On access: set ref_bit=1
```

**Advantage over LRU**: No need to reorder on every access. Just set a bit. Much cheaper for high-throughput scenarios.

---

## Cache Stampede and Thundering Herd

### The Problem

When a popular cached item expires, many concurrent requests all see a cache miss simultaneously, all hit the database:

```
Time -->
  Cache: [HOT_KEY expires]
  
  Request 1: miss -> query DB
  Request 2: miss -> query DB     All hit DB simultaneously!
  Request 3: miss -> query DB     DB overloaded!
  Request 4: miss -> query DB
  ...
  Request 1000: miss -> query DB
```

### Solution 1: Mutex/Lock (Cache Lock)

```
get(key):
  value = cache.get(key)
  if value is None:
    if acquire_lock(key, timeout=5s):
      try:
        value = db.query(key)
        cache.set(key, value, ttl=60)
      finally:
        release_lock(key)
    else:
      sleep(50ms)
      return get(key)  // retry, lock holder should have filled cache
  return value
```

**Tradeoff**: Serializes refills (only 1 DB query), but adds latency for waiters.

### Solution 2: Probabilistic Early Expiration (XFetch)

Refresh cache BEFORE it expires. Each access has a probability of triggering early refresh:

```
should_recompute = (current_time - (expiry - ttl * beta * log(random()))) > 0

Parameters:
  beta = 1.0 (higher = earlier refresh)
  As expiry approaches, probability of refresh increases
  Only ONE request wins the race -> refreshes cache
```

**Advantage**: No locking, no thundering herd, cache stays warm.

### Solution 3: Stale-While-Revalidate

Serve stale data immediately, trigger async background refresh:
```
get(key):
  value, expiry = cache.get_with_metadata(key)
  if value is not None:
    if expiry < current_time:  // expired but still in cache
      async_refresh(key)       // background refresh
    return value               // serve stale immediately
  else:
    return fetch_and_cache(key)  // true cache miss
```

---

## Redis Internals

### Core Data Structures

**1. Simple Dynamic String (SDS)**:
```
+--------+------+------+--------+
| len(5) | free(3) | flags | "hello\0"  |
+--------+------+------+--------+

vs C string: just "hello\0"

Advantages:
- O(1) strlen (stored in header)
- Binary-safe (can contain \0)
- Buffer overflow protection (bounds-checked)
- Reduced reallocation (pre-allocated free space)
```

**2. Ziplist** (compact list for small collections):
```
+--------+------+-------+-------+-----+-------+--------+
| zlbytes| zltail| zllen | entry1| ... | entryN| zlend  |
+--------+------+-------+-------+-----+-------+--------+

Each entry: [prev_entry_len | encoding | data]

Used when: list < 128 elements AND all elements < 64 bytes
Advantage: Cache-friendly, no pointer overhead
Disadvantage: O(N) access, cascading updates
```

**3. Skiplist** (sorted sets):
```
Level 4: HEAD -----------------------------------------> NIL
Level 3: HEAD ----------------> [30] -----------------> NIL
Level 2: HEAD -------> [10] -> [30] -------> [50] ----> NIL
Level 1: HEAD -> [5] -> [10] -> [30] -> [40] -> [50] -> NIL
```
- O(log N) search, insert, delete
- Used for Redis Sorted Sets (ZSET)
- Simpler to implement than balanced BSTs, good concurrent performance

**4. Dict (Hash Table)**:
- Two hash tables (ht[0] and ht[1]) for incremental rehashing
- Rehash: when load factor > 1, allocate ht[1] with 2x size, gradually move buckets
- Each operation moves a few buckets (amortized O(1) rehash, no spike)

### Persistence

**RDB (Point-in-Time Snapshots)**:
```
fork() -> child writes entire dataset to disk as binary file
Parent continues serving (copy-on-write pages)

Pros: Compact, fast recovery, good for backups
Cons: Data loss between snapshots (e.g., last 5 minutes)
Config: save 900 1    (save if 1 key changed in 900s)
        save 300 10   (save if 10 keys changed in 300s)
```

**AOF (Append-Only File)**:
```
Every write command appended to log:
  *3\r\n$3\r\nSET\r\n$5\r\nhello\r\n$5\r\nworld\r\n

Fsync policies:
  - always: fsync after every write (safest, slowest)
  - everysec: fsync once per second (good tradeoff)
  - no: let OS decide (fastest, up to 30s data loss)

AOF rewrite: background process compacts log (removes overwritten keys)
```

**Hybrid (Redis 4.0+)**: RDB snapshot + AOF tail. Fast recovery (load RDB) + minimal data loss (replay recent AOF).

### Cluster Mode (Hash Slots)

```
Total: 16384 hash slots distributed across nodes

Node A: slots 0-5460
Node B: slots 5461-10922
Node C: slots 10923-16383

slot = CRC16(key) % 16384

Client -> Node A: GET user:42
  slot = CRC16("user:42") % 16384 = 7231
  Node A: "MOVED 7231 10.0.0.2:6379" (redirect to Node B)
  Client -> Node B: GET user:42 -> "value"
```

**Resharding**: Move slot ranges between nodes (live, progressive migration).

### Pub/Sub
- SUBSCRIBE channel: client registers interest
- PUBLISH channel msg: all subscribers on all nodes receive
- Limitation: fire-and-forget (no persistence, no replay if subscriber offline)
- Alternative: Redis Streams (persistent, consumer groups, like mini-Kafka)

---

## Memcached vs Redis

| Feature | Memcached | Redis |
|---------|-----------|-------|
| Data structures | String only (key-value) | Strings, Lists, Sets, Sorted Sets, Hashes, Streams, HyperLogLog |
| Persistence | None (pure cache) | RDB + AOF |
| Clustering | Client-side sharding (ketama) | Native cluster (hash slots) |
| Memory efficiency | Slab allocator (less fragmentation) | jemalloc |
| Threading | Multi-threaded | Single-threaded (+ I/O threads in 6.0+) |
| Max value size | 1MB default | 512MB |
| Pub/Sub | No | Yes |
| Lua scripting | No | Yes (atomic operations) |
| Replication | No | Leader-follower |
| Transactions | No | MULTI/EXEC (optimistic locking with WATCH) |
| Use case | Simple caching, large-scale | Caching + data structures + messaging |

**When to choose Memcached**: Pure key-value caching with multi-threaded performance needs. Simpler operational model.
**When to choose Redis**: Need data structures, persistence, pub/sub, atomic operations, or scripting.

---

## CDN Deep Dive

### How CDN Works

```
User (Mumbai)                                             Origin (US-East)
     |                                                         |
     |  1. DNS: cdn.example.com                                |
     v                                                         |
+----------+    2. Returns nearest edge IP                     |
| CDN DNS  |    (geo-based or anycast)                         |
+----------+                                                   |
     |                                                         |
     v                                                         |
+-----------+                                                  |
| Edge POP  |  3a. Cache HIT -> serve immediately              |
| (Mumbai)  |  3b. Cache MISS -> fetch from origin             |
+-----------+  <-----------------------------------------------|
     |         4. Cache response for future requests           |
     v                                                         |
  [Response to user in ~20ms vs ~200ms from origin]
```

### DNS-Based Routing vs Anycast

**DNS-Based**: CDN's authoritative DNS returns different IPs based on client's resolver location.
- Pros: Fine-grained control, can consider server health/load
- Cons: DNS caching can route to wrong POP, depends on resolver location (not user location)

**Anycast**: Same IP address announced from all edge POPs via BGP. Network routes to closest.
- Pros: Instant failover (BGP reroutes), no DNS TTL issues
- Cons: Less control over routing decisions, BGP convergence time

### Pull vs Push CDN

| | Pull CDN | Push CDN |
|--|----------|----------|
| Mechanism | Edge fetches from origin on first miss | Content pre-uploaded to all edges |
| Best for | Dynamic traffic, long-tail content | Known popular content (videos, software updates) |
| Freshness | Controlled by TTL/Cache-Control | Explicitly managed |
| Origin load | Proportional to unique misses | Only during push |
| Examples | Cloudflare, Fastly | Video VOD platforms |

### Cache Invalidation Strategies

```
1. TTL-Based:
   Cache-Control: max-age=3600     // edge caches for 1 hour
   CDN respects TTL then refetches

2. Purge/Invalidation API:
   POST /purge {"url": "/images/logo.png"}
   CDN removes from all edge caches immediately

3. Versioned URLs:
   /static/app.v2.3.1.js           // new version = new URL
   Old version naturally expires, new version caches fresh
   (Best practice for static assets)

4. Stale-While-Revalidate:
   Cache-Control: max-age=60, stale-while-revalidate=300
   Serve stale for 5min while background-refreshing
```

### Edge Computing

Modern CDNs execute code at the edge (Cloudflare Workers, Lambda@Edge):
- A/B testing logic at edge (no origin round-trip)
- Request routing/transformation
- Authentication/authorization
- Personalized content assembly
- Reduces origin load AND latency

---

## Files
- [lru_cache.cpp](lru_cache.cpp)
- [lfu_cache.cpp](lfu_cache.cpp)
- [cache_aside.cpp](cache_aside.cpp) - simulates DB + cache
- [ttl_cache.cpp](ttl_cache.cpp)

## Interview Questions
1. Cache-Aside vs Write-Through vs Write-Behind - trade-offs and when to use each?
2. Implement LRU in O(1) - explain hashmap + doubly linked list approach in detail.
3. How do you avoid thundering herd / cache stampede? Compare locking vs probabilistic approaches.
4. Cache penetration vs cache avalanche vs hot key - definitions and mitigations for each.
5. Why is invalidation hard? "There are 2 hard things in CS..." Explain consistency challenges.
6. CDN - how does TTL work with `Cache-Control` headers? What about `stale-while-revalidate`?
7. When should you NOT cache? (frequently changing data, low-hit-ratio data, security-sensitive)
8. Distributed cache consistency - sharding strategies? What happens when a node dies?
9. Redis vs Memcached - when to choose which? Explain threading model differences.
10. How does Redis achieve persistence without blocking? Explain fork() and copy-on-write.
11. What is the ARC eviction policy and why does it outperform LRU?
12. Explain Redis cluster hash slot migration. How does the client handle MOVED responses?

## Daily Assignment
1. Implement LRU cache with O(1) get/put using HashMap + DoublyLinkedList.
2. Implement LFU cache with O(1) operations using frequency buckets.
3. Build a `UserService` with cache-aside on top of a fake `UserDb`. Add TTL expiry + background sweep.
4. Implement probabilistic early expiration (XFetch algorithm) to prevent cache stampede.
5. Bonus: Build a simple CDN simulator - multiple edge nodes with consistent hashing for cache distribution.
