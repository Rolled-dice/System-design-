# HLD Interview Question Bank

## The 6-Step HLD Framework

1. **Requirements** (5 min) - functional, non-functional, out-of-scope
2. **Capacity estimation** (5 min) - QPS, storage, bandwidth
3. **API design** (5 min) - REST/gRPC endpoints + signatures
4. **High-level diagram** (10 min) - clients, LB, services, DBs, queues, cache, CDN
5. **Deep dive** (20 min) - 1-2 components: data model, algorithms, trade-offs
6. **Bottlenecks + scale + reliability** (5 min) - sharding, caching, CDN, failover

---

## Tier 1 - Must Practice (Asked Almost Everywhere)

| # | Problem | Key Concepts |
|---|---------|--------------|
| 1 | URL Shortener (TinyURL/bit.ly) | Base62, sharding, cache, CDN, key generation |
| 2 | Twitter / Threads | Timeline fanout (push/pull/hybrid), Snowflake ID, search |
| 3 | Instagram | Media storage, feed, news ranking, CDN |
| 4 | WhatsApp / Messenger | WebSocket scale, presence, E2E, group fanout |
| 5 | Uber / Lyft | Geo-spatial index, matching, real-time location ingest |
| 6 | YouTube / Netflix | Video upload, transcoding, adaptive bitrate, CDN |
| 7 | Dropbox / Google Drive | Chunking, dedup (content hash), sync, deltas |
| 8 | Web Crawler | Frontier (queue), URL dedup, politeness, robots.txt |
| 9 | Search Autocomplete | Trie, top-K per prefix, cache, async update |
| 10 | Rate Limiter | Token/leaky bucket, distributed counters in Redis |

## Tier 2

| # | Problem | Key Concepts |
|---|---------|--------------|
| 11 | News Feed (Facebook) | Fanout, ranking, feed cache |
| 12 | Distributed Cache (Memcached/Redis-like) | Consistent hashing, replication, eviction |
| 13 | Distributed File System (HDFS/GFS) | Block storage, replication factor, NameNode |
| 14 | Notification System | Channels, retry, DLQ, dedup |
| 15 | Online Code Compiler / Judge | Sandboxing, queue, resource limits |
| 16 | Pastebin | Like URL shortener + content storage |
| 17 | Distributed Job Scheduler | Cron, priority queue, worker pool, leader election |
| 18 | Stock Exchange / Trading Engine | Matching engine, order book, low-latency, FIX protocol |
| 19 | Live Streaming (Twitch) | Ingest, transcoding, low-latency CDN, chat |
| 20 | E-commerce (Amazon) | Catalog, cart, inventory, checkout, search |

## Tier 3

| # | Problem |
|---|---------|
| 21 | Distributed Counter / Analytics |
| 22 | Web Analytics (Google Analytics) |
| 23 | Ride-pooling (UberPOOL) |
| 24 | Calendar / Booking System |
| 25 | Doordash / Food Delivery |
| 26 | Slack / Discord |
| 27 | Zoom / Google Meet (video calls) |
| 28 | Distributed Tracing (Jaeger) |
| 29 | Distributed Lock Manager |
| 30 | Email System (Gmail) |

---

## Capacity Cheat Sheet

| Metric | Order of Magnitude |
|--------|--------------------|
| Daily active users | 1B big tech, 100M unicorn, 10M startup |
| Peak QPS = avg * 3-5 | seconds_in_day = 86,400 |
| 1 KB row * 1M rows | = 1 GB |
| 1 MB media * 1M | = 1 TB |
| Network: 1 Gbps | = 125 MB/s |
| SSD seq read | ~500 MB/s |
| SSD random IOPS | ~50k-100k |
| Memory access | ~100 ns |
| Network round trip same DC | ~0.5 ms |
| Network round trip cross-region | 50-150 ms |

## Latency Numbers Every Programmer Should Know

| Operation | Time |
|-----------|------|
| L1 cache | 0.5 ns |
| L2 cache | 7 ns |
| Mutex lock/unlock | 25 ns |
| Main memory | 100 ns |
| Compress 1KB w/ Zippy | 3 us |
| Send 1KB over 1 Gbps | 10 us |
| Read 4KB from SSD | 150 us |
| Read 1MB sequentially from memory | 250 us |
| Round trip same DC | 500 us |
| Read 1MB from SSD | 1 ms |
| Disk seek | 10 ms |
| Read 1MB from disk | 20 ms |
| Round trip CA -> Netherlands | 150 ms |

---

## Component Cheat Sheet

| Need | Use |
|------|-----|
| Strong consistency | Postgres, Spanner, FoundationDB, Zookeeper |
| Eventual consistency, high write | Cassandra, DynamoDB |
| Document/JSON | MongoDB, Couchbase |
| Full-text search | Elasticsearch, OpenSearch |
| Time-series | InfluxDB, TimescaleDB, Prometheus |
| Graph | Neo4j, Amazon Neptune |
| Cache | Redis, Memcached |
| Object store | S3, GCS, Azure Blob |
| CDN | Cloudflare, Fastly, CloudFront, Akamai |
| Stream / Queue | Kafka (log), RabbitMQ (smart broker), SQS, NATS |
| Stream processing | Flink, Spark Streaming, Kafka Streams |
| Coordinator | Zookeeper, etcd, Consul |
| Load balancer | NGINX, HAProxy, ALB/NLB, Envoy |
| Service mesh | Istio, Linkerd |
| API gateway | Kong, Apigee, AWS API GW |

---

## Common Trade-Offs Discussion Points

1. **SQL vs NoSQL** - schema flexibility, joins, transactions, scale
2. **Strong vs Eventual consistency** - use case impact
3. **Push vs Pull (timeline, notifications)** - latency vs cost
4. **Sync vs Async replication** - durability vs latency
5. **Long polling vs WebSocket vs SSE** - bidirectionality, scale
6. **Stateless vs Stateful services** - scale, complexity
7. **Monolith vs Microservices** - team scale, operational cost
8. **Server-side vs Client-side rendering** - SEO, latency, cost
9. **Cache invalidation strategies** - TTL vs event-driven
10. **Build vs Buy** - core competency vs commodity

---

## Reliability Patterns to Mention

- **Circuit Breaker** - stop cascading failures
- **Bulkhead** - isolate resources per dependency
- **Retry with exponential backoff + jitter**
- **Idempotency keys** for safe retries
- **Health checks** + auto-failover
- **Multi-AZ / multi-region** for DR
- **Graceful degradation** (serve stale cache, partial response)
- **Chaos engineering** to find weak spots
- **Rate limiting + load shedding**
- **Backpressure** in async pipelines
