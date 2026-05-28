# Day 26 - HLD: URL Shortener (TinyURL / bit.ly)

## HLD Interview Framework - The 4-Step Method

Before diving into the URL shortener, let us establish a systematic approach for any High-Level Design (HLD) interview. This 4-step method ensures you demonstrate breadth and depth.

### Step 1: Requirements (5 minutes)

**Functional Requirements** - What does the system DO?
- List 3-5 core features
- Prioritize: what is in scope vs out of scope for a 45-min interview
- Ask clarifying questions: "Should we support custom aliases? Analytics? Expiry?"

**Non-Functional Requirements** - How well must it perform?
- Scale: How many users? Read/write ratio?
- Latency: What is acceptable response time?
- Availability: Can we tolerate downtime? (99.9% vs 99.99%)
- Consistency: Strong vs eventual? What happens during partition?
- Durability: Can we lose data? Recovery point objective (RPO)?

**Clarifying Questions to Ask**:
- What is the expected traffic volume?
- Read-heavy or write-heavy?
- What is the retention period?
- Geographic distribution of users?
- Any compliance/regulatory requirements?

### Step 2: Capacity Estimation (5 minutes)

Master these formulas - they apply to every HLD problem:

**QPS (Queries Per Second)**:
```
Monthly Active Users (MAU) -> Daily Active Users (DAU) = MAU * 0.2 to 0.5
Daily requests = DAU * actions_per_user_per_day
QPS = daily_requests / 86,400
Peak QPS = QPS * 3 to 10 (depending on traffic pattern)
```

**Storage**:
```
Storage = total_records * avg_record_size
total_records = write_QPS * seconds_per_year * retention_years
    OR = monthly_writes * 12 * retention_years
```

**Bandwidth**:
```
Ingress = write_QPS * avg_request_size
Egress = read_QPS * avg_response_size
```

**Cache Size (80/20 rule)**:
```
Working set = daily_read_QPS * 86400 * avg_record_size * 0.2
  (20% of data serves 80% of reads)
Practical cache = top 1-5% of data that fits in memory budget
```

**Memory**:
```
If caching N records of size S: N * S bytes
Add overhead: hash table ~ 1.5x record count * (key_size + pointer_size)
```

### Step 3: High-Level Design (15 minutes)

Draw the component diagram:
- Client -> Load Balancer -> API Gateway -> Services
- Data stores (SQL, NoSQL, Cache, Object Store)
- Async processing (Message Queue -> Workers)
- CDN for static/cacheable content

Describe the data flow for primary use cases:
- Write path: step by step from client to persistence
- Read path: step by step from client to response

### Step 4: Deep-Dive (20 minutes)

Pick 2-3 components and go deep:
- Algorithm/data structure choices with trade-offs
- Failure handling and edge cases
- Scalability bottlenecks and solutions
- Monitoring and operational concerns

**Key principle**: Every design decision must answer "WHY this and not the alternative?"

---

## Problem Statement

Design a URL shortening service (like bit.ly or TinyURL) that:
- Converts long URLs to short, memorable codes
- Redirects short URLs to original long URLs
- Handles billions of reads with low latency

---

## Step 1: Requirements

### Functional Requirements
1. `shorten(longUrl) -> shortUrl` - Generate a short code for a long URL
2. `expand(shortUrl) -> longUrl` - Redirect short URL to original (HTTP 302)
3. Custom alias support (optional)
4. URL expiry (optional TTL)
5. Analytics: click count, geographic distribution, referrer

### Non-Functional Requirements
- 100M new URLs/month (~40 writes/sec avg, ~400/sec peak)
- 10B reads/month (read:write = 100:1) - extremely read-heavy
- Redirect latency: < 100ms p99
- High availability: > 99.9% (downtime < 8.7 hours/year)
- 5-year retention for URLs
- Globally distributed users

---

## Step 2: Capacity Estimation

### QPS Calculation
```
Writes: 100M/month = 100M / (30 * 86400) = ~40 writes/sec
Peak writes: 40 * 10 = ~400/sec (viral events)

Reads: 10B/month = 10B / (30 * 86400) = ~3,800 reads/sec
Peak reads: 3800 * 10 = ~38,000/sec
```

