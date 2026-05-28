# Day 17 - Databases: SQL vs NoSQL + Indexing

## SQL (Relational)
- Schema-on-write, ACID, joins, normalization
- Examples: PostgreSQL, MySQL, Oracle, SQL Server
- Best for: financial records, OLTP, strong consistency, complex joins

## NoSQL Families
| Type | Examples | Use case |
|------|----------|----------|
| **Key-Value** | Redis, DynamoDB, Riak | Sessions, caches, simple lookups |
| **Document** | MongoDB, Couchbase | Semi-structured, evolving schema |
| **Wide-Column** | Cassandra, HBase, ScyllaDB | Massive writes, time-series, log data |
| **Graph** | Neo4j, ArangoDB, JanusGraph | Social, recommendations, fraud |

## ACID vs BASE
- **ACID**: Atomicity, Consistency, Isolation, Durability (RDBMS strength)
- **BASE**: Basically Available, Soft state, Eventual consistency (NoSQL trade-off)

## Indexing
| Index Type | Use Case |
|------------|----------|
| **B-Tree** | Range queries, default in most RDBMS |
| **Hash** | Exact lookups (`=`) |
| **Bitmap** | Low-cardinality columns (gender, status) |
| **Inverted** | Full-text search (Lucene, Elastic) |
| **Geo (R-Tree)** | Spatial queries |
| **LSM-Tree** | Write-heavy NoSQL (Cassandra, RocksDB) |

### Trade-offs
- More indexes -> faster reads, slower writes, more disk
- Composite index column order matters - leftmost prefix rule
- Index selectivity = unique values / total rows; high is better

## Transaction Isolation Levels (SQL)
| Level | Dirty Read | Non-Repeatable | Phantom |
|-------|-----------|----------------|---------|
| Read Uncommitted | Yes | Yes | Yes |
| Read Committed   | No  | Yes | Yes |
| Repeatable Read  | No  | No  | Yes |
| Serializable     | No  | No  | No  |

## Files
- [btree_index_demo.cpp](btree_index_demo.cpp) - using `std::map` as B-tree analogue
- [hash_index_demo.cpp](hash_index_demo.cpp)
- [inverted_index.cpp](inverted_index.cpp) - full-text search
- [in_memory_kv.cpp](in_memory_kv.cpp) - simple Redis-like KV with TTL

## Interview Questions
1. SQL vs NoSQL - when to choose which?
2. Why are joins expensive in distributed NoSQL?
3. Explain B-tree vs LSM-tree trade-offs.
4. What is a covering index?
5. ACID vs BASE - real example of each.
6. How does indexing affect write throughput?
7. Explain MVCC (multi-version concurrency control).
8. What is denormalization and when do you do it?
9. Phantom read - example and how Serializable prevents it.
10. Why is Cassandra fast for writes? (LSM, append-only, no read before write)

## Daily Assignment
1. Build an in-memory KV store with `set/get/del/expire(ttl)`. Add LRU eviction.
2. Build an inverted index for a list of documents - support `search("word1 word2")` returning matching doc IDs.
3. Implement a `Table` with primary B-tree index + secondary hash index. Insert 10k rows and benchmark.
