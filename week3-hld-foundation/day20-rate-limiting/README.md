# Day 20 - Rate Limiting

## Why Rate Limit?
- Protect from abuse / DDoS
- Fair resource sharing across tenants
- Cost control (3rd-party APIs)
- Avoid cascading failures

## Algorithms

### 1. Fixed Window Counter
Count requests per fixed window (e.g., per minute). **Simple, but bursty at window edges.**

### 2. Sliding Window Log
Store timestamps of every request, count those within window. **Accurate, but memory expensive.**

### 3. Sliding Window Counter
Hybrid: weighted average of current and previous window. **Memory efficient, ~accurate.**

### 4. Token Bucket
Bucket of capacity B, refilled at rate R tokens/sec. Each request consumes 1 token. **Allows bursts up to B.**

### 5. Leaky Bucket
Queue with constant outflow rate. Excess requests drop or wait. **Smooths bursts to constant rate.**

## Where to Place Rate Limiter
- API gateway (per IP, per user, per API key)
- Per-service (defense in depth)
- Distributed: store counters in Redis with atomic increment

## Files
- [token_bucket.cpp](token_bucket.cpp)
- [leaky_bucket.cpp](leaky_bucket.cpp)
- [fixed_window.cpp](fixed_window.cpp)
- [sliding_window.cpp](sliding_window.cpp)

## Interview Questions
1. Token bucket vs leaky bucket - when to use each?
2. How would you build a distributed rate limiter using Redis?
3. Fixed window edge bursting - explain with example.
4. Per-user vs per-IP rate limiting - which is better and when?
5. How to communicate rate-limit info back to clients? (`X-RateLimit-*` headers, 429)
6. Should rate limiter fail open or closed if Redis is down?
7. What is the cost of sliding window log?

## Daily Assignment
1. Implement Token Bucket and benchmark allowed/denied at different rates.
2. Implement Leaky Bucket using a queue + scheduled drain.
3. Build a per-user rate limiter: max 100 req/min per user, return false on exceeded.
4. Bonus: design distributed rate limiter using Lua script in Redis.
