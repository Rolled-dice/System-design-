# Day 26 - HLD: URL Shortener (TinyURL / bit.ly)

## Functional Requirements
1. `shorten(longUrl) -> shortUrl`
2. `expand(shortUrl) -> longUrl` (302 redirect)
3. (Optional) custom alias, expiry, analytics

## Non-Functional
- 100M new URLs/month -> ~40 writes/sec, peak ~400/sec
- 10B reads/month -> read:write = ~100:1, **read-heavy**
- Latency: redirect < 100ms p99
- High availability > 99.9%
- 5-year retention

## Capacity Estimation
- 100M/mo * 12 * 5 = **6B URLs total** in 5 years
- Avg URL: 2KB long URL + 100B metadata -> ~2.1KB/row
- Total storage: 6B * 2.1KB ~ **12.6 TB**
- Read QPS: 10B / (30 * 86400) ~ **3.8K avg, 38K peak**
- Cache: hot 20% -> **2.5 TB** working set... cache only top 1% = **~120GB** in Redis

## Short Code Generation Approaches

### A. Hash-based (MD5/SHA-256 + base62, take first 7 chars)
- Pros: stateless, deterministic
- Cons: collisions possible -> need to handle with retry / suffix

### B. Base62 of monotonic counter (preferred)
- 62^7 = 3.5 trillion possible codes - ample for 5 years
- Counter from a distributed sequence (ZooKeeper, ticket server, sharded DB)
- No collisions, predictable, sequential -> mitigate with shuffling/encoding

### C. Pre-generated key pool
- Background workers fill a Redis pool of unused keys
- Service pops keys atomically -> O(1) writes

## Architecture

```
[Client] -> [DNS] -> [LB] -> [API Gateway]
                                |
              +-----------------+------------------+
              |                 |                  |
        [Write Service]   [Read Service]   [Analytics]
              |                 |                  |
        [Key Generator]   [Redis Cache]      [Kafka]
              |                 |
              +-----> [Sharded DB (PostgreSQL / DynamoDB)]
                                |
                          [CDN for hot redirects]
```

## Database Schema (SQL)
```sql
url(short_code PK, long_url, user_id, created_at, expires_at);
INDEX on user_id;
```
Sharding key: `short_code` hash -> partition.

## Read Path
1. Client GETs `bit.ly/abc123`
2. Edge / API Gateway -> Read Service
3. Check Redis (LRU); if hit, return 302
4. If miss, query DB -> populate cache
5. Async: log analytics event to Kafka

## Write Path
1. Client POSTs long URL
2. Validate, dedupe (optional - check by hash of long URL)
3. Generate short code via counter -> base62
4. Insert in DB, write to cache
5. Return short URL

## Files
- [base62_encoder.cpp](base62_encoder.cpp)
- [url_shortener_service.cpp](url_shortener_service.cpp) - in-memory simulation

## Interview Questions
1. Why base62 (not base64)? URL safety - no `/`, `+`, `=`.
2. How to avoid collisions? counter approach guarantees uniqueness.
3. How to handle custom aliases? Reserve namespace, check uniqueness, fallback to generated.
4. How to handle expired URLs? Lazy expiry on read or background sweeper.
5. How to scale reads to 100k QPS? CDN + Redis + read replicas.
6. How to do analytics without slowing read path? Kafka -> stream processor -> warehouse.
7. How to prevent abuse? Rate limit per user/IP, captchas, blacklist domains.
8. Why 7 chars? 62^7 = 3.5T, enough headroom; 6 chars (56B) marginal.
9. What if you must shorten the same long URL to same short code? Hash dedup table.
10. Disaster recovery - what if counter service goes down? Multi-AZ, cached batches.

## Daily Assignment
1. Implement base62 encoder/decoder.
2. Build in-memory shortener with counter; add custom alias support.
3. Add Redis-style cache layer in front of DB.
4. Sketch a sequence diagram for write flow.
