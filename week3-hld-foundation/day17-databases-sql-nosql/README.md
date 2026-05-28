# Day 17 - Databases: SQL vs NoSQL + Indexing Internals

## Table of Contents
- [Database Fundamentals](#database-fundamentals)
- [B-Tree Deep Dive](#b-tree-deep-dive)
- [LSM-Tree Deep Dive](#lsm-tree-deep-dive)
- [Write-Ahead Log (WAL)](#write-ahead-log-wal)
- [MVCC - Multi-Version Concurrency Control](#mvcc---multi-version-concurrency-control)
- [SQL Query Execution Pipeline](#sql-query-execution-pipeline)
- [Indexing Internals](#indexing-internals)
- [Transaction Isolation Levels](#transaction-isolation-levels)
- [NoSQL Data Modeling](#nosql-data-modeling)
- [Connection Pooling](#connection-pooling)
- [Files](#files)
- [Interview Questions](#interview-questions)
- [Daily Assignment](#daily-assignment)

---

## Database Fundamentals

### SQL (Relational)
- Schema-on-write: structure defined before data insertion
- ACID guarantees: Atomicity, Consistency, Isolation, Durability
- Normalization reduces redundancy; joins reconstruct related data
- Examples: PostgreSQL, MySQL, Oracle, SQL Server, CockroachDB
- Best for: financial records, OLTP, strong consistency, complex queries with joins

### NoSQL Families

| Type | Examples | Data Model | Use Case |
|------|----------|------------|----------|
| **Key-Value** | Redis, DynamoDB, Riak | HashMap-like | Sessions, caches, simple lookups |
| **Document** | MongoDB, Couchbase, Firestore | JSON/BSON documents | Semi-structured, evolving schema |
| **Wide-Column** | Cassandra, HBase, ScyllaDB | Column families, sparse rows | Massive writes, time-series |
| **Graph** | Neo4j, ArangoDB, JanusGraph | Nodes + Edges + Properties | Social networks, fraud detection |

### ACID vs BASE

| Property | ACID (SQL) | BASE (NoSQL) |
|----------|-----------|--------------|
| Focus | Correctness | Availability |
| Consistency | Strong (after txn) | Eventual |
| Trade-off | Higher latency | Lower latency |
| Example | Bank transfer | Social media likes count |

---

## B-Tree Deep Dive

B-Trees are the default index structure in almost every relational database (PostgreSQL, MySQL InnoDB, Oracle, SQL Server).

### Why B-Trees?

Disks (even SSDs) are slow for random access but fast for sequential reads. B-Trees minimize disk I/O by:
1. High fanout (hundreds of keys per node) = short tree
2. Each node = one disk page (4KB or 8KB)
3. Only 3-4 disk reads for billions of rows

### Node Structure

```
B-Tree Node (order m = maximum children per node):

+----------------------------------------------------------+
| P0 | K1 | P1 | K2 | P2 | K3 | P3 | ... | Km-1 | Pm-1  |
+----------------------------------------------------------+
  |       |       |       |                          |
  v       v       v       v                          v
[<K1]  [K1..K2] [K2..K3] [K3..K4]              [>=Km-1]

Where:
  Ki = key values (sorted)
  Pi = pointers to child nodes (or data pages in leaf nodes)
  
Leaf nodes also contain: row pointers (ctid in PostgreSQL) or actual row data (clustered index)
```

### Fanout Calculation

```
Page size = 8192 bytes (PostgreSQL default)
Key size = 8 bytes (bigint)
Pointer size = 6 bytes (page number + offset)
Header = 24 bytes

Usable space = 8192 - 24 = 8168 bytes
Keys per node = floor(8168 / (8 + 6)) = floor(8168 / 14) = 583

Fanout (branching factor) = 584 children per node
```

### Height Formula

```
Height h = ceil(log_B(N))

Where B = branching factor, N = number of keys

Example: 1 billion rows (N = 10^9), fanout B = 584
  h = ceil(log_584(10^9))
  h = ceil(9 / log10(584))
  h = ceil(9 / 2.766)
  h = ceil(3.25)
  h = 4

Only 4 levels! Root + 3 disk reads to find any row among 1 billion.
Root is always cached in memory, so effectively 3 disk reads.
```

### Split Operation (Step by Step)

When a node becomes full (m keys) and we insert a new key:

```
Before: Node full (order 5, max 4 keys)
+-----+-----+-----+-----+
| 10  | 20  | 30  | 40  |
+-----+-----+-----+-----+

Insert 25:

Step 1: Find position -> between 20 and 30
Step 2: Node is full -> SPLIT at median (key 25 becomes median here)

After split:
                    [25] <- promoted to parent
                   /    \
        +---------+      +---------+
        | 10 | 20 |      | 30 | 40 |
        +---------+      +---------+

If parent is also full -> recursive split up the tree
If root splits -> tree height increases by 1 (only way tree grows taller)
```

### Merge Operation (Deletion)

When a node has fewer than ceil(m/2) - 1 keys after deletion:

```
Before:
          [30]
         /    \
  [10|20]      [40]  <- underfull after deleting 50!

Options:
1. Borrow from sibling (if sibling has spare keys)
   - Rotate through parent
   
2. Merge with sibling (if sibling at minimum)
   - Merge two children + parent key into one node
   - Parent loses a key (may cascade)
```

### B+ Tree (What Databases Actually Use)

```
Internal nodes: only keys + child pointers (NO data)
Leaf nodes: keys + data pointers + linked list to next leaf

        [30 | 60]               <- internal (routing only)
       /    |     \
  [10|20]  [30|40|50]  [60|70|80]  <- leaf nodes (have data)
     |----->|--------->|             <- linked for range scans

Advantages over B-Tree:
1. Internal nodes fit MORE keys (no data pointers) = higher fanout
2. Range queries: follow leaf linked list (no tree traversal)
3. All data at same depth = predictable I/O
```

---

## LSM-Tree Deep Dive

LSM-Trees (Log-Structured Merge Trees) optimize for write-heavy workloads. Used by: Cassandra, RocksDB, LevelDB, HBase, ScyllaDB, InfluxDB.

### Architecture

```
WRITE PATH:
                                                    
  Write -> WAL (sequential disk write, durability)
       |
       v
  MemTable (in-memory sorted structure: Red-Black Tree or SkipList)
       |
       | (when MemTable full, e.g., 64MB)
       v
  Immutable MemTable (read-only, being flushed)
       |
       | (flush to disk as sorted file)
       v
  Level 0 SSTables (may overlap key ranges)
       |
       | (compaction)
       v
  Level 1 SSTables (non-overlapping, sorted)
       |
       | (compaction)
       v
  Level 2 SSTables (10x size of Level 1)
       |
       v
  ...
```

### SSTable (Sorted String Table) Format

```
+---------------------------------------------------+
| Data Block 1 (sorted key-value pairs, ~4KB)       |
+---------------------------------------------------+
| Data Block 2                                       |
+---------------------------------------------------+
| ...                                                |
+---------------------------------------------------+
| Data Block N                                       |
+---------------------------------------------------+
| Meta Block (bloom filter)                          |
+---------------------------------------------------+
| Meta Block (stats)                                 |
+---------------------------------------------------+
| Index Block (key -> block offset mapping)          |
+---------------------------------------------------+
| Footer (offsets to index and meta blocks)          |
+---------------------------------------------------+
```

### Compaction Strategies

**Size-Tiered Compaction (STCS)**:
```
L0: [SS1] [SS2] [SS3] [SS4]   <- when 4 tables accumulate
          |
          v  (merge all 4 into 1 larger table)
L1: [MERGED_SS]
          |
          v  (when L1 has enough tables)
L2: [BIGGER_MERGED_SS]

Pros: Good write amplification
Cons: Space amplification (old + new coexist during merge), read amplification (many files to check)
Used by: Cassandra (default)
```

**Leveled Compaction (LCS)**:
```
L0: [overlap allowed]
L1: |---[a-f]---|---[g-m]---|---[n-z]---|   (non-overlapping, ~10MB each)
L2: |[a-c][d-f][g-i][j-m][n-p][q-s][t-z]|  (10x total size of L1)

Compaction: pick one L1 file, merge with overlapping L2 files
Result: sorted, non-overlapping files in L2

Pros: Low space amplification, bounded read amplification
Cons: Higher write amplification (each key written ~10x per level)
Used by: RocksDB (default), LevelDB
```

### Read/Write Amplification Analysis

```
Write Amplification (WA) = bytes written to disk / bytes written by app

Size-Tiered: WA ~ O(number of tiers) = relatively low
Leveled:     WA ~ O(10 * num_levels) = higher (each compaction rewrites data)

Read Amplification (RA) = disk reads per query

Size-Tiered: RA ~ O(num_sstables) = potentially high (many files to check)
Leveled:     RA ~ O(num_levels) = bounded (one file per level)

Space Amplification (SA) = disk used / actual data size

Size-Tiered: SA can be 2x (during compaction, old + new coexist)
Leveled:     SA ~ 1.1x (minimal overhead)
```

### Bloom Filters for Read Optimization

Problem: Reading from LSM-tree may require checking many SSTables.
Solution: Bloom filter per SSTable tells you if a key is definitely NOT in that file.

```
Bloom Filter: bit array + k hash functions

Insert key:  hash1(key)=3, hash2(key)=7, hash3(key)=11
             Set bits[3]=1, bits[7]=1, bits[11]=1

Query key:   Check bits[3], bits[7], bits[11]
             All 1? -> MAYBE present (check file)
             Any 0? -> DEFINITELY not present (skip file!)

False positive rate: (1 - e^(-kn/m))^k
  m = bit array size, n = elements, k = hash functions
  
Typical config: 10 bits per key, 3 hash functions -> ~1% false positive rate
```

---

## Write-Ahead Log (WAL)

### Crash Recovery Mechanism

The WAL ensures durability: every change is logged BEFORE modifying data pages.

```
WRITE OPERATION:
1. Write change record to WAL (sequential I/O, fast)
2. fsync() WAL to disk (guarantees durability)
3. Modify data page in buffer pool (in memory)
4. Return success to client

CRASH RECOVERY:
1. Read WAL from last checkpoint
2. Replay all committed transactions (REDO)
3. Rollback all uncommitted transactions (UNDO)
4. Database is now consistent
```

**Why WAL works**:
- WAL writes are sequential (fast on disk, even HDD)
- Data page writes are random (slow, can be batched/deferred)
- On crash: replay WAL to reconstruct any lost in-memory changes

### Checkpointing

Checkpoints reduce recovery time by establishing a known-good point:

```
Timeline:
  WAL: [TX1][TX2][TX3][CHECKPOINT][TX4][TX5][TX6][CRASH]
                            ^
                            |
  Recovery starts HERE (not from beginning of WAL)
  Only need to replay TX4, TX5, TX6

Checkpoint process:
1. Write all dirty pages from buffer pool to disk
2. Record checkpoint position in WAL
3. Old WAL segments before checkpoint can be recycled
```

### Group Commit

Optimization: batch multiple transactions' fsync into one:

```
Without group commit:        With group commit:
TX1 -> write WAL -> fsync    TX1 -> write WAL --|
TX2 -> write WAL -> fsync    TX2 -> write WAL --|--> single fsync
TX3 -> write WAL -> fsync    TX3 -> write WAL --|
(3 fsyncs)                   (1 fsync for all 3)

Impact: fsync is ~5ms on HDD. Without grouping: 200 TPS max.
With grouping: can batch 100 TXs per fsync = 20,000 TPS.
```

---

## MVCC - Multi-Version Concurrency Control

### How PostgreSQL Implements MVCC

PostgreSQL stores multiple versions of each row simultaneously. Readers never block writers, writers never block readers.

### Tuple Headers (xmin/xmax)

```
Each row version (tuple) has hidden system columns:

+--------+--------+--------+-------+------------------+
|  xmin  |  xmax  |  cid   | ctid  |   user data      |
+--------+--------+--------+-------+------------------+

xmin = transaction ID that CREATED this tuple version
xmax = transaction ID that DELETED/UPDATED this tuple (0 if alive)
cid  = command ID within transaction (for same-txn visibility)
ctid = physical location (page, offset) - self or newer version
```

### Visibility Rules

A tuple is VISIBLE to transaction T if:
```
1. xmin is committed AND xmin < T's snapshot
   (tuple was created before our snapshot)

2. xmax is either:
   a. 0 or invalid (tuple not deleted), OR
   b. xmax is NOT committed, OR
   c. xmax >= T's snapshot (deletion happened after our snapshot)
```

### Example: UPDATE Creates New Version

```
Initial state:
  Page: [Row1: xmin=100, xmax=0, data="Alice"]

Transaction 200 executes: UPDATE users SET name='Bob' WHERE id=1

After update:
  Page: [Row1: xmin=100, xmax=200, data="Alice"]    <- old version (dead)
        [Row2: xmin=200, xmax=0,   data="Bob"]      <- new version (live)

Transaction 150 (started before 200) still sees "Alice"!
  (xmin=100 < 150, xmax=200 > 150 -> visible)

Transaction 250 (started after 200 committed) sees "Bob"
  (Row1: xmax=200 < 250 -> not visible)
  (Row2: xmin=200 < 250 -> visible)
```

### VACUUM (Garbage Collection)

Dead tuples (old versions no longer visible to any transaction) waste space:

```
VACUUM process:
1. Find tuples where xmax < oldest active transaction ID
   (guaranteed no transaction can ever see them again)
2. Mark their space as reusable in the Free Space Map (FSM)
3. Update Visibility Map (VM) for index-only scans

autovacuum: background process, triggers when dead tuple ratio exceeds threshold
Problem: long-running transactions prevent vacuum from reclaiming space (xmax threshold stuck)
```

---

## SQL Query Execution Pipeline

```
SQL Query: SELECT name FROM users WHERE age > 25 ORDER BY name LIMIT 10

+--------------------------------------------------+
|  1. PARSING                                       |
|  - Lexical analysis (tokenize SQL)               |
|  - Syntax analysis (build parse tree)            |
|  - Semantic analysis (resolve table/column names)|
+--------------------------------------------------+
              |
              v
+--------------------------------------------------+
|  2. PLANNING / OPTIMIZATION                       |
|  - Generate possible execution plans             |
|  - Estimate cost of each plan                    |
|  - Choose lowest-cost plan                       |
+--------------------------------------------------+
              |
              v
+--------------------------------------------------+
|  3. EXECUTION (Volcano/Iterator Model)            |
|  - Each operator is an iterator: open/next/close |
|  - Data flows bottom-up through operator tree    |
+--------------------------------------------------+
```

### Query Optimizer: Rule-Based vs Cost-Based

**Rule-Based (heuristic)**:
- Apply fixed transformation rules regardless of data
- Example: "Always use index if available", "push predicates down"
- Simple but suboptimal for complex queries

**Cost-Based (modern databases)**:
- Estimate cost of each plan using statistics:
  - Table size (pg_class.reltuples)
  - Column cardinality (pg_stats.n_distinct)
  - Value distribution (histograms)
  - Correlation (physical vs logical ordering)
- Cost model: `cost = seq_page_cost * pages + cpu_tuple_cost * tuples`
- Picks plan with lowest estimated total cost

### Volcano (Iterator) Model

```
                    LIMIT 10
                       |
                    SORT (name)
                       |
                    FILTER (age > 25)
                       |
                  SEQ SCAN (users)

Execution: LIMIT calls SORT.next()
           SORT calls FILTER.next() repeatedly until sorted
           FILTER calls SEQ_SCAN.next(), applies predicate
           SEQ_SCAN reads next tuple from heap

Each operator: open() -> next() -> next() -> ... -> close()
Pipeline: tuples flow one at a time (no materialization needed unless sort/hash)
```

---

## Indexing Internals

### Covering Index (Index-Only Scan)

```
CREATE INDEX idx_users_age_name ON users(age, name);

Query: SELECT name FROM users WHERE age > 25;

Without covering index:
  1. Scan index for age > 25 -> get ctid (row locations)
  2. Random I/O: fetch each row from heap to get 'name'
  
With covering index (name IN the index):
  1. Scan index for age > 25 -> read 'name' directly from index leaf
  2. No heap access needed! (index-only scan)

PostgreSQL: CREATE INDEX ... INCLUDE(name)  -- include non-key columns
```

### Partial Index

```
CREATE INDEX idx_active_users ON users(email) WHERE active = true;

Only indexes rows where active=true (much smaller index)
Useful when queries always filter on a condition

Example: 1M users, only 10K active
  Full index: 1M entries
  Partial index: 10K entries (100x smaller, fits in memory)
```

### Composite Index and Leftmost Prefix Rule

```
CREATE INDEX idx_abc ON table(a, b, c);

This index supports queries on:
  WHERE a = ?                    (uses index)
  WHERE a = ? AND b = ?         (uses index)
  WHERE a = ? AND b = ? AND c = ? (uses index, full)
  WHERE a = ? AND c = ?         (uses index for 'a' only)
  
Does NOT support:
  WHERE b = ?                    (cannot skip 'a')
  WHERE c = ?                    (cannot skip 'a' and 'b')
  WHERE b = ? AND c = ?         (cannot skip 'a')

Think of it like a phone book sorted by (last_name, first_name, city):
  Can find all "Smith" -> yes
  Can find "Smith, John" -> yes
  Can find all "John" (any last name) -> NO, must scan all
```

### Index-Only Scans and Visibility Map

```
PostgreSQL problem: even with covering index, must check if tuple is visible
(MVCC means old versions may exist)

Solution: Visibility Map (VM)
  - One bit per page: "all tuples on this page are visible to all transactions"
  - If VM bit set: no need to check heap (pure index-only scan)
  - VACUUM sets VM bits after cleaning dead tuples

Impact: fresh table after VACUUM -> 100% index-only scans (blazing fast)
        table with many updates -> VM bits cleared -> heap fetches needed
```

---

## Transaction Isolation Levels

| Level | Dirty Read | Non-Repeatable Read | Phantom Read | Implementation |
|-------|-----------|-------------------|--------------|----------------|
| Read Uncommitted | Yes | Yes | Yes | No locks (rarely used) |
| Read Committed | No | Yes | Yes | Statement-level snapshot |
| Repeatable Read | No | No | Yes* | Transaction-level snapshot |
| Serializable | No | No | No | SSI (Serializable Snapshot Isolation) |

**PostgreSQL implementation**:
- Read Committed: new snapshot per statement (sees other committed txns between statements)
- Repeatable Read: snapshot at first query (frozen view for entire transaction)
- Serializable: detects read-write dependencies, aborts transactions that would cause anomalies (SSI algorithm)

*PostgreSQL's Repeatable Read actually prevents phantoms too (snapshot-based), unlike MySQL's gap locks approach.

---

## NoSQL Data Modeling

### Denormalization Patterns

In NoSQL, you model data based on query patterns (not normalization):

```
SQL (normalized):
  users: {id, name, email}
  orders: {id, user_id, total, date}
  order_items: {id, order_id, product_id, qty}

NoSQL (denormalized for "get user's recent orders" query):
  user_orders: {
    PK: user_id,
    SK: order_date#order_id,
    name: "Alice",
    total: 59.99,
    items: [{product: "Book", qty: 2}, ...]
  }
  
One read returns everything. No joins. Trade-off: data duplication.
```

### Wide-Row Design (Cassandra Pattern)

```
Time-series data:
  Partition Key: sensor_id
  Clustering Key: timestamp (sorted within partition)

  sensor_readings:
  +------------+----------------------------+-------+
  | sensor_id  |       timestamp            | value |
  +============+============================+=======+
  | sensor_1   | 2024-01-01T00:00:00        | 23.5  |
  | sensor_1   | 2024-01-01T00:01:00        | 23.6  |
  | sensor_1   | 2024-01-01T00:02:00        | 23.4  |
  +------------+----------------------------+-------+
  | sensor_2   | 2024-01-01T00:00:00        | 18.2  |
  +------------+----------------------------+-------+

Query: SELECT * FROM sensor_readings 
       WHERE sensor_id = 'sensor_1' 
       AND timestamp > '2024-01-01' AND timestamp < '2024-01-02'

Efficient: reads one partition sequentially (data sorted by timestamp on disk)
```

### Time-Series Bucketing Pattern

```
Problem: unbounded partition growth (sensor writes forever)
Solution: bucket by time period

  Partition Key: (sensor_id, date_bucket)
  Clustering Key: timestamp

  sensor_readings:
  PK: (sensor_1, 2024-01-01) -> only 1 day of data per partition
  PK: (sensor_1, 2024-01-02) -> next day in separate partition

Benefits:
  - Bounded partition size
  - Easy TTL/deletion (drop old partitions)
  - Even distribution across cluster
```

---

## Connection Pooling

### Why Pooling Matters

```
Without pooling:
  Each request: TCP handshake + TLS + auth + query + close
  Cost: ~5-20ms overhead per connection
  PostgreSQL: fork() new process per connection (~10MB RSS each)
  At 1000 concurrent: 1000 processes * 10MB = 10GB RAM just for connections!

With pooling:
  Pool maintains N persistent connections
  Requests borrow connection, use it, return it
  Cost: ~0ms overhead (already connected)
  At 1000 concurrent with pool of 50: 50 processes, 500MB RAM
```

### PgBouncer Modes

```
+-------------------------------------------+
|            PgBouncer Pool Modes            |
+-------------------------------------------+

1. SESSION MODE:
   Client gets dedicated backend for entire session
   Client: CONNECT -> [use backend X for all queries] -> DISCONNECT
   Pooling benefit: only when clients disconnect/reconnect
   Use: when using session-level features (prepared statements, temp tables)

2. TRANSACTION MODE:
   Client gets backend for duration of one transaction
   Client: BEGIN -> [backend X] -> COMMIT -> (backend X returned to pool)
   Next TX might use different backend
   Best for: most web applications (stateless between TXs)
   Cannot use: session-level features, prepared statements

3. STATEMENT MODE:
   Client gets backend for one statement only
   Client: SELECT... -> [backend X] -> (returned immediately)
   Most aggressive pooling, most restrictions
   Cannot use: multi-statement transactions, session features
```

### Pool Sizing Formula

```
Optimal pool size = ((core_count * 2) + effective_spindle_count)

Example: 4-core server with SSD (effective spindles ~= 1-2):
  Pool size = (4 * 2) + 2 = 10

Why small pools are better:
  - Less context switching
  - Better CPU cache utilization
  - Less lock contention
  - PostgreSQL internal: each backend has its own proc entry, catalog cache

HikariCP recommendation: pool_size = 10 handles most workloads
  (50 connections often SLOWER than 10 due to contention)
```

---

## Files
- [btree_index_demo.cpp](btree_index_demo.cpp) - using `std::map` as B-tree analogue
- [hash_index_demo.cpp](hash_index_demo.cpp)
- [inverted_index.cpp](inverted_index.cpp) - full-text search
- [in_memory_kv.cpp](in_memory_kv.cpp) - simple Redis-like KV with TTL

## Interview Questions
1. SQL vs NoSQL - when to choose which? Give specific examples.
2. Why are joins expensive in distributed NoSQL? What is the alternative?
3. Explain B-tree vs LSM-tree trade-offs. When does each win?
4. What is a covering index? How does it enable index-only scans?
5. ACID vs BASE - real example of each in production systems.
6. How does indexing affect write throughput? Quantify the impact.
7. Explain MVCC - how does PostgreSQL handle concurrent readers and writers?
8. What is denormalization and when do you do it? What are the costs?
9. Phantom read - example and how Serializable prevents it.
10. Why is Cassandra fast for writes? (LSM-tree, append-only, no read-before-write)
11. Explain the WAL - why is it necessary for crash recovery?
12. What is write amplification in LSM-trees? How do compaction strategies differ?
13. How does the Bloom filter optimize reads in LSM-tree based databases?
14. What is the leftmost prefix rule for composite indexes?
15. Connection pooling - why is a pool of 10 often faster than 100?

## Daily Assignment
1. Build an in-memory KV store with `set/get/del/expire(ttl)`. Add LRU eviction when capacity exceeded.
2. Build an inverted index for a list of documents - support `search("word1 word2")` returning matching doc IDs (AND semantics).
3. Implement a `Table` with primary B-tree index (`std::map`) + secondary hash index (`std::unordered_map`). Insert 10k rows and benchmark lookup times.
4. Implement a simple LSM-tree: in-memory sorted map that flushes to sorted file on disk when full. Support get() that checks memory first, then files.
5. Bonus: Implement group commit - batch multiple writes and flush WAL once.