### Storage Calculation
```
Records over 5 years: 100M/month * 12 * 5 = 6 Billion URLs
Average record size:
  - short_code: 7 bytes
  - long_url: 2000 bytes (avg)
  - metadata: 100 bytes (user_id, created_at, expires_at)
  - Total: ~2.1 KB per record

Total storage: 6B * 2.1KB = ~12.6 TB
```

### Bandwidth Calculation
```
Ingress (writes): 400/sec * 2.1KB = ~840 KB/sec (negligible)
Egress (reads): 38,000/sec * 2.1KB = ~80 MB/sec (significant but manageable)
```

### Cache Sizing
```
Daily read requests: 3800/sec * 86400 = ~328M reads/day
Unique URLs accessed per day (assuming power-law): ~20% of total = ~10M unique URLs/day
Hot cache (top 20% of daily accesses): 10M * 0.2 = 2M records
Cache size: 2M * 2.1KB = ~4.2 GB (easily fits in single Redis instance)

More aggressive: top 1% of ALL URLs = 60M * 2.1KB = ~126 GB (Redis cluster)
```

### Key Insight
This system is overwhelmingly read-heavy (100:1 ratio). Cache hit rate will be high because URL access follows a power-law distribution (few URLs get most clicks). Design should optimize for reads.

---

## Step 3: High-Level Architecture

```
[Client Browser]
       |
       v
[CDN Edge] -- cache popular redirects (301 for immutable, 302 for analytics)
       |
       v
[Global Load Balancer] (DNS-based geo-routing)
       |
       +-----------+-----------+
       |           |           |
   [Region A]  [Region B]  [Region C]
       |
   [API Gateway] (rate limit, auth, routing)
       |
       +-------------------+-------------------+
       |                   |                   |
[Write Service]      [Read Service]      [Analytics Service]
       |                   |                   |
[Key Generator]      [Redis Cache]        [Kafka]
       |                   |                   |
       +-------> [Sharded Database] <---------+
                 (PostgreSQL / DynamoDB)        |
                                          [Flink/Spark]
                                               |
                                          [Data Warehouse]
```

### Write Path (Shorten URL)
```
1. Client POSTs { long_url, custom_alias?, ttl? }
2. API Gateway: rate limit check, authentication
3. Write Service:
   a. Validate URL (well-formed, not blocked domain)
   b. Optional dedup: hash(long_url) lookup - same URL returns existing code
   c. Generate short code (counter -> base62)
   d. Insert record in DB
   e. Populate cache (write-through)
4. Return short URL to client
```

### Read Path (Redirect)
```
1. Client GETs short.url/abc1234
2. CDN edge: check cache (if 301, serve directly)
3. If miss -> API Gateway -> Read Service
4. Read Service: check Redis cache
   a. Cache HIT: return 302 redirect + log analytics event
   b. Cache MISS: query DB -> populate cache -> return 302
5. Async: publish click event to Kafka for analytics
```

---

## Step 4: Deep-Dive - Short Code Generation

### Why Base62 (Not Base64)?

Base64 uses characters: `A-Z, a-z, 0-9, +, /`

The `+` and `/` characters are problematic in URLs:
- `/` conflicts with path separators
- `+` is interpreted as space in query strings
- `=` (padding) adds confusion

Base62 uses only: `A-Z, a-z, 0-9` (62 characters) - all URL-safe without encoding.

**Why not Base58?** (removes 0, O, l, I to avoid visual confusion)
- Slightly less space-efficient (need longer codes)
- Used by Bitcoin addresses where human readability matters
- For URLs, visual confusion is less of a concern

### Why 7 Characters?

