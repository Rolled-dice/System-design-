# Day 20 - Rate Limiting

## Table of Contents
- [Why Rate Limit](#why-rate-limit)
- [Token Bucket Algorithm](#token-bucket-algorithm)
- [Leaky Bucket Algorithm](#leaky-bucket-algorithm)
- [Fixed Window Counter](#fixed-window-counter)
- [Sliding Window Log](#sliding-window-log)
- [Sliding Window Counter](#sliding-window-counter)
- [Distributed Rate Limiting](#distributed-rate-limiting)
- [Practical Considerations](#practical-considerations)
- [HTTP 429 Design](#http-429-design)
- [Files](#files)
- [Interview Questions](#interview-questions)
- [Daily Assignment](#daily-assignment)

---

## Why Rate Limit

Rate limiting controls the rate of incoming requests to protect systems from overload:

| Purpose | Example |
|---------|---------|
| DDoS protection | Block IPs sending >10K req/s |
| Fair resource sharing | Each tenant gets 1000 req/min |
| Cost control | 3rd-party API charges per call |
| Prevent cascading failures | Shed load before backend saturates |
| Compliance | API SLA guarantees specific limits |

### Where to Place Rate Limiters

```
Internet -> [CDN/WAF (L3/L4)] -> [API Gateway (L7)] -> [Service Mesh] -> [Application]
              |                      |                      |                 |
         IP-based               Per-API-key            Per-service       Business logic
         geo-blocking           per-user               rate limits       limits
         basic DDoS             endpoint limits        circuit breaker   custom rules
```

---

## Token Bucket Algorithm

### Formal Model

```
Parameters:
  B = bucket capacity (maximum tokens / max burst size)
  R = refill rate (tokens added per second)

State:
  tokens = current number of tokens in bucket (0 <= tokens <= B)
  last_refill_time = timestamp of last refill

Algorithm:
  on_request():
    // Lazy refill: calculate tokens accumulated since last check
    now = current_time()
    elapsed = now - last_refill_time
    tokens = min(B, tokens + elapsed * R)
    last_refill_time = now
    
    if tokens >= 1:
      tokens -= 1
      return ALLOW
    else:
      return DENY
```

### Mathematical Analysis

```
Burst Capacity:
  Maximum burst = B tokens can be consumed instantly
  (bucket can be full, all consumed in one burst)

Average Rate:
  Over long period, average allowed rate = R requests/second
  Proof: bucket refills at rate R. Even if emptied by burst,
  subsequent requests limited to R/s until bucket refills.

Refill Formula:
  tokens_after_time_t = min(B, tokens_current + R * t)
  
  Time to refill from 0 to full: t_full = B / R

Example:
  B = 100 tokens, R = 10 tokens/second
  - Burst: can send 100 requests instantly
  - Sustained: 10 requests/second
  - After burst, wait B/R = 10 seconds to fully refill
  - After burst, next request allowed in 1/R = 0.1 seconds
```

### Token Bucket Visualization

```
Bucket (capacity B=5):

Time 0:  [*][*][*][*][*]  tokens=5 (full)
         Request -> ALLOW, tokens=4

Time 1:  [*][*][*][*][ ]  tokens=4 + R*dt refill
         4 rapid requests -> ALLOW x4, tokens=0 + refill

Time 2:  [ ][ ][ ][ ][ ]  tokens=0 (empty)
         Request -> DENY (no tokens!)

Time 3:  [*][ ][ ][ ][ ]  tokens=1 (refilled 1 token)
         Request -> ALLOW, tokens=0

Refill: +R tokens per second, capped at B
```

**Use cases**: API rate limiting (allows bursts for interactive usage), AWS API Gateway, network traffic shaping.

---

## Leaky Bucket Algorithm

### Formal Model

```
Parameters:
  B = bucket capacity (queue size)
  R = leak rate (constant outflow rate, requests processed per second)

State:
  water = current queue level (pending requests)

Algorithm:
  on_request():
    // Drain leaked amount since last check
    now = current_time()
    elapsed = now - last_leak_time
    water = max(0, water - elapsed * R)
    last_leak_time = now
    
    if water < B:
      water += 1
      return ALLOW  (request queued, processed at rate R)
    else:
      return DENY   (bucket overflow, request dropped)
```

### Queue Theory Connection

```
Leaky Bucket = M/D/1 Queue (Markov arrivals, Deterministic service, 1 server)
  - Input: bursty traffic (variable arrival rate)
  - Output: constant rate R (deterministic service)
  - Buffer: size B (finite queue)

  Input (bursty):     ||| || ||||  |   || |||
  Output (constant):  | | | | | | | | | | | |
                      
  Traffic shaping: converts bursty input to smooth output
```

### Token Bucket vs Leaky Bucket

| Property | Token Bucket | Leaky Bucket |
|----------|-------------|--------------|
| Burst handling | Allows bursts (up to B) | Smooths bursts to constant rate |
| Output rate | Variable (can spike) | Fixed at R |
| Behavior when full | Rejects new requests | Rejects new requests |
| Use case | API limiting (user-friendly bursts) | Traffic shaping (network QoS) |
| Think of it as | "Spending budget" | "Water draining from bucket" |

---

## Fixed Window Counter

### Formal Model

```
Parameters:
  W = window size (e.g., 60 seconds)
  L = limit (max requests per window)

State:
  counter = requests in current window
  window_start = start of current window

Algorithm:
  on_request():
    now = current_time()
    current_window = floor(now / W) * W
    
    if current_window != window_start:
      counter = 0
      window_start = current_window
    
    if counter < L:
      counter += 1
      return ALLOW
    else:
      return DENY
```

### Edge Burst Problem (2x Burst at Boundary)

```
Window size: 1 minute, Limit: 100 requests/minute

Timeline:
  |-------- Window 1 --------|-------- Window 2 --------|
  |                           |                          |
  0:00                      1:00                       2:00

Attack pattern:
  - Send 0 requests from 0:00 to 0:30
  - Send 100 requests from 0:30 to 1:00 (allowed: counter=100, within limit)
  - Window resets at 1:00, counter=0
  - Send 100 requests from 1:00 to 1:30 (allowed: counter=100, within limit)

Result: 200 requests in 60 seconds (0:30 to 1:30)!
  That is 2x the intended limit of 100/minute!

Timeline visualization:
  |.......[100 req]|[100 req].......|
  0:00   0:30    1:00    1:30     2:00
         <--- 200 requests in 1 minute! --->
```

**Mitigation**: Use sliding window (log or counter) to prevent boundary exploitation.

---

## Sliding Window Log

### Formal Model

```
Parameters:
  W = window size (e.g., 60 seconds)
  L = limit (max requests per window)

State:
  log = sorted list of timestamps (of allowed requests)

Algorithm:
  on_request():
    now = current_time()
    
    // Remove timestamps older than window
    remove_all(log, where timestamp < now - W)
    
    if len(log) < L:
      log.append(now)
      return ALLOW
    else:
      return DENY
```

### Space Complexity Analysis

```
Space: O(L) per user (must store all timestamps in window)

Example: limit = 10,000 requests/minute
  - Each timestamp: 8 bytes
  - Per user: 10,000 * 8 = 80KB
  - 1M users: 80GB of memory!

This is why sliding window LOG is expensive for high limits.
Sliding window COUNTER is the practical solution.

Time complexity:
  - Remove expired: O(K) where K = expired entries (amortized O(1) with sorted structure)
  - Count: O(1) if we maintain running count
  - Overall: O(1) amortized per request
```

---

## Sliding Window Counter

### Formal Model (Weighted Formula)

```
Parameters:
  W = window size (e.g., 60 seconds)
  L = limit

State:
  prev_count = request count in previous window
  curr_count = request count in current window
  window_start = start of current window

Algorithm:
  on_request():
    now = current_time()
    current_window = floor(now / W) * W
    
    if current_window != window_start:
      prev_count = curr_count
      curr_count = 0
      window_start = current_window
    
    // Calculate position within current window
    elapsed_ratio = (now - current_window) / W   // 0.0 to 1.0
    
    // Weighted estimate of requests in sliding window
    overlap_percentage = 1.0 - elapsed_ratio
    estimated_count = prev_count * overlap_percentage + curr_count
    
    if estimated_count < L:
      curr_count += 1
      return ALLOW
    else:
      return DENY
```

### Worked Example

```
Window = 60s, Limit = 100

Previous window (0:00-1:00): prev_count = 84 requests
Current window  (1:00-2:00): curr_count = 36 requests
Current time: 1:15 (15 seconds into current window)

elapsed_ratio = 15 / 60 = 0.25
overlap_percentage = 1.0 - 0.25 = 0.75 (75% of prev window still in sliding view)

estimated_count = 84 * 0.75 + 36 = 63 + 36 = 99

99 < 100 -> ALLOW (barely!)
Next request: 100 -> DENY
```

### Error Bound Analysis

```
The sliding window counter is an APPROXIMATION.
Assumes requests in previous window were uniformly distributed.

Worst case error:
  If all prev_window requests happened at the END of prev window,
  the weighted formula UNDERestimates (allows slightly more than limit).
  
  If all prev_window requests happened at the START of prev window,
  the weighted formula OVERestimates (blocks too aggressively).

Error bound: at most L * (1 - elapsed_ratio) requests over-counted or under-counted
  Typically within 1-5% of exact count.

Trade-off: O(1) space per user (just 2 counters) vs O(L) for exact sliding log
  99% accuracy for 99.99% less memory. Excellent trade-off.
```

---

## Distributed Rate Limiting

### Redis Lua Script Approach (Atomic Operations)

```lua
-- Sliding window counter in Redis (atomic Lua script)
local key = KEYS[1]
local window = tonumber(ARGV[1])  -- window size in seconds
local limit = tonumber(ARGV[2])
local now = tonumber(ARGV[3])

local window_start = math.floor(now / window) * window
local prev_key = key .. ":" .. (window_start - window)
local curr_key = key .. ":" .. window_start

local prev_count = tonumber(redis.call("GET", prev_key) or "0")
local curr_count = tonumber(redis.call("GET", curr_key) or "0")

local elapsed_ratio = (now - window_start) / window
local estimated = prev_count * (1 - elapsed_ratio) + curr_count

if estimated < limit then
    redis.call("INCR", curr_key)
    redis.call("EXPIRE", curr_key, window * 2)
    return 1  -- ALLOWED
else
    return 0  -- DENIED
end
```

**Why Lua script?** Redis executes Lua atomically (no interleaving with other commands). Prevents race conditions between checking count and incrementing.

### Race Condition Analysis (Without Lua)

```
WITHOUT atomic operation (race condition):

Thread A:                      Thread B:
  GET counter -> 99              GET counter -> 99
  99 < 100? YES                  99 < 100? YES
  INCR counter -> 100            INCR counter -> 101  <- OVER LIMIT!

Both threads allowed because they both read 99 before either incremented.

WITH Lua script: entire check-and-increment is atomic, no interleaving possible.
```

### Sliding Window in Redis (Sorted Sets)

```
Alternative to counter approach: use sorted sets with timestamps

ZADD user:123:requests <timestamp> <unique_id>
ZREMRANGEBYSCORE user:123:requests 0 <now - window>
ZCARD user:123:requests -> count

Atomic pipeline:
  MULTI
    ZREMRANGEBYSCORE user:123:requests 0 (now - 60)
    ZADD user:123:requests now uuid
    ZCARD user:123:requests
  EXEC
  
  if count > limit: ZREM user:123:requests uuid  // rollback

Pros: exact sliding window (no approximation)
Cons: O(L) memory per user (stores all request timestamps)
```

### Cell-Based Rate Limiting (Token Bucket in Redis)

```
Generic Cell Rate Algorithm (GCRA) - single Redis key per user:

Key: rate_limit:user:123
Value: TAT (Theoretical Arrival Time) - next time a request is allowed

Algorithm:
  now = current_time()
  tat = GET key (or now if not exists)
  
  new_tat = max(tat, now) + (1/R)  // R = rate limit
  
  allow_at = new_tat - burst_offset
  
  if allow_at <= now:
    SET key new_tat EXPIRE ...
    return ALLOW
  else:
    return DENY (retry after: allow_at - now)

Advantage: O(1) space per user (single timestamp), no Lua needed for basic version
```

---

## Practical Considerations

### API Gateway Rate Limiting

```
                      +-------------------+
Internet -> [CDN] -> | API Gateway       |
                     | Rate Limit Rules:  |
                     | - Global: 10K/s    |
                     | - Per-IP: 100/min  |
                     | - Per-API-Key: var |
                     | - Per-Endpoint:    |
                     |   POST /api: 10/s  |
                     |   GET /api: 100/s  |
                     +-------------------+
                              |
                     +--------+--------+
                     |        |        |
                  [Svc A]  [Svc B]  [Svc C]
```

### Per-Tenant Fairness

```
Multi-tenant system with shared resources:

PROBLEM: one noisy tenant can exhaust shared resources

SOLUTION: Hierarchical rate limits

  Global limit: 100,000 requests/second (system capacity)
  Per-tenant limit: 10,000 requests/second (fair share)
  Per-user limit: 100 requests/minute (abuse prevention)
  Per-endpoint limit: varies by endpoint cost

  Check order: per-user -> per-tenant -> global -> per-endpoint
  (fail fast at most specific level)
```

### Adaptive/Dynamic Rate Limits

```
Static limits are fragile:
  - Set too low: reject legitimate traffic
  - Set too high: don't protect backend

ADAPTIVE approach:
  Monitor backend health (CPU, latency, error rate)
  Adjust limits dynamically:
  
  if backend_cpu > 80%:
    current_limit = current_limit * 0.8  // reduce by 20%
  elif backend_cpu < 40% AND current_limit < max_limit:
    current_limit = current_limit * 1.1  // increase by 10%
    
  AIMD pattern (like TCP congestion control):
    Additive Increase: slowly raise limit when healthy
    Multiplicative Decrease: rapidly drop limit on overload
```

### Hierarchical Rate Limits

```
Level 1 (Global):   System can handle 100K req/s total
Level 2 (Service):  Each service gets proportional share
Level 3 (Tenant):   Each tenant gets SLA-defined share
Level 4 (User):     Each user within tenant gets fair share
Level 5 (Endpoint): Different limits per API endpoint

+---------------------------------------------------+
|           Global: 100,000 req/s                    |
+---------------------------------------------------+
|  Service A: 40K  |  Service B: 35K  | Service C: 25K |
+---------------------------------------------------+
| Tenant 1: 5K | Tenant 2: 3K | Tenant 3: 2K | ... |
+---------------------------------------------------+
| User 1: 100/min | User 2: 100/min | ...           |
+---------------------------------------------------+
```

---

## HTTP 429 Design

### Response Headers

```
HTTP/1.1 429 Too Many Requests
Content-Type: application/json
Retry-After: 30
X-RateLimit-Limit: 100
X-RateLimit-Remaining: 0
X-RateLimit-Reset: 1672531260

{
  "error": "rate_limit_exceeded",
  "message": "Too many requests. Please retry after 30 seconds.",
  "limit": 100,
  "window": "1 minute",
  "retry_after": 30
}
```

**Headers explained**:
| Header | Purpose |
|--------|---------|
| `Retry-After` | Seconds until client can retry (or HTTP date) |
| `X-RateLimit-Limit` | Maximum requests allowed in window |
| `X-RateLimit-Remaining` | Requests remaining in current window |
| `X-RateLimit-Reset` | Unix timestamp when window resets |

### Client Backoff (Exponential with Jitter)

```
Formula:
  sleep = min(cap, base * 2^attempt) + random_jitter

Parameters:
  base = 1 second (initial backoff)
  cap = 60 seconds (maximum backoff)
  attempt = retry attempt number (0, 1, 2, ...)

Example progression:
  Attempt 0: min(60, 1 * 2^0) + jitter = 1s + jitter
  Attempt 1: min(60, 1 * 2^1) + jitter = 2s + jitter
  Attempt 2: min(60, 1 * 2^2) + jitter = 4s + jitter
  Attempt 3: min(60, 1 * 2^3) + jitter = 8s + jitter
  Attempt 4: min(60, 1 * 2^4) + jitter = 16s + jitter
  Attempt 5: min(60, 1 * 2^5) + jitter = 32s + jitter
  Attempt 6: min(60, 1 * 2^6) + jitter = 60s + jitter (capped)

WHY JITTER?
  Without jitter: all clients retry at SAME time (thundering herd)
  With jitter: retries spread out, reduces load spike

  Full jitter:   sleep = random(0, min(cap, base * 2^attempt))
  Equal jitter:  half = min(cap, base * 2^attempt) / 2
                 sleep = half + random(0, half)
  Decorrelated:  sleep = random(base, previous_sleep * 3)
```

### Fail-Open vs Fail-Closed

```
What happens if rate limiter itself is down (Redis failure)?

FAIL-OPEN (allow all traffic):
  if rate_limiter.is_healthy():
    return rate_limiter.check(request)
  else:
    return ALLOW  // let all traffic through
  
  Pros: no customer impact from rate limiter failures
  Cons: temporarily unprotected (risk of backend overload)

FAIL-CLOSED (deny all traffic):
  if rate_limiter.is_healthy():
    return rate_limiter.check(request)
  else:
    return DENY  // reject all traffic
  
  Pros: backend always protected
  Cons: complete outage if rate limiter fails

Best practice: FAIL-OPEN with monitoring + alerting
  Rate limiter down is rare and temporary.
  Backend overload from burst is less damaging than total outage.
```

---

## Files
- [token_bucket.cpp](token_bucket.cpp)
- [leaky_bucket.cpp](leaky_bucket.cpp)
- [fixed_window.cpp](fixed_window.cpp)
- [sliding_window.cpp](sliding_window.cpp)

## Interview Questions
1. Token bucket vs leaky bucket - when to use each? What is the output behavior difference?
2. How would you build a distributed rate limiter using Redis? Why use Lua scripts?
3. Fixed window edge bursting - explain with numerical example showing 2x burst.
4. Per-user vs per-IP rate limiting - which is better and when?
5. How to communicate rate-limit info back to clients? Design the response.
6. Should rate limiter fail open or closed if Redis is down? Justify your choice.
7. What is the cost of sliding window log? How does sliding window counter reduce it?
8. Explain the sliding window counter's weighted formula. What is the error bound?
9. How would you implement adaptive rate limiting? What signals would you use?
10. Token bucket: if B=50 and R=10/s, what is max burst? What is sustained rate?
11. Design a hierarchical rate limiter: global -> per-tenant -> per-user. How do you coordinate?
12. Explain exponential backoff with jitter. Why is jitter critical for distributed systems?
13. How does the GCRA (Generic Cell Rate Algorithm) work? What are its advantages?
14. Rate limiting at L4 vs L7 - what are the differences in what you can see/control?
15. How would you handle rate limiting for WebSocket connections vs REST APIs?

## Daily Assignment
1. Implement Token Bucket and benchmark allowed/denied at different request rates. Verify average rate converges to R.
2. Implement Leaky Bucket using a queue + scheduled drain. Compare output smoothness vs token bucket.
3. Build a per-user rate limiter: max 100 req/min per user, return false on exceeded. Use sliding window counter.
4. Implement the fixed window edge burst exploit: demonstrate how to get 2x the limit.
5. Bonus: design distributed rate limiter using Redis Lua script. Handle the race condition case.
6. Advanced: implement adaptive rate limiting that reduces limits when backend latency increases.
