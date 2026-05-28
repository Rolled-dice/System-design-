# Day 19 - Messaging Queues + Event-Driven Architecture

## Why Async Messaging?
- Decouple producers and consumers
- Handle bursts via buffering
- Enable retry and durability
- Fan-out (one event -> many consumers)

## Key Brokers
| Broker | Model | Strengths |
|--------|-------|-----------|
| **Kafka** | Distributed log, partitioned, persistent | High throughput, replay, stream processing |
| **RabbitMQ** | Smart broker / dumb consumer (AMQP) | Routing, priority, RPC |
| **AWS SQS** | Managed queue | Simplicity, FIFO option |
| **Redis Streams** | Lightweight log | Low-latency, simple |
| **NATS** | Pub/Sub, JetStream for persistence | Microservice messaging, low overhead |

## Messaging Patterns
- **Point-to-Point**: queue, single consumer wins
- **Publish-Subscribe**: topic, all subscribers receive
- **Request-Reply**: temporary reply queue (RPC over messaging)
- **Competing Consumers**: scale a queue with worker pool
- **Fan-Out**: one event broadcast to many systems

## Delivery Guarantees
- **At-most-once** - fire and forget; lossy
- **At-least-once** - retry on failure; duplicates possible -> idempotent consumers
- **Exactly-once** - hard; needs transactional outbox or Kafka exactly-once semantics

## Common Concerns
- **Dead Letter Queue (DLQ)** - park failed messages after N retries
- **Idempotency** - dedupe via message id / business key
- **Backpressure** - slow consumer should slow producer or shed load
- **Ordering** - per-partition (Kafka) or global (single queue)
- **Poison messages** - forward to DLQ, alert

## Files
- [pubsub.cpp](pubsub.cpp) - in-memory pub/sub
- [worker_queue.cpp](worker_queue.cpp) - producer/consumer threads
- [dlq_with_retry.cpp](dlq_with_retry.cpp) - retry + DLQ

## Interview Questions
1. Kafka vs RabbitMQ - when to choose which?
2. How does Kafka achieve high throughput? (sequential disk writes, zero-copy, batching)
3. What is a partition in Kafka? How does it affect ordering?
4. At-least-once vs exactly-once - implementation cost?
5. Transactional outbox pattern - what problem does it solve?
6. Backpressure strategies in async systems.
7. Why is idempotency critical in event-driven systems?
8. DLQ design - retry policy, alerting, manual replay?
9. Saga pattern (orchestration vs choreography).

## Daily Assignment
1. Build an in-memory `PubSub` with `subscribe(topic, handler)` and `publish(topic, msg)`.
2. Implement a thread-safe bounded blocking queue with one producer, N consumers.
3. Add retry with exponential backoff + DLQ when retries exhausted.