```
Mathematical justification:
  62^6 = 56,800,235,584     (~56 billion) - sufficient for 5 years
  62^7 = 3,521,614,606,208  (~3.5 trillion) - sufficient for 580+ years
  62^8 = 218,340,105,584,896 (~218 trillion) - overkill

We need 6 billion codes for 5 years.
62^7 gives 3.5 trillion - that is 583x headroom.
Even with 50% utilization waste, 7 chars is more than enough.

Why not 6? 62^6 = 56B is enough numerically, but:
  - Leaves less headroom for growth
  - Counter-based approach may waste some IDs (gaps)
  - 7 chars is still very short for a URL
```

### Counter Service Design Comparison

| Approach | How It Works | Pros | Cons |
|----------|-------------|------|------|
| **ZooKeeper** | Centralized counter, range allocation | Consistent, proven | Single point of failure, latency |
| **Ticket Server** (Flickr) | DB auto-increment, 2 servers | Simple, reliable | Limited throughput per server |
| **Snowflake ID** (Twitter) | Timestamp + machine + sequence | No coordination, high throughput | 64-bit (too long for base62 URL) |
| **Pre-generated Pool** | Background worker fills Redis set | O(1) allocation, no coordination | Pool exhaustion risk, complexity |
| **Hash-based** | MD5(long_url)[:7] | Stateless, deterministic | Collisions require handling |

### Recommended: Range-Based Counter with ZooKeeper

```
ZooKeeper assigns ranges:
  Server A: range [1, 1_000_000]
  Server B: range [1_000_001, 2_000_000]
  Server C: range [2_000_001, 3_000_000]

Each server increments locally (no network call per ID).
When range exhausted, request new range from ZooKeeper.

Benefits:
  - No coordination between servers (range is pre-assigned)
  - Each server generates IDs at local memory speed
  - ZooKeeper only consulted once per million IDs
  - No collisions (ranges are disjoint)
  - Predictable, sortable (useful for analytics)

Drawback:
  - Sequential IDs are predictable (security concern)
  - Solution: apply a bijective scrambling function (Feistel cipher)
    encode(counter) -> scrambled -> base62 -> short_code
    This makes codes appear random while maintaining uniqueness
```

### Pre-Generated Key Pool (Alternative)

```
Background worker:
  - Generate random 7-char base62 strings
  - Check for uniqueness against DB
  - Push to Redis SET (unused_keys)
  - Maintain pool size > 10,000 keys

On shorten request:
  - SPOP from Redis SET (atomic, O(1))
  - Associate with long URL in DB

Advantages:
  - Zero computation at request time
  - Guaranteed unique (pre-checked)
  - Decouples generation from serving

Disadvantages:
  - Pool exhaustion if worker falls behind (needs monitoring)
  - Extra infrastructure (background worker, Redis pool)
  - Harder to make deterministic (same long URL -> different short code each time)
```

---

## Deep-Dive: Caching Strategy

### Cache Warming

Problem: Cold cache after deployment or cache flush causes a thundering herd to the database.

```
Strategies:
1. Pre-warm: Before cutting over, populate cache with top-N URLs
   - Query analytics: SELECT short_code, long_url FROM urls
     ORDER BY click_count DESC LIMIT 1000000
   - Load into Redis before routing traffic

2. Gradual rollout: Use consistent hashing, add nodes one at a time
   - Each new node only handles 1/N of traffic initially
   - Its cache warms naturally from DB reads

3. Read-through with circuit breaker:
   - If DB latency spikes (cache miss storm), serve stale cached data
   - Or queue requests and batch-fetch from DB
```

### Cache Invalidation

URLs rarely change (immutable once created). Invalidation is needed only for:
- URL expiry (TTL passed)
- URL deletion (abuse/takedown)
- Long URL update (rare, if supported)

```
Strategy: TTL-based expiry + explicit invalidation
- Set Redis TTL slightly longer than URL expiry (e.g., URL expires in 7 days, cache TTL = 7 days + 1 hour)
- On explicit delete: DEL key from Redis + mark deleted in DB
- No complex invalidation logic needed (immutable data is cache-friendly)
```

---

## Deep-Dive: Analytics Pipeline

### Why Async Analytics?

The redirect (read) path must be fast (< 100ms). Synchronously writing analytics would add:
- DB write latency: 5-20ms
- Risk of analytics DB failure blocking redirects
- Coupling between read path and analytics

