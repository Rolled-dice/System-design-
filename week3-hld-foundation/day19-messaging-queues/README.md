# Day 19 - Messaging Queues + Event-Driven Architecture

## Table of Contents
- [Why Async Messaging](#why-async-messaging)
- [Kafka Internals](#kafka-internals)
- [RabbitMQ Deep Dive](#rabbitmq-deep-dive)
- [Delivery Guarantees](#delivery-guarantees)
- [Event-Driven Architecture Patterns](#event-driven-architecture-patterns)
- [Backpressure Mechanisms](#backpressure-mechanisms)
- [Files](#files)
- [Interview Questions](#interview-questions)
- [Daily Assignment](#daily-assignment)

---

## Why Async Messaging

### Synchronous vs Asynchronous Communication

```
SYNCHRONOUS (HTTP/RPC):
  Service A --request--> Service B --request--> Service C
  Service A <--response-- Service B <--response-- Service C
  
  Problems:
  - Temporal coupling: all must be up simultaneously
  - Cascading failures: C down = A fails
  - Tight coupling: A must know B exists

ASYNCHRONOUS (Message Queue):
  Service A --publish--> [Message Queue] --consume--> Service B
                                         --consume--> Service C
  
  Benefits:
  - Decoupling: A does not know about B or C
  - Buffering: queue absorbs bursts
  - Resilience: B can be down, messages wait
  - Fan-out: one event, many consumers
```

### Key Messaging Brokers

| Broker | Model | Throughput | Ordering | Use Case |
|--------|-------|-----------|----------|----------|
| **Kafka** | Distributed log | 1M+ msg/s | Per-partition | Event streaming, analytics |
| **RabbitMQ** | Smart broker (AMQP) | 50K msg/s | Per-queue | Task queues, RPC, routing |
| **AWS SQS** | Managed queue | Unlimited (managed) | FIFO option | Simple decoupling |
| **Redis Streams** | In-memory log | 500K+ msg/s | Per-stream | Low-latency, lightweight |
| **NATS** | Pub/Sub + JetStream | 10M+ msg/s | Per-subject | Microservice messaging |
| **Pulsar** | Distributed log | 1M+ msg/s | Per-partition | Multi-tenant, geo-replication |

---

## Kafka Internals

### Topic/Partition Architecture

```
Topic: "orders" (logical stream of events)

+---------------------------------------------------+
|                    Topic: orders                    |
+---------------------------------------------------+
| Partition 0: [msg0][msg1][msg2][msg3][msg4]...    |
| Partition 1: [msg0][msg1][msg2][msg3]...          |
| Partition 2: [msg0][msg1][msg2][msg3][msg4][msg5] |
+---------------------------------------------------+

Key concepts:
- Each partition is an ordered, immutable append-only log
- Messages within a partition have sequential offset (0, 1, 2, ...)
- Ordering guaranteed WITHIN a partition only
- Partitions distributed across broker nodes (parallelism)
- Partition key determines which partition: partition = hash(key) % num_partitions
```

### Segment Files on Disk

Each partition stored as sequence of segment files:

```
Partition 0 directory on disk:
  /kafka-logs/orders-0/
    00000000000000000000.log     <- segment 1 (offsets 0-999)
    00000000000000000000.index   <- sparse offset index
    00000000000000000000.timeindex <- timestamp index
    00000000000000001000.log     <- segment 2 (offsets 1000-1999)
    00000000000000001000.index
    00000000000000001000.timeindex
    00000000000000002000.log     <- segment 3 (active, being written)
    ...

Segment file format (.log):
  +--------+--------+-----------+-----+---------+
  | offset | size   | timestamp | key | value   |
  +--------+--------+-----------+-----+---------+
  | 0      | 45     | 170912345 | k1  | {...}   |
  | 1      | 62     | 170912346 | k2  | {...}   |
  | ...                                          |

Index file format (.index):
  [offset -> file position] (sparse, every Nth entry)
  To find offset 507: binary search index for <=507, then scan .log from there
```

### How Sequential I/O Enables High Throughput

```
Random I/O (traditional DB):
  Seek to page 42  (10ms seek)
  Read 4KB
  Seek to page 789 (10ms seek)
  Read 4KB
  -> ~100 IOPS on HDD = 400KB/s

Sequential I/O (Kafka):
  Read continuously from current position
  -> 100MB/s+ on HDD (250x faster!)
  -> 600MB/s+ on SSD

Why Kafka is sequential:
  - Writes: always append to end of segment (no random inserts)
  - Reads: consumers read in order (offset 100, 101, 102, ...)
  - OS page cache: sequential access triggers read-ahead prefetching
```

### Zero-Copy (sendfile Syscall)

```
TRADITIONAL data transfer (4 copies, 2 syscalls):
  Disk -> Kernel Page Cache -> User Space Buffer -> Kernel Socket Buffer -> NIC
           copy 1              copy 2               copy 3             copy 4

ZERO-COPY with sendfile() (2 copies, 1 syscall):
  Disk -> Kernel Page Cache ---------> NIC
           copy 1           DMA copy 2
  (data never enters user space!)

Impact: Kafka uses sendfile() for consumer reads
  - No CPU-intensive copying
  - Reduces context switches (no user/kernel boundary crossing)
  - Enables saturating network bandwidth on consumer reads
```

### Consumer Groups and Rebalancing

```
Topic: orders (6 partitions)
Consumer Group: "order-processor"

Scenario 1: 3 consumers in group
  Consumer A: Partition 0, 1
  Consumer B: Partition 2, 3  
  Consumer C: Partition 4, 5
  (each partition assigned to exactly one consumer in group)

Scenario 2: Consumer C dies -> REBALANCE
  Consumer A: Partition 0, 1, 4
  Consumer B: Partition 2, 3, 5
  (partitions redistributed among remaining consumers)

Scenario 3: 6 consumers in group
  Consumer A: Partition 0
  Consumer B: Partition 1
  Consumer C: Partition 2
  Consumer D: Partition 3
  Consumer E: Partition 4
  Consumer F: Partition 5
  (maximum parallelism = number of partitions)

Scenario 4: 8 consumers in group
  -> 2 consumers are IDLE (more consumers than partitions is wasteful)
```

**Rebalancing protocols**:
- **Eager rebalancing**: Stop ALL consumers, reassign all partitions (simple but causes downtime)
- **Cooperative rebalancing (incremental)**: Only affected partitions revoked and reassigned (minimal disruption)

### Exactly-Once Semantics (EOS)

Kafka achieves exactly-once through three mechanisms:

```
1. IDEMPOTENT PRODUCER:
   Producer assigns sequence number to each message per partition.
   Broker deduplicates: if seq_num already seen, silently discard.
   Prevents duplicates from retries.
   
   Config: enable.idempotence=true

2. TRANSACTIONAL API:
   Producer can atomically write to multiple partitions:
   
   producer.beginTransaction();
   producer.send(topic1, msg1);
   producer.send(topic2, msg2);
   producer.sendOffsetsToTransaction(consumer_offsets);  // commit consumed offsets
   producer.commitTransaction();  // atomic: all or nothing
   
   Config: transactional.id="my-transactional-producer"

3. READ_COMMITTED consumers:
   Consumer only sees messages from committed transactions.
   Uncommitted/aborted messages are invisible.
   
   Config: isolation.level=read_committed
```

### ISR (In-Sync Replicas) and Leader Election

```
Partition 0: Leader=Broker1, Replicas=[Broker1, Broker2, Broker3]

ISR (In-Sync Replica set): replicas that are caught up with leader
  - Replica is in ISR if it has replicated within replica.lag.time.max.ms (default 10s)
  - Leader tracks ISR membership

Normal state: ISR = {Broker1, Broker2, Broker3}

Broker3 falls behind (slow disk):
  ISR = {Broker1, Broker2}  (Broker3 removed from ISR)
  
Broker1 (leader) crashes:
  New leader elected from ISR only (Broker2 becomes leader)
  No data loss! (Broker2 had all committed messages)
  
  If ISR = {} (all replicas dead):
    Option 1 (default): wait for ISR member to recover (unavailable but no data loss)
    Option 2 (unclean leader election): elect any alive replica (available but possible data loss)
```

### Retention and Compaction

```
TIME-BASED RETENTION (default):
  Keep all messages for retention.ms (default 7 days)
  After 7 days, delete old segments
  Use: event streams, audit logs

SIZE-BASED RETENTION:
  Keep at most retention.bytes per partition
  Delete oldest segments when exceeded

LOG COMPACTION:
  Keep only LATEST value for each key (like a changelog):
  
  Before compaction:
    [K1:V1] [K2:V1] [K1:V2] [K3:V1] [K2:V2] [K1:V3]
  
  After compaction:
    [K1:V3] [K2:V2] [K3:V1]  (latest value per key retained)
  
  Use: CDC topics, state snapshots, KV stores backed by Kafka
```

---

## RabbitMQ Deep Dive

### AMQP Model

```
Producer -> Exchange -> Binding -> Queue -> Consumer

+----------+     +----------+     +--------+     +----------+
| Producer |---->| Exchange |---->| Queue  |---->| Consumer |
+----------+     +----------+     +--------+     +----------+
                      |                               
                      |           +--------+     +----------+
                      +---------->| Queue  |---->| Consumer |
                                  +--------+     +----------+

Exchange types determine routing logic:
```

### Exchange Types

```
1. DIRECT Exchange:
   Route by exact routing key match
   
   Producer publishes with routing_key="order.created"
   Binding: queue_A bound with key "order.created" -> receives message
   Binding: queue_B bound with key "order.shipped" -> does NOT receive

2. TOPIC Exchange:
   Pattern matching with wildcards (* = one word, # = zero or more)
   
   Binding patterns:
     queue_A: "order.*"        -> matches order.created, order.shipped
     queue_B: "order.created"  -> matches only order.created
     queue_C: "#"              -> matches everything (catch-all)
     queue_D: "*.created"      -> matches order.created, payment.created

3. FANOUT Exchange:
   Broadcast to ALL bound queues (ignores routing key)
   
   queue_A: receives message
   queue_B: receives message
   queue_C: receives message

4. HEADERS Exchange:
   Route based on message header attributes (not routing key)
   Match-all or match-any header conditions
```

### Acknowledgment Modes

```
AUTO-ACK (fire and forget):
  Broker removes message as soon as it is delivered
  Risk: consumer crashes before processing -> message lost

MANUAL ACK:
  Consumer processes message, then sends ACK
  Broker removes message only after ACK
  If consumer crashes -> message redelivered to another consumer
  
  basic_ack(delivery_tag)    -> message processed successfully
  basic_nack(delivery_tag)   -> reject, optionally requeue
  basic_reject(delivery_tag) -> reject single message

PREFETCH (QoS):
  basic_qos(prefetch_count=10)
  Broker sends max 10 unacked messages to this consumer
  Prevents fast producer overwhelming slow consumer
```

### Clustering vs Federation

```
CLUSTERING (single logical broker):
  All nodes share queues, exchanges, bindings
  Queues exist on one node, mirrored to others (HA queues)
  Single admin domain
  Requires low-latency network (same DC)

FEDERATION (connecting independent brokers):
  Separate brokers linked by federation plugin
  Messages forwarded between brokers on demand
  Cross-DC, cross-region
  Eventual consistency, tolerates high latency
```

---

## Delivery Guarantees

### Exactly-Once Impossibility (Two Generals Problem)

```
General A                    General B
  [Attack at dawn]              [?]
      |                          |
      |---messenger 1----------->|  (might be captured)
      |                          |
      |<--messenger 2 (ACK)-----|  (might be captured)
      |                          |
      |---messenger 3 (ACK-ACK)->|  (might be captured)
      |                          |
      ... (infinite regress)

Neither general can be CERTAIN the other received the message.
Similarly, in distributed systems:
  - Producer sends message to broker
  - Broker processes and ACKs
  - ACK might be lost -> producer retries -> DUPLICATE

TRUE exactly-once delivery is IMPOSSIBLE in an unreliable network.
```

### Practical Workarounds

```
"Effectively exactly-once" = at-least-once delivery + idempotent processing

Strategies:
1. IDEMPOTENT CONSUMER:
   - Assign unique message_id to each message
   - Consumer stores processed message_ids (dedup table)
   - On receive: if message_id in dedup_table, skip; else process and record
   
2. TRANSACTIONAL OUTBOX (see below):
   - Atomically write to DB + outbox in same transaction
   - Publisher polls outbox and sends messages
   - Idempotent at the source

3. KAFKA EXACTLY-ONCE:
   - Idempotent producer (dedup at broker by sequence number)
   - Transactions (atomic multi-partition writes)
   - End-to-end: read from input topic, process, write to output topic (all in one transaction)
```

---

## Event-Driven Architecture Patterns

### Event Sourcing

Instead of storing current state, store the sequence of events that led to current state:

```
TRADITIONAL (state-based):
  Account: {id: 1, balance: 150}
  
  After deposit $50: UPDATE balance=200
  Previous state LOST forever

EVENT SOURCING (event log):
  Event Log (append-only):
    1. AccountCreated {id:1, owner:"Alice"}
    2. MoneyDeposited {id:1, amount:100}
    3. MoneyWithdrawn {id:1, amount:50}
    4. MoneyDeposited {id:1, amount:100}  <- latest
  
  Current state = replay all events:
    0 + 100 - 50 + 100 = $150

  Can reconstruct state at ANY point in time!
  Can derive new views by replaying events with new logic!
```

**Projections (Read Models)**:
```
Event Log -> [Projection] -> Read-Optimized View

Same events, multiple views:
  Event Log -> [Balance Projection] -> "balance: $150"
  Event Log -> [Audit Projection] -> "3 transactions, last: deposit $100"
  Event Log -> [Analytics Projection] -> "avg deposit: $100, avg withdraw: $50"
```

**Snapshots for Performance**:
```
Problem: replaying 1M events for current state is slow

Solution: periodically snapshot the state:
  Snapshot at event #999: {balance: $5000}
  
  To get current state:
    Load snapshot ($5000)
    Replay only events 1000-1050 (last 50 events)
    Much faster than replaying all 1050 events!
```

### CQRS (Command Query Responsibility Segregation)

Separate the write model from the read model:

```
+-------+     +----------------+     +-----------+
| Client|---->| Command Side   |---->| Write DB  |
| Write |     | (validates,    |     | (normalized|
+-------+     |  business rules)|    |  optimized |
              +----------------+     |  for writes)|
                                     +-----------+
                                          |
                                     [Event/CDC]
                                          |
                                          v
+-------+     +----------------+     +-----------+
| Client|---->| Query Side     |---->| Read DB   |
| Read  |     | (simple lookups)|    | (denormal- |
+-------+     +----------------+     |  ized,     |
                                     |  optimized |
                                     |  for reads)|
                                     +-----------+

Benefits:
- Write model: normalized, ACID, optimized for updates
- Read model: denormalized, eventually consistent, optimized for queries
- Scale independently (more read replicas without affecting writes)
- Each side uses ideal storage (write: PostgreSQL, read: Elasticsearch)
```

### Saga Pattern (Orchestration vs Choreography)

**Orchestration Saga** (central coordinator):
```
Saga Orchestrator     Order Service    Payment Service    Inventory Service
       |                    |                |                   |
       |--Create Order----->|                |                   |
       |<---Order Created---|                |                   |
       |                    |                |                   |
       |--Process Payment---|--------------->|                   |
       |<---Payment OK------|----------------|                   |
       |                    |                |                   |
       |--Reserve Inventory-|----------------|------------------>|
       |<---Reserved--------|----------------|-------------------|
       |                    |                |                   |
       |--Complete Order--->|                |                   |
       |                    |                |                   |
       
ON FAILURE (payment fails):
       |--Process Payment---|--------------->|                   |
       |<---Payment FAILED--|----------------|                   |
       |                    |                |                   |
       |--Cancel Order----->|  (compensating action)             |
       |                    |                |                   |
```

**Choreography Saga** (event-driven, no coordinator):
```
Order Service       Event Bus        Payment Service      Inventory Service
     |                 |                   |                    |
     |--OrderCreated-->|                   |                    |
     |                 |--OrderCreated---->|                    |
     |                 |                   |                    |
     |                 |<--PaymentDone-----|                    |
     |                 |---PaymentDone---->|                    |
     |                 |                   |--InventoryReserved-->|
     |                 |<--InventoryReserved|                   |
     |<--OrderCompleted|                   |                    |
     
ON FAILURE:
     |                 |<--PaymentFailed---|                    |
     |<--PaymentFailed-|                   |                    |
     |--OrderCancelled>|                   |                    |
```

| Aspect | Orchestration | Choreography |
|--------|--------------|--------------|
| Coupling | Centralized (orchestrator knows all steps) | Decentralized (each service reacts) |
| Visibility | Easy to trace (single flow) | Hard to trace (events scattered) |
| Complexity | Orchestrator can become monolith | More services, more event handlers |
| Failure handling | Explicit compensations in orchestrator | Each service handles its own compensation |

### Transactional Outbox Pattern

Problem: "Dual write" - how to atomically update DB AND publish event?

```
WRONG approach (race condition):
  1. Write to DB   <- succeeds
  2. Publish event <- FAILS (broker down)
  Result: DB updated but no event published (inconsistency)

TRANSACTIONAL OUTBOX:
  1. In SAME database transaction:
     - Write business data to orders table
     - Write event to outbox table
  2. Separate process reads outbox, publishes to broker:

  +---Database Transaction---+
  | INSERT INTO orders (...) |
  | INSERT INTO outbox       |
  |   (event_type, payload,  |
  |    published=false)      |
  +--------------------------+

  Publisher process (polling or CDC):
    SELECT * FROM outbox WHERE published=false
    -> publish to Kafka
    -> UPDATE outbox SET published=true
```

**CDC (Change Data Capture) approach**:
```
Instead of polling outbox table, use database's transaction log:

  Database WAL/binlog -> [Debezium CDC Connector] -> Kafka

  - Captures inserts to outbox table from the WAL
  - Streams changes to Kafka in real-time
  - No polling overhead, very low latency
  - Debezium handles offset tracking (exactly-once semantics)
```

---

## Backpressure Mechanisms

When producers are faster than consumers:

```
1. BOUNDED QUEUE + DROP:
   Queue has max size. When full, new messages dropped or rejected.
   Producer must handle rejection (retry, dead-letter).
   
2. BOUNDED QUEUE + BLOCK:
   When full, producer blocks until space available.
   Propagates pressure upstream (producer slows down).
   
3. RATE LIMITING:
   Consumer tells producer its maximum rate.
   Producer rate-limits itself (cooperative).
   
4. CREDIT-BASED FLOW CONTROL (RabbitMQ):
   Consumer grants N "credits" to broker.
   Broker sends N messages, waits for more credits.
   Consumer processes, grants more credits when ready.
   
5. CONSUMER LAG MONITORING:
   Track consumer offset vs latest offset (lag).
   If lag grows: add more consumers, alert, auto-scale.
   
   Kafka consumer lag:
     latest_offset - consumer_committed_offset = lag
     lag > threshold -> alert/scale
```

---

## Files
- [pubsub.cpp](pubsub.cpp) - in-memory pub/sub
- [worker_queue.cpp](worker_queue.cpp) - producer/consumer threads
- [dlq_with_retry.cpp](dlq_with_retry.cpp) - retry + DLQ

## Interview Questions
1. Kafka vs RabbitMQ - when to choose which? Compare their models.
2. How does Kafka achieve high throughput? (sequential I/O, zero-copy, batching, partitions)
3. What is a partition in Kafka? How does it affect ordering guarantees?
4. At-least-once vs exactly-once - implementation cost and trade-offs?
5. Transactional outbox pattern - what problem does it solve? Compare polling vs CDC.
6. Backpressure strategies in async systems - how to handle slow consumers?
7. Why is idempotency critical in event-driven systems? How do you implement it?
8. DLQ design - retry policy, alerting, manual replay strategies?
9. Saga pattern (orchestration vs choreography) - trade-offs and when to use each.
10. Explain Kafka's exactly-once semantics - how do idempotent producers + transactions work?
11. What is event sourcing? How do snapshots prevent unbounded replay?
12. Explain CQRS - why separate read/write models? What is the consistency trade-off?
13. How does Kafka's ISR mechanism prevent data loss during leader election?
14. What is zero-copy and why does Kafka use sendfile()?
15. Explain the Two Generals Problem and why true exactly-once is impossible.

## Daily Assignment
1. Build an in-memory `PubSub` with `subscribe(topic, handler)` and `publish(topic, msg)`. Support multiple subscribers per topic.
2. Implement a thread-safe bounded blocking queue with one producer, N consumers. Add consumer group semantics (each message consumed by exactly one consumer in group).
3. Add retry with exponential backoff + DLQ when retries exhausted.
4. Implement a simplified transactional outbox: write to "DB" + "outbox" atomically, separate publisher thread reads outbox and publishes.
5. Bonus: Implement an event sourcing store - append events, replay to compute current state, add periodic snapshots.
