# Day 18 - Sharding, Replication, CAP

## CAP Theorem
In a distributed system you can pick **2 out of 3**:
- **C**onsistency - every read sees latest write
- **A**vailability - every request gets a response
- **P**artition tolerance - system works despite network splits

Real systems must tolerate partitions, so trade-off is **CP** vs **AP**.

| System | Choice |
|--------|--------|
| MongoDB (default) | CP |
| HBase, Zookeeper, etcd | CP |
| Cassandra, DynamoDB | AP (tunable) |
| MySQL/Postgres single node | CA (no P) |

## PACELC
Extends CAP: under Partition pick A or C; **Else** pick **L**atency or **C**onsistency.

## Sharding (Horizontal Partitioning)

### Strategies
| Strategy | Pros | Cons |
|----------|------|------|
| **Range** | Easy range scans | Hot spots (sequential keys) |
| **Hash** | Even distribution | No range scans |
| **Consistent Hash** | Minimal reshuffle on rebalance | More complex |
| **Geo / Directory** | Locality-aware | Custom routing logic |

### Challenges
- Cross-shard joins (avoid or use scatter-gather)
- Re-sharding (consistent hashing helps)
- Hot shard / hot key (salting, replication)

## Replication

### Topologies
- **Leader-Follower** (Master-Slave) - one writer, N readers (Postgres, MySQL)
- **Multi-Leader** - multiple writers, conflict resolution needed (CRDTs, last-write-wins)
- **Leaderless** (Cassandra, Dynamo) - any node accepts write, quorum reads/writes

### Sync vs Async Replication
- **Sync**: stronger consistency, higher write latency
- **Async**: faster writes, possible data loss / replica lag
- **Semi-sync**: ack after at least 1 replica

### Quorum
- N = replicas, W = write quorum, R = read quorum
- Strong consistency when **W + R > N**
- Common: N=3, W=2, R=2

## Files
- [hash_sharding.cpp](hash_sharding.cpp)
- [range_sharding.cpp](range_sharding.cpp)
- [leader_follower_replication.cpp](leader_follower_replication.cpp) - simulated

## Interview Questions
1. Why is "CA" not really achievable in distributed systems?
2. PACELC vs CAP - what's the addition?
3. Sharding key selection - what makes a good one?
4. Hot shard problem and mitigations.
5. How does Cassandra's tunable consistency work?
6. Replica lag - how to detect and handle?
7. Read-your-writes consistency vs eventual - examples.
8. Multi-leader conflict resolution strategies.
9. Quorum: N=5, W=3, R=3 - is it strong consistency? (Yes, W+R > N)
10. Re-sharding live - how to do it without downtime?

## Daily Assignment
1. Implement hash sharding for 1M keys across 4 nodes - measure distribution std-dev.
2. Add a `Cluster` class with leader-follower replication; simulate write -> async replicate -> reads.
3. Compute quorum: given N/W/R, validate strong consistency rule.