### Architecture

```
[Read Service] -- click event --> [Kafka Topic: url_clicks]
                                       |
                                  [Flink/Spark Streaming]
                                       |
                    +------------------+------------------+
                    |                  |                  |
              [Real-time]        [Batch hourly]    [Daily rollup]
              Aggregator         Aggregator         Aggregator
                    |                  |                  |
              [Redis Counter]    [ClickHouse]      [BigQuery/Redshift]
              (live click count) (time-series)     (historical analytics)
```

### Click Event Schema

```json
{
  "short_code": "abc1234",
  "timestamp": "2024-01-15T10:30:00Z",
  "client_ip": "hash(ip)", 
  "user_agent": "Mozilla/5.0...",
  "referrer": "https://twitter.com/...",
  "geo": { "country": "US", "city": "SF" },
  "device": "mobile"
}
```

### Why Kafka?

- Decouples producer (read service) from consumers (analytics)
- Handles traffic spikes (buffers during peak)
- Multiple consumers: real-time counter, batch aggregation, fraud detection
- Replay capability: if analytics pipeline crashes, replay events from offset
- Retention: keep raw events for 7 days for debugging/reprocessing

---

## Deep-Dive: Abuse Prevention

### Threat Model

| Threat | Impact | Mitigation |
|--------|--------|------------|
| Spam short URLs (phishing) | Brand damage, user harm | Domain blacklist, ML classifier |
| API abuse (mass generation) | Storage cost, ID exhaustion | Rate limiting per user/IP |
| Redirect to malware | User harm | URL scanning (Google Safe Browsing API) |
| Enumeration attack | Privacy breach | Non-sequential codes (scrambled counter) |
| DDoS on read path | Availability | CDN, rate limiting, auto-scaling |

### Rate Limiting Implementation

```
Per-user: 100 URLs/hour (authenticated)
Per-IP: 10 URLs/hour (anonymous)
Global: 10,000 URLs/second (circuit breaker)

Implementation: Token bucket in Redis
  Key: rate_limit:{user_id}
  Value: {tokens: 95, last_refill: timestamp}
  
  On request:
    1. Check tokens > 0
    2. Decrement token
    3. If tokens == 0, return 429 Too Many Requests
    4. Refill at rate of 100/hour
```

### URL Scanning Pipeline

```
On shorten:
  1. Check domain against blacklist (Redis SET, O(1))
  2. If not blacklisted, create URL immediately (don't block user)
  3. Async: submit to scanning pipeline
     a. Google Safe Browsing API check
     b. VirusTotal URL scan
     c. ML model (features: domain age, TLD, path patterns)
  4. If flagged: mark URL as disabled, notify creator
```

---

## Deep-Dive: Multi-Region Deployment

### Why Multi-Region?

- Users are globally distributed (latency reduction)
- Availability: region failure should not take down the service
- Compliance: some regions require data locality (GDPR)

### Architecture

```
                    [Global DNS (Geo-routing)]
                    /          |           \
            [US-East]     [EU-West]     [AP-South]
           /    |    \
    [CDN Edge] [LB] [Services]
                |
          [Regional DB]  <-- async replication --> [Other regions]
          [Regional Cache]
```

### Data Replication Strategy

```
Write: single-leader (one region owns writes for a given URL)
  - Short code's first character determines home region
  - 'a'-'j' -> US-East, 'k'-'t' -> EU-West, 'u'-'9' -> AP-South
  - Writes route to home region, replicate async to others

Read: multi-leader (any region can serve any read)
  - Reads served from local replica
  - Eventual consistency is acceptable (URL content rarely changes)
  - If cache miss + local DB miss: query home region (rare, only for very new URLs)
```

### Conflict Resolution

Since URLs are immutable once created, write conflicts are impossible (same code cannot be assigned twice due to counter ranges). The only conflict scenario is simultaneous creation of the same custom alias in different regions - resolved by checking home region before confirming.

---

## Database Schema and Sharding

