# Day 18 - Sharding, Replication, CAP Theorem

## Table of Contents
- [CAP Theorem](#cap-theorem)
- [PACELC Extended Model](#pacelc-extended-model)
- [Sharding Deep Dive](#sharding-deep-dive)
- [Consistent Hashing for Sharding](#consistent-hashing-for-sharding)
- [Hot Key Detection and Mitigation](#hot-key-detection-and-mitigation)
- [Cross-Shard Transactions](#cross-shard-transactions)
- [Replication Deep Dive](#replication-deep-dive)
- [Leader Election with Raft](#leader-election-with-raft)
- [Conflict Resolution and CRDTs](#conflict-resolution-and-crdts)
- [Cassandra Tunable Consistency](#cassandra-tunable-consistency)
- [Anti-Entropy and Repair](#anti-entropy-and-repair)
- [Files](#files)
- [Interview Questions](#interview-questions)
- [Daily Assignment](#daily-assignment)

---

## CAP Theorem

### Formal Definition

In a distributed data store, it is impossible to simultaneously provide all three of the following guarantees:

- **Consistency (C)**: Every read receives the most recent write or an error (linearizability)
- **Availability (A)**: Every request receives a non-error response (but not necessarily the most recent write)
- **Partition Tolerance (P)**: The system continues to operate despite network partitions between nodes

### Proof Sketch (Why You Cannot Have All 3)

```
Consider two nodes N1 and N2 with a network partition between them:

     N1                    N2
  [data: X=1]    |X|    [data: X=1]
                 |X|  <- network partition
                 |X|
Client writes X=2 to N1:

     N1                    N2
  [data: X=2]    |X|    [data: X=1]  <- cannot replicate!
                 |X|
                 |X|
Now client reads from N2:

Option A (Choose Consistency):
  N2 says "ERROR - cannot confirm latest value"
  -> Sacrificed AVAILABILITY

Option B (Choose Availability):
  N2 returns X=1 (stale!)
  -> Sacrificed CONSISTENCY

There is no Option C. During a partition, you MUST choose.
```

**Key insight**: In the absence of partitions, you can have both C and A. CAP is about what you sacrifice WHEN a partition occurs. Since network partitions are inevitable in distributed systems, the real choice is CP vs AP.

### Real System Classification

| System | Choice | Behavior During Partition |
|--------|--------|--------------------------|
| MongoDB (default) | CP | Primary unavailable = no writes |
| HBase, ZooKeeper, etcd | CP | Minority partition unavailable |
| Cassandra, DynamoDB | AP (tunable) | Accept writes on both sides, resolve later |
| MySQL/PostgreSQL (single) | CA* | No partitions possible (single node) |
| CockroachDB | CP | Raft-based, minority unavailable |
| Riak | AP | Sibling versions, client resolves |

*CA only exists for single-node systems (no P needed). Distributed systems always face partitions.

---

## PACELC Extended Model

CAP only describes behavior during partitions. PACELC extends this:

```
if (Partition) {
    choose Availability or Consistency   // PAC part (same as CAP)
} else {  // normal operation
    choose Latency or Consistency        // ELC part (new insight)
}
```

| System | P: A or C | E: L or C | Explanation |
|--------|-----------|-----------|-------------|
| Cassandra | PA | EL | Favors availability and low latency always |
| DynamoDB | PA | EL | Same philosophy as Cassandra |
| MongoDB | PC | EC | Sacrifices availability and latency for consistency |
| PNUTS (Yahoo) | PC | EL | Consistent during partition, but low latency normally |
| VoltDB | PC | EC | Always consistent, synchronous replication |

**Why PACELC matters**: Two systems can both be "CP" under CAP but behave very differently in normal operation. A system that sacrifices latency for consistency (EC) versus one that favors latency (EL) has fundamentally different performance characteristics.

---

## Sharding Deep Dive

### Sharding Strategies

| Strategy | Mechanism | Pros | Cons |
|----------|-----------|------|------|
| **Range** | key ranges: A-M->shard1, N-Z->shard2 | Range scans efficient | Hot spots (sequential keys) |
| **Hash** | hash(key) % N_shards | Even distribution | No range scans, resharding pain |
| **Consistent Hash** | Ring-based (see Day 21) | Minimal reshuffling | More complex routing |
| **Directory/Lookup** | Central mapping table | Flexible | Lookup service = SPOF |
| **Geo** | Region-based | Data locality | Cross-region queries expensive |

### Resharding Strategies

When you need to add/remove shards:

**1. Virtual Shards (Pre-splitting)**:
```
Instead of N physical shards, create N*100 virtual shards:

Physical nodes: [Node A] [Node B] [Node C]
Virtual shards:  V1-V33   V34-V66  V67-V100

Adding Node D:
  Move V1-V25 from A, V34-V50 from B, V67-V83 from C to D
  Only virtual shard assignment changes, no data re-hashing!

Physical nodes: [Node A] [Node B] [Node C] [Node D]
Virtual shards:  V26-V33  V51-V66  V84-V100  V1-V25,V34-V50,V67-V83
```

**2. Shadow Traffic Migration**:
```
Phase 1: Double-write to old AND new shard layout
Phase 2: Backfill historical data to new layout
Phase 3: Verify data consistency (checksums)
Phase 4: Switch reads to new layout
Phase 5: Stop writes to old layout
Phase 6: Decommission old layout
```

---

## Consistent Hashing for Sharding

```
Hash Ring (0 to 2^32):

              Node A (pos 1000)
                 *
              .     .
           .           .
         .               .
  Node D *                 * Node B (pos 4000)
  (pos 9000)              .
         .               .
           .           .
              .     .
                 *
              Node C (pos 7000)

Key "user:42" -> hash = 2500 -> walks clockwise -> lands on Node B
Key "user:99" -> hash = 8000 -> walks clockwise -> lands on Node D

Adding Node E at position 3000:
  Only keys between 1000-3000 move from Node B to Node E
  All other keys unchanged! (K/N keys remapped on average)
```

---

## Hot Key Detection and Mitigation

### The Problem

Some keys receive disproportionate traffic (celebrity tweet, viral product):

```
Shard 1: 100 req/s   [normal]
Shard 2: 100 req/s   [normal]
Shard 3: 50,000 req/s [HOT KEY: "trending_tweet_12345"]
Shard 4: 100 req/s   [normal]
```

### Detection Methods

1. **Counter-based**: Track per-key access counts at shard level, report keys exceeding threshold
2. **Sampling**: Sample 1% of requests, extrapolate hot keys
3. **Client-side tracking**: SDK tracks which keys it reads most frequently

### Mitigation Strategies

**1. Key Salting (Read Sharding)**:
```
Instead of one key "hot_tweet_123", create N copies:
  "hot_tweet_123:shard_0"
  "hot_tweet_123:shard_1"
  "hot_tweet_123:shard_2"
  ...
  "hot_tweet_123:shard_9"

Read: randomly pick one of the 10 salted keys
  -> load spread across 10 shards instead of 1

Write: write to ALL salted copies (or use eventual consistency)

Trade-off: 10x write amplification for hot keys, but reads distributed
```

**2. Dedicated Hot-Key Cache**:
```
Request flow:
  1. Check hot-key cache (in-memory, L1) -> HIT? return immediately
  2. If miss, route to shard normally
  
Hot-key detection triggers:
  - Move key to dedicated cache layer
  - Cache has short TTL (1-5s) to stay fresh
  - Cache absorbs 99%+ of reads for that key

Architecture:
  Client -> [Hot-Key Cache (Redis)] -> [Normal Sharding Layer] -> DB
```

**3. Read Replicas for Hot Shards**:
```
Normal shard: single leader
Hot shard: leader + N read replicas

Reads distributed across replicas (round-robin or least-connections)
Writes still go to leader only
```

---

## Cross-Shard Transactions

### Two-Phase Commit (2PC)

```
Coordinator                Shard A              Shard B
     |                        |                    |
     |  PREPARE (tx_data)     |                    |
     |----------------------->|                    |
     |  PREPARE (tx_data)     |                    |
     |----------------------------------------------->|
     |                        |                    |
     |  VOTE: YES             |                    |
     |<-----------------------|                    |
     |  VOTE: YES                                  |
     |<-----------------------------------------------|
     |                        |                    |
     |  (all voted YES)       |                    |
     |                        |                    |
     |  COMMIT                |                    |
     |----------------------->|                    |
     |  COMMIT                                     |
     |----------------------------------------------->|
     |                        |                    |
     |  ACK                   |                    |
     |<-----------------------|                    |
     |  ACK                                        |
     |<-----------------------------------------------|
```

**2PC Problems**:
- **Blocking**: If coordinator crashes after PREPARE, participants are stuck (holding locks)
- **Latency**: 2 round-trips minimum
- **Single point of failure**: Coordinator crash = blocked transactions

### Saga Pattern (Alternative to 2PC)

Saga breaks distributed transaction into a sequence of local transactions with compensating actions:

```
ORCHESTRATION SAGA:

Orchestrator             Service A        Service B        Service C
     |                      |                |                |
     |-- Do Step 1 -------->|                |                |
     |<- Step 1 Done -------|                |                |
     |                      |                |                |
     |-- Do Step 2 -------->|--------------->|                |
     |<- Step 2 Done -------|----------------|                |
     |                      |                |                |
     |-- Do Step 3 -------->|--------------->|--------------->|
     |<- Step 3 FAILED -----|----------------|----------------|
     |                      |                |                |
     |-- Compensate Step 2 ->|-------------->|                |
     |-- Compensate Step 1 ->|               |                |
     |                      |                |                |

CHOREOGRAPHY SAGA:

Service A          Event Bus          Service B          Service C
   |                  |                   |                  |
   |-- OrderCreated ->|                   |                  |
   |                  |-- OrderCreated -->|                  |
   |                  |                   |-- PaymentDone -->|
   |                  |                   |                  |-- ShipmentStarted
   |                  |                   |                  |
   (If failure: publish compensating event, each service undoes its step)
```

**Saga vs 2PC**:
| Property | 2PC | Saga |
|----------|-----|------|
| Consistency | Strong (ACID) | Eventual (compensatable) |
| Availability | Low (blocking) | High (no global lock) |
| Complexity | Simple logic, complex infra | Complex logic, simple infra |
| Isolation | Full | None (dirty reads possible between steps) |
| Use when | Strong consistency required | High availability, long transactions |

---

## Replication Deep Dive

### Topologies

```
LEADER-FOLLOWER (most common):
  Writes    Reads
    |      / | \
    v     v  v  v
  [Leader] -> [Follower 1]
           -> [Follower 2]
           -> [Follower 3]

MULTI-LEADER (geo-distributed):
  [Leader DC1] <--sync--> [Leader DC2] <--sync--> [Leader DC3]
      |                       |                       |
  [Followers]            [Followers]            [Followers]

LEADERLESS (Cassandra/Dynamo):
  Client writes to ANY node (or multiple via quorum)
  [Node1] [Node2] [Node3]  (all equal, no leader)
  Client reads from multiple, takes latest version
```

### Synchronous vs Asynchronous Replication

```
SYNCHRONOUS:
  Client -> Leader: WRITE
  Leader -> Replica: REPLICATE
  Replica -> Leader: ACK
  Leader -> Client: SUCCESS
  
  Guarantee: read from any replica always returns latest
  Cost: write latency = leader_write + network_RTT + replica_write

ASYNCHRONOUS:
  Client -> Leader: WRITE
  Leader -> Client: SUCCESS (immediately!)
  Leader -> Replica: REPLICATE (background)
  
  Guarantee: none (replica might be behind)
  Benefit: low write latency

SEMI-SYNCHRONOUS:
  Wait for at least 1 replica to ACK, rest async
  Compromise: guaranteed at least 2 copies, but not all
```

### Replication Lag Problems and Solutions

| Problem | Description | Solution |
|---------|-------------|----------|
| Read-your-writes | User writes then reads from stale replica | Route user's reads to leader after their writes (sticky session or causal token) |
| Monotonic reads | User sees X=2, then later sees X=1 (read from more-stale replica) | Pin user to one replica, or use version vectors |
| Consistent prefix reads | Cause appears after effect (replication order differs) | Use causal dependency tracking |

---

## Leader Election with Raft

Raft is the most widely used consensus algorithm in modern distributed systems (etcd, CockroachDB, TiKV, Consul).

### Core Concepts

```
Term: Monotonically increasing epoch number
  Term 1: Leader=A    Term 2: Leader=B    Term 3: Leader=A
  
Each node is in one state:
  FOLLOWER -> CANDIDATE -> LEADER
```

### Leader Election Protocol

```
Normal operation (Leader A is healthy):
  Leader A: sends heartbeat AppendEntries to all followers every 150ms
  Followers: reset election timeout on each heartbeat

Leader A crashes:

  Follower B: election timeout expires (no heartbeat for 300ms)
  Follower B: increments term, becomes CANDIDATE, votes for self
  
  B -> C: RequestVote(term=2, candidateId=B, lastLogIndex=5, lastLogTerm=1)
  B -> D: RequestVote(term=2, candidateId=B, lastLogIndex=5, lastLogTerm=1)
  B -> E: RequestVote(term=2, candidateId=B, lastLogIndex=5, lastLogTerm=1)
  
  C -> B: VoteGranted=true  (B's log is at least as up-to-date)
  D -> B: VoteGranted=true
  E -> B: VoteGranted=false (E has higher lastLogTerm)
  
  B received majority (3 out of 5, including self) -> B becomes LEADER
  B: starts sending heartbeats
```

### Log Replication (AppendEntries)

```
Leader                    Follower
  |                          |
  | AppendEntries:           |
  | - term=2                 |
  | - prevLogIndex=5         |
  | - prevLogTerm=1          |
  | - entries=[{term:2,      |
  |            cmd:"SET x=3"}]|
  |------------------------->|
  |                          | Check: do I have entry at
  |                          | index 5 with term 1?
  |                          | YES -> append new entries
  |                          | NO -> reject (leader backs up)
  |                          |
  | Success=true             |
  |<-------------------------|
  |                          |
  | (once majority ACK:      |
  |  entry is COMMITTED,     |
  |  apply to state machine) |
```

### Safety Guarantee

**Election restriction**: A candidate cannot win election unless its log contains all committed entries. Ensured by:
- Voters reject candidates whose log is less up-to-date (compare lastLogTerm, then lastLogIndex)
- Majority overlap: if entry E is committed (replicated to majority), any future leader's voters must include at least one node with E

**Result**: Once committed, an entry can never be lost (even through leader changes).

---

## Conflict Resolution and CRDTs

### The Problem with Multi-Leader

When two leaders independently write to the same key:

```
Leader 1 (DC-US): SET user.name = "Bob"    (timestamp T1)
Leader 2 (DC-EU): SET user.name = "Robert" (timestamp T2)

Network partition heals -> CONFLICT! Which value wins?
```

### Last-Write-Wins (LWW)

Simple: highest timestamp wins. **Problem**: clock skew can lose valid writes.

### CRDTs (Conflict-Free Replicated Data Types)

CRDTs are data structures mathematically guaranteed to converge without coordination.

**G-Counter (Grow-only Counter)**:
```
Each node maintains its own counter:
  Node A: {A:3, B:0, C:0}
  Node B: {A:0, B:5, C:0}
  Node C: {A:0, B:0, C:2}

Value = sum of all entries = 3 + 5 + 2 = 10

Merge: element-wise MAX
  merge({A:3,B:2,C:0}, {A:1,B:5,C:2}) = {A:3,B:5,C:2}

Converges regardless of merge order (commutative, associative, idempotent)
```

**PN-Counter (Positive-Negative Counter)**:
```
Two G-Counters: one for increments (P), one for decrements (N)
Value = sum(P) - sum(N)

P: {A:5, B:3, C:1}  -> total increments = 9
N: {A:1, B:0, C:2}  -> total decrements = 3
Value = 9 - 3 = 6
```

**LWW-Register (Last-Writer-Wins Register)**:
```
Store (value, timestamp) pairs
Merge: keep entry with highest timestamp

Node A: ("Bob", T=100)
Node B: ("Robert", T=105)
Merge: ("Robert", T=105) wins

Simple but lossy - concurrent writes with lower timestamps silently discarded.
```

**OR-Set (Observed-Remove Set)**:
```
Problem: in a replicated set, concurrent add("x") and remove("x") -> conflict

OR-Set solution: each element tagged with unique ID
  Add("x"): add (x, unique_tag_1)
  Remove("x"): remove ALL currently observed tags for x

Node A: add("x") -> {(x, tag_A1)}
Node B: add("x") -> {(x, tag_B1)}
Node A: remove("x") -> removes {(x, tag_A1)} only (hasn't seen tag_B1)
Merge: {(x, tag_B1)} survives! (add on B was concurrent with remove on A)

Semantics: "add wins over concurrent remove" (observed-remove)
```

---

## Cassandra Tunable Consistency

### Quorum Math

```
N = total replicas (replication factor)
W = write quorum (ACKs needed for write success)
R = read quorum (replicas read for a read operation)

STRONG CONSISTENCY when: W + R > N

Proof: Why does W + R > N guarantee seeing the latest write?

  With N replicas, a write that ACKs from W replicas is stored on W nodes.
  A read that queries R replicas reads from R nodes.
  
  If W + R > N, then W and R MUST overlap by at least 1 node:
    overlap = W + R - N > 0
  
  That overlapping node has the latest write!
  
  Example: N=3, W=2, R=2
    W + R = 4 > 3 = N
    overlap = 4 - 3 = 1 (at least 1 node has latest in every read)

  Visual (N=3 nodes: [A] [B] [C]):
    Write quorum: [A*] [B*] [C ]  <- W=2, stored on A and B
    Read quorum:  [A ] [B*] [C*]  <- R=2, reads B and C
                          ^
                     overlap! B has the latest write
```

### Consistency Level Options

| Consistency | W or R | Guarantee |
|-------------|--------|-----------|
| ONE | 1 | Fastest, weakest |
| QUORUM | ceil((N+1)/2) | Strong with matching read |
| ALL | N | Strongest, slowest, no fault tolerance |
| LOCAL_QUORUM | Quorum within DC | Strong within DC |
| EACH_QUORUM | Quorum in each DC | Cross-DC strong |

### Trade-off Matrix (N=3)

| Config | Consistency | Availability | Latency |
|--------|-------------|--------------|---------|
| W=1, R=3 | Strong (1+3>3) | Write: high, Read: low (all must respond) | Fast writes |
| W=3, R=1 | Strong (3+1>3) | Write: low, Read: high | Fast reads |
| W=2, R=2 | Strong (2+2>3) | Balanced | Balanced |
| W=1, R=1 | Eventual (1+1<3) | Highest | Fastest |

---

## Anti-Entropy and Repair

### Merkle Trees for Replica Sync

When replicas diverge (network issues, failed writes), need to efficiently find differences:

```
Merkle Tree (Hash Tree):
Each leaf = hash of a key range's data
Each parent = hash of children's hashes

         Root: H(H_AB, H_CD)
              /            \
      H_AB: H(H_A, H_B)    H_CD: H(H_C, H_D)
        /       \              /         \
   H_A: hash   H_B: hash   H_C: hash   H_D: hash
   (keys 0-25) (keys 26-50) (keys 51-75) (keys 76-100)

Comparison between replicas:
  1. Compare root hashes -> different? dig deeper
  2. Compare children -> H_AB matches, H_CD different
  3. Compare H_CD's children -> H_C different, H_D matches
  4. Only sync keys 51-75 (where H_C differs)

Efficiency: O(log N) comparisons to find divergent ranges
  (vs O(N) full comparison)
```

### Hinted Handoff

When a write's target node is temporarily down:

```
Client writes key K (should go to Node C, which is down):

  Node A (coordinator):
    - Cannot reach Node C
    - Stores write locally as a "hint":
      hint = {target: C, key: K, value: V, timestamp: T}
    - Returns success to client (maintains availability)
    
  When Node C comes back online:
    - Node A replays all stored hints to Node C
    - Node C is now caught up
    - Hints are deleted from Node A

Limitation: hints expire (typically 3 hours). If node is down longer,
full anti-entropy repair needed.
```

---

## Files
- [hash_sharding.cpp](hash_sharding.cpp)
- [range_sharding.cpp](range_sharding.cpp)
- [leader_follower_replication.cpp](leader_follower_replication.cpp) - simulated

## Interview Questions
1. Why is "CA" not really achievable in distributed systems?
2. PACELC vs CAP - what is the addition and why does it matter?
3. Sharding key selection - what makes a good shard key? Give examples.
4. Hot shard problem and mitigations - explain key salting and dedicated cache.
5. How does Cassandra's tunable consistency work? Derive the quorum formula.
6. Replica lag - how to detect and handle? What is read-your-writes consistency?
7. Multi-leader conflict resolution strategies - compare LWW vs CRDTs.
8. Quorum: N=5, W=3, R=3 - is it strong consistency? What about W=2, R=2?
9. Re-sharding live - how to do it without downtime? Explain virtual shards.
10. Explain Raft leader election. What prevents split-brain?
11. What are CRDTs? Explain G-Counter and OR-Set. When would you use them?
12. Two-Phase Commit vs Saga - when to use each? What are the trade-offs?
13. How do Merkle trees enable efficient replica synchronization?
14. What is hinted handoff and when does it fail?
15. Explain the safety guarantee in Raft - why can committed entries never be lost?

## Daily Assignment
1. Implement hash sharding for 1M keys across 4 nodes - measure distribution std-dev.
2. Add a `Cluster` class with leader-follower replication; simulate write -> async replicate -> read from follower (show lag).
3. Compute quorum: given N/W/R, validate strong consistency rule and print overlap count.
4. Implement a simple G-Counter CRDT with merge operation across 3 simulated nodes.
5. Bonus: Implement simplified Raft leader election (term, RequestVote, timeout-based election).