### Schema (SQL)
```sql
CREATE TABLE urls (
    short_code VARCHAR(7) PRIMARY KEY,
    long_url TEXT NOT NULL,
    user_id BIGINT,
    created_at TIMESTAMP DEFAULT NOW(),
    expires_at TIMESTAMP,
    click_count BIGINT DEFAULT 0, -- approximate, updated periodically
    is_active BOOLEAN DEFAULT TRUE
);

CREATE INDEX idx_user_urls ON urls(user_id, created_at DESC);
CREATE INDEX idx_expiry ON urls(expires_at) WHERE expires_at IS NOT NULL;
```

### Sharding Strategy

```
Shard key: hash(short_code) % num_shards
Why short_code? 
  - Read path uses short_code (most frequent operation)
  - Uniform distribution (codes are random-looking)
  - No hot shards (unlike sharding by user_id where one user could be very active)

Number of shards: start with 16, expand to 256 as data grows
Resharding: consistent hashing allows adding shards without full redistribution
```

### Why Not NoSQL (DynamoDB)?

| Dimension | PostgreSQL + Sharding | DynamoDB |
|-----------|----------------------|----------|
| Consistency | Strong (ACID) | Eventually consistent (or strong with cost) |
| Query flexibility | Full SQL, joins, analytics | Key-value only, limited queries |
| Cost at scale | Predictable (hardware) | Pay-per-request (expensive at 38K QPS) |
| Operational | More ops work (sharding, replicas) | Fully managed |
| Custom indexes | Flexible | Limited (GSI, max 20) |

For a URL shortener: either works well. PostgreSQL if you want rich analytics queries. DynamoDB if you want zero-ops and can afford per-request pricing.

---

## 301 vs 302 Redirect Decision

| Status Code | Meaning | Caching | Analytics | Use When |
|-------------|---------|---------|-----------|----------|
| **301** (Moved Permanently) | Browser caches redirect | Browser skips our server on subsequent visits | Lose click tracking | Immutable URLs, no analytics needed |
| **302** (Found / Temporary) | Browser always hits our server | Every click passes through us | Full analytics | Need click tracking, URL might change |

**Recommendation**: Use 302 for analytics-enabled URLs (default). Offer 301 as an option for users who want maximum performance and do not need tracking.

---

## Files
- [base62_encoder.cpp](base62_encoder.cpp) - Base62 encoding/decoding implementation
- [url_shortener_service.cpp](url_shortener_service.cpp) - In-memory URL shortener simulation

## Interview Questions
1. Why base62 (not base64)? URL safety - no `/`, `+`, `=` characters that need encoding.
2. Why 7 chars? 62^7 = 3.5 trillion combinations - 583x headroom over 5-year requirement.
3. How to avoid collisions? Counter-based approach guarantees uniqueness by design.
4. How to handle custom aliases? Reserve namespace check (lookup before insert), fallback to generated.
5. How to handle expired URLs? Lazy expiry on read (check timestamp) + background sweeper for cache cleanup.
6. How to scale reads to 100K QPS? CDN edge caching + Redis cluster + read replicas.
7. How to do analytics without slowing read path? Async via Kafka -> stream processor -> warehouse.
8. How to prevent abuse? Rate limiting (token bucket), domain blacklist, async URL scanning.
9. What if counter service goes down? Range-based allocation means local server has 1M IDs buffered.
10. 301 vs 302 - which and why? 302 for analytics (every click hits us); 301 for max performance.
11. How to handle the thundering herd on cache miss? Request coalescing (single-flight) - one DB query, fan out to all waiters.
12. Multi-region: how to ensure same code is not assigned twice? Disjoint counter ranges per region.

## Daily Assignment
1. Implement base62 encoder/decoder with property: decode(encode(x)) == x for all x.
2. Build in-memory URL shortener with counter-based generation; add custom alias support.
3. Add an LRU cache layer in front of the "database" (map); measure hit rate.
4. Implement rate limiting (token bucket) for the shorten endpoint.
5. Sketch a sequence diagram for the write flow including all error cases.
