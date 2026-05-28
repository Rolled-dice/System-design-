# Day 27 - HLD: Twitter + Uber

## HLD Interview Framework (Quick Reference)

Refer to [Day 26](../day26-url-shortener-hld/README.md) for the full 4-step HLD framework. Applied here:

1. **Requirements**: Social feed + real-time ride matching
2. **Capacity Estimation**: Twitter: 500M tweets/day at 1KB each; Uber: 2.5M location updates/sec
3. **High-Level Design**: Fanout architecture for Twitter; geo-indexed matching for Uber
4. **Deep-Dive**: Celebrity fanout math, H3 hexagonal indexing, ETA computation

---

## Part A - Twitter (Social Feed)

### Requirements

**Functional**:
1. Post a tweet (text, optional media, max 280 chars)
2. Follow / unfollow users
3. Home timeline (tweets from people you follow, reverse chronological + ranked)
4. User timeline (tweets you posted)
5. Search, hashtags, mentions, likes, retweets

**Non-Functional**:
- ~500M users, ~200M DAU
- ~500M tweets/day -> ~5,800 tweets/sec avg, peak ~50K/sec
- Read:Write ~ 100:1 (timelines read far more than tweets posted)
- Timeline render latency: < 200ms p99
- Availability: > 99.99% for reads, > 99.9% for writes

### Capacity Estimation

```
Storage:
  500M tweets/day * 1KB avg = 500 GB/day
  180 TB/year (text only)
  Media: 10% of tweets have images (50M * 500KB = 25 TB/day)

Timeline reads:
  200M DAU * 10 timeline loads/day = 2B timeline reads/day
  2B / 86400 = ~23,000 timeline reads/sec
  Peak: ~230,000/sec

Fanout writes (on tweet by normal user with 1000 followers):
  5,800 tweets/sec * 1000 followers = 5.8M timeline writes/sec

Cache:
  Each user's timeline cache: 800 tweets * 8 bytes (tweet ID) = 6.4 KB
  200M active timelines * 6.4 KB = ~1.3 TB Redis (timeline IDs only)
```

---

### Snowflake ID Design Rationale

**Why not UUID?**
- UUID is 128 bits (16 bytes) vs Snowflake 64 bits (8 bytes) - 2x storage saving at billions of records
- UUID is random - no natural ordering, terrible for B-tree index locality
- UUID cannot be used as a cursor for pagination without a secondary sort column

**Why not auto-increment?**
- Single point of failure (one DB generates all IDs)
- Cannot shard (which shard holds the next ID?)
- Exposes business metrics (ID 5000 means 5000th tweet)

**Snowflake structure (64 bits)**:
```
+------------------------------------------------------------------+
| 1 bit unused | 41 bits timestamp | 5 bits DC | 5 bits machine | 12 bits sequence |
+------------------------------------------------------------------+

- Timestamp: milliseconds since custom epoch -> 2^41 ms = ~69 years
- Datacenter ID: 2^5 = 32 datacenters
- Machine ID: 2^5 = 32 machines per DC
- Sequence: 2^12 = 4096 IDs per millisecond per machine

Total capacity: 32 DCs * 32 machines * 4096/ms = 4.19M IDs/sec globally
```

**Key properties**:
1. **Time-sortable**: IDs increase with time (roughly) -> natural chronological order
2. **No coordination**: Each machine generates independently (no network call)
3. **Datacenter-aware**: Embedded DC ID helps with locality debugging
4. **k-sorted**: IDs from same millisecond may be slightly out of order across machines, but within 1ms window - acceptable for timelines

---

### Fanout Analysis with Mathematical Proof

**The Celebrity Problem**:

Consider a celebrity with 100M followers who tweets once.

**Fanout-on-Write (Push model)**:
```
Cost per tweet:
  - Write tweet to Tweet DB: 1 write
  - Push tweet ID to each follower's timeline cache: 100M writes to Redis

Time calculation:
  - Redis LPUSH latency: ~0.1ms per operation
  - Single-threaded: 100M * 0.1ms = 10,000 seconds (~2.8 hours!)
  - Even with 1000 workers in parallel: 10 seconds per celebrity tweet
  - During those 10 seconds, celebrity tweets again... backlog builds

Memory cost:
  - Each follower's timeline grows by 8 bytes (tweet ID)
  - 100M * 8 bytes = 800 MB per tweet by this celebrity
  - 10 tweets/day = 8 GB/day just for ONE celebrity

Conclusion: Pure push is INFEASIBLE for celebrities.
```

**Fanout-on-Read (Pull model)**:
```
Cost per timeline read:
  - Get list of users I follow: 1 query (cached)
  - For each followee, get their latest tweets: N queries (N = followee count)
  - Merge and sort: O(N * K * log(NK)) where K = tweets per followee

Time calculation:
  - Average user follows 200 people
  - 200 Redis queries * 0.1ms = 20ms just for fetching
  - Merge sort of 200 * 20 = 4000 tweets: < 1ms
  - Total: ~21ms - FAST for individual read

Problem: at 230K timeline reads/sec * 200 Redis queries each = 46M Redis reads/sec
  - Feasible with Redis cluster, but high infra cost
```

**Hybrid approach (Twitter's actual solution)**:
```
Rules:
  - Users with < 10,000 followers: fanout-on-write (push to all followers)
  - Users with >= 10,000 followers ("celebrities"): do NOT fan out
  - On timeline read: merge pushed timeline + pull celebrities' recent tweets

Math for hybrid:
  - Push handles 99% of tweets (most users have < 10K followers)
  - Pull only needed for ~0.1% of followees (celebrities)
  - Average user follows ~5 celebrities -> 5 Redis queries on read
  - Timeline read cost: O(1) for cached push timeline + O(5) for celebrity pull
  - Total: ~1-2ms for timeline assembly

Fanout cost:
  - Average non-celebrity has ~500 followers
  - 5800 tweets/sec * 500 = 2.9M push operations/sec (manageable)
  - Celebrity tweets: zero push cost, only pulled when their followers read
```

---

### Timeline Cache Structure

```
Redis data structure: Sorted Set per user
  Key: timeline:{user_id}
  Score: tweet_id (Snowflake - time-sortable, so score = chronological order)
  Value: tweet_id

Operations:
  - ZADD timeline:{user_id} {tweet_id} {tweet_id}  (push new tweet)
  - ZREVRANGE timeline:{user_id} 0 19                (get latest 20 tweets)
  - ZREMRANGEBYRANK timeline:{user_id} 0 -801        (trim to 800 entries)
  - ZCARD timeline:{user_id}                          (timeline size)

Why Sorted Set?
  - O(log N) insertion (maintains sort order)
  - O(log N + M) range queries (get tweets 20-40 for pagination)
  - Built-in dedup (same tweet ID cannot appear twice)
  - ZREMRANGEBYRANK for memory-bounded trimming
  - Scores allow cursor-based pagination (last_tweet_id as cursor)
```

**Timeline read with celebrity merge**:
```
1. ZREVRANGE timeline:{user_id} 0 19   -> pushed tweets (from non-celebs)
2. For each celebrity the user follows:
     ZREVRANGE user_tweets:{celeb_id} 0 4  -> recent 5 tweets
3. Merge all results, sort by tweet_id (time), take top 20
4. Fetch full tweet objects from Tweet Cache/DB
5. Return assembled timeline
```

---

### Search Architecture: Inverted Index

```
Tweet "The quick brown fox #coding" posted at 2024-01-15T10:30:00

Indexing pipeline:
  [Tweet Service] -> [Kafka: new_tweets] -> [Search Indexer Consumer]
                                                     |
                                              [Tokenize + Analyze]
                                              "the", "quick", "brown", "fox", "coding"
                                                     |
                                              [Write to Elasticsearch]

Inverted index in Elasticsearch:
  "quick" -> [tweet_123, tweet_456, tweet_789]
  "brown" -> [tweet_123, tweet_234]
  "fox"   -> [tweet_123, tweet_567]
  "#coding" -> [tweet_123, tweet_890, ...]

Search query "quick fox":
  - Intersect posting lists for "quick" AND "fox"
  - Result: tweet_123
  - Rank by recency, engagement, relevance score
```

**Real-time indexing (Early Bird)**:
Twitter's "Early Bird" system keeps the last ~7 days of tweets in an in-memory inverted index for real-time search. Older tweets fall back to on-disk Lucene indexes.

```
Why in-memory for recent tweets?
  - "What's happening NOW" queries need sub-100ms latency
  - Recent tweets are most searched (breaking news, trending events)
  - 7 days of tweets at 500M/day = 3.5B documents in-memory
  - At 100 bytes per index entry: ~350 GB distributed across cluster
```

---

### Trending Topics Algorithm

**Requirements**: Surface topics that are currently popular AND novel (not just always-popular topics).

**Formula: Exponential Decay + Novelty**:
```
trend_score(topic, t) = volume(topic, t) * novelty(topic, t) * decay(t)

Where:
  volume(topic, t) = count of tweets mentioning topic in window [t-1h, t]
  
  novelty(topic, t) = volume(topic, t) / baseline(topic)
    - baseline = average volume over past 7 days
    - High novelty = sudden spike relative to normal
    - "#christmas" on Dec 25: high volume but low novelty (expected)
    - "#earthquake" suddenly: lower volume but very high novelty

  decay(t) = e^(-lambda * age)
    - lambda controls how fast old trends fade
    - Typical: lambda = 0.1, meaning score halves every ~7 hours
```

**Why novelty matters**: Without it, topics like "news", "sports", "music" would always trend because they have permanently high volume. Novelty ensures only SPIKES surface.

**Implementation**:
```
[Tweet Stream] -> [Kafka] -> [Trending Service]
                                    |
                              [Sliding Window Counter]
                              (count per topic per 5-min bucket)
                                    |
                              [Score Calculator]
                              (apply formula above)
                                    |
                              [Top-K Heap]
                              (maintain top 50 trends per region)
                                    |
                              [Cache] -> [Client]
                              (refresh every 5 minutes)
```

---

### Twitter Architecture Summary

```
[Mobile/Web Client]
       |
[API Gateway] (GraphQL)
       |
+------+------+------+------+
|      |      |      |      |
Tweet  Timeline  User  Search  Trending
Svc    Svc       Graph  Svc    Svc
|      |      |      |      |
|  [Redis    [Graph  [Elastic- [Redis
|  Timeline]  DB]    search]   Counters]
|
[Tweet DB] + [Kafka] -> [Fanout Workers] -> [Redis Timelines]
                    |
               [Analytics Pipeline]
```

---

## Part B - Uber (Ride-Hailing)

### Requirements

**Functional**:
1. Driver online/offline + location updates (every 4-5 seconds)
2. Rider requests ride -> match nearest available driver
3. Real-time trip tracking (both parties see live location)
4. Pricing (base + distance + time + surge)
5. Payments, ratings, trip history

**Non-Functional**:
- ~10M active drivers, ~100M riders
- Driver location updates: 10M * 0.25/sec = 2.5M updates/sec at peak
- Match latency: < 2 seconds from request to driver assignment
- Geo queries: "drivers within 3km" at 100K QPS
- Location accuracy: < 50 meters

### Capacity Estimation

```
Location updates:
  2.5M/sec * 50 bytes (lat, lng, timestamp, driver_id) = 125 MB/sec ingress
  Daily: 125 MB/sec * 86400 = ~10.8 TB/day (historical)
  Hot storage (last 30 seconds): 2.5M drivers * 50 bytes = 125 MB (fits in memory)

Geo queries:
  100K QPS * ~500 bytes response = 50 MB/sec egress

Trip data:
  5M trips/day * 2KB per trip record = 10 GB/day
```

---

### H3 Hexagonal Indexing - Why Hexagons?

Uber uses H3, a hierarchical hexagonal grid system. Understanding WHY hexagons are chosen over other shapes reveals deep geometric reasoning.

**Comparison of grid systems**:

```
+--------------------------------------------------------------+
| Property          | Squares    | Triangles  | Hexagons       |
+-------------------+------------+------------+----------------+
| Neighbors         | 4 (edge)   | 3 (edge)   | 6 (edge)       |
|                   | 8 (incl.   | 12 (incl.  | 6 (all equal)  |
|                   | diagonal)  | vertex)    |                |
| Distance to       | Varies:    | Varies     | UNIFORM: all   |
| neighbor center   | 1 (adj)    |            | neighbors are  |
|                   | sqrt(2)    |            | equidistant    |
|                   | (diagonal) |            |                |
| Edge effects      | Diagonal   | Complex    | None (uniform) |
|                   | distance   | shapes     |                |
|                   | != edge    |            |                |
|                   | distance   |            |                |
| Tessellation      | Yes        | Yes        | Yes            |
| Visual regularity | High       | Low        | High           |
+-------------------+------------+------------+----------------+
```

**Why uniform neighbor distance matters for Uber**:

With squares, the cell at position (0,1) is 1 unit away, but the cell at (1,1) is sqrt(2) = 1.414 units away. This means "search neighboring cells" gives irregular coverage - diagonal neighbors are 41% further.

With hexagons, ALL 6 neighbors are exactly the same distance from the center. This means:
- "Search this cell + 6 neighbors" gives a perfect, uniform coverage ring
- No diagonal artifacts in nearest-driver search
- Expanding search by one ring always adds uniform coverage

**H3 Resolution Levels**:
```
Resolution 0: ~4.3M km^2 per hex (continent-scale)
Resolution 4: ~1,770 km^2 (city-scale)
Resolution 7: ~5.16 km^2 (neighborhood-scale) <- Uber uses this for matching
Resolution 9: ~0.105 km^2 (block-scale) <- for surge pricing
Resolution 15: ~0.9 m^2 (sub-meter, maximum precision)
```

**Operations**:
```cpp
// Convert lat/lng to H3 index
H3Index h3 = geoToH3(lat, lng, resolution=7);

// Get neighboring hexagons (k-ring)
vector<H3Index> ring1 = kRing(h3, 1);  // 7 hexes (self + 6 neighbors)
vector<H3Index> ring2 = kRing(h3, 2);  // 19 hexes (ring of 12 around ring1)

// All drivers in ring:
for (H3Index hex : ring1) {
    drivers += driverIndex.getDriversInHex(hex);
}
```

---

### ETA Computation

**Problem**: Given a driver at location A and a rider at location B, estimate time of arrival.

**Approach: Weighted Graph + Dijkstra with Live Traffic**:

```
Road network modeled as weighted directed graph:
  - Nodes: intersections
  - Edges: road segments
  - Weight: travel_time = distance / speed

  where speed = speed_limit * congestion_factor

Congestion factor (0.0 to 1.0):
  - Computed from recent driver GPS traces on that road segment
  - Updated every 30-60 seconds
  - 1.0 = free flow, 0.2 = severe congestion (5x slower)

ETA computation:
  1. Snap driver and rider locations to nearest road nodes
  2. Run Dijkstra (or A*) on weighted graph
  3. Sum edge weights along shortest path
  4. Add pickup time estimate (30-60 seconds)
```

**Why not straight-line distance / average speed?**
- Roads are not straight lines (actual path may be 2-3x straight-line distance)
- Traffic varies by road and time of day (highway may be faster despite longer distance)
- One-way streets, turn restrictions, road closures affect routing

**Optimization for speed** (A* over Dijkstra):
```
A* heuristic: straight-line distance / max_possible_speed
  - Admissible (never overestimates true cost)
  - Prunes 60-80% of nodes vs plain Dijkstra
  - ETA computation in ~5ms for city-scale graphs

Contraction Hierarchies (CH):
  - Precompute shortcut edges between important nodes
  - Query time: < 1ms (vs 5ms for A*)
  - Trade-off: preprocessing takes hours, must redo when road network changes
  - Uber uses CH for static graph + live traffic overlay
```

---

### Surge Pricing Algorithm

**Goal**: Balance supply and demand per geographic area by adjusting price.

**Formula**:
```
surge_multiplier(hex_cell) = max(1.0, demand(cell) / supply(cell))

Where:
  demand(cell) = number of ride requests in cell in last 5 minutes
  supply(cell) = number of available drivers in cell in last 5 minutes

Capped at max_multiplier (e.g., 5.0x)
Smoothed: new_surge = 0.7 * old_surge + 0.3 * computed_surge (avoid oscillation)
```

**Why per-hex-cell?**
- Stadium lets out: demand spikes in 2-3 hexes, not entire city
- Granular pricing incentivizes drivers to move to high-demand areas
- Resolution 9 (~300m diameter) gives block-level granularity

**Implementation**:
```
[Ride Requests] -> [Request Counter per H3 cell] (sliding 5-min window)
[Driver Locations] -> [Driver Counter per H3 cell] (real-time count)

Every 30 seconds:
  for each active cell:
    demand = request_count(cell, last_5_min)
    supply = available_driver_count(cell, now)
    raw_surge = demand / max(supply, 1)
    smoothed = 0.7 * previous_surge(cell) + 0.3 * raw_surge
    surge(cell) = clamp(smoothed, 1.0, 5.0)
```

---

### Driver-Rider Matching: Scoring Function

**Naive approach**: Match with nearest driver. Problem: nearest driver might have low rating, low acceptance rate, or be about to complete another trip.

**Scoring function**:
```
score(driver, rider) = w1 * (1/ETA) + w2 * rating + w3 * acceptance_rate + w4 * trip_alignment

Where:
  1/ETA: closer drivers score higher (inverse of time)
    - Normalized: 1/ETA_minutes, capped at 1/1 = 1.0 for very close drivers
  
  rating: driver's average rating (4.0-5.0 scale, normalized to 0-1)
    - (rating - 4.0) / 1.0

  acceptance_rate: historical acceptance % (0.0 to 1.0)
    - Low acceptance means driver likely to reject -> wasted time

  trip_alignment: does driver's current heading align with rider direction?
    - cos(angle_between_driver_heading_and_rider_direction)
    - Reduces detour for driver, better experience

Weights (tuned by ML from historical data):
  w1 = 0.5 (proximity is most important)
  w2 = 0.2 (quality matters)
  w3 = 0.2 (reliability matters)
  w4 = 0.1 (nice to have)
```

**Matching flow**:
```
1. Rider requests ride at (lat, lng)
2. Geo query: find all available drivers within 3km (k-ring search)
3. For each candidate driver: compute score
4. Sort by score descending
5. Offer to top-scored driver (timeout: 15 seconds)
6. If rejected/timeout: offer to next driver
7. If all reject in radius: expand radius to 5km, retry
8. If still no match after 60s: inform rider "no drivers available"
```

---

### WebSocket at Scale for Real-Time Location

**Problem**: 10M drivers each have a persistent connection sending updates every 4 seconds. How to manage 10M concurrent WebSocket connections?

**Architecture**:
```
[10M Drivers] -> [Connection Router (DNS/LB)] -> [Connection Server Pool]
                                                     |
                    +--------------------------------+
                    |        |        |        |
                 [CS-1]   [CS-2]  [CS-3]  ... [CS-N]
                 (50K      (50K    (50K
                 connections) ...)   ...)
                    |
              Each CS handles:
                - 50,000 concurrent WebSocket connections
                - Receives location updates
                - Forwards to Location Ingest Service
                - Receives dispatch messages (trip offers)
                - Pushes to connected driver
```

**Connection routing**:
```
Connection Registry (Redis):
  driver:{driver_id} -> connection_server_id

When rider matches with driver:
  1. Look up driver's connection server: GET driver:12345 -> CS-7
  2. Send dispatch message to CS-7
  3. CS-7 pushes to driver's WebSocket
```

**Heartbeat and reconnection**:
```
- Server sends ping every 30 seconds
- If no pong within 5 seconds: mark connection dead
- Client auto-reconnects with exponential backoff (1s, 2s, 4s, 8s, max 30s)
- On reconnect: re-register in connection registry
- During disconnect: driver marked "potentially offline" (grace period 30s)
```

**Scaling math**:
```
10M connections / 50K per server = 200 connection servers
Each server: 50K connections * 1 update/4s = 12,500 messages/sec inbound
Total ingest: 2.5M messages/sec -> Location Ingest Service (Kafka-backed)
```

---

### Uber Architecture Summary

```
[Driver App] <--WebSocket--> [Connection Servers (200+)]
                                     |
                              [Location Ingest] -> [Kafka: driver_locations]
                                     |                    |
                              [Geo Index Service]    [Historical Store]
                              (H3 + Redis)           (Cassandra)
                                     |
[Rider App] --HTTPS--> [API Gateway] -> [Match Service]
                                              |
                                     [Trip Service] -> [Trip DB]
                                              |
                                     [Pricing Service] (surge)
                                              |
                                     [Payment Service]
                                              |
                                     [Notification Service]
```

---

## Files
- [snowflake_id.cpp](snowflake_id.cpp) - 64-bit unique ID generation
- [timeline_fanout.cpp](timeline_fanout.cpp) - Hybrid fanout simulator
- [geohash.cpp](geohash.cpp) - Geohash encode/decode + neighbor cells
- [driver_index.cpp](driver_index.cpp) - In-memory geo index for nearby driver query

## Interview Questions

### Twitter
1. Pull vs Push vs Hybrid - when to use each? (Push for normal users, pull for celebrities, hybrid for balance)
2. How do you handle celebrities? Prove with math why push fails. (100M Redis writes at 0.1ms = 10,000s)
3. Snowflake ID - why not UUID? (Sortable, smaller, k-sorted, embeds time, no coordination)
4. How does search work? (Kafka -> tokenize -> inverted index in Elasticsearch, Early Bird for real-time)
5. How to compute trending topics? (Volume * novelty * decay; novelty = current/baseline ratio)
6. How would you implement retweets without duplicating data? (Reference to original tweet_id, display at retweet time)
7. Timeline pagination? (Cursor-based with tweet_id, never offset - offset is O(N) and inconsistent)

### Uber
1. GeoHash vs QuadTree vs H3 - trade-offs? (H3: uniform neighbors, hierarchical, no diagonal artifacts)
2. How to handle 2.5M location updates/sec? (Sharded ingest by city/region, Kafka buffer, in-memory geo index)
3. Why not store every driver location in SQL? (2.5M writes/sec exceeds any SQL DB; need in-memory + async persist)
4. How to match the BEST driver, not just nearest? (Weighted scoring: 1/ETA * rating * acceptance_rate)
5. How to ensure exactly one driver gets the request? (State machine with CAS: OFFERED -> ACCEPTED, reject if not OFFERED)
6. Surge pricing - real-time supply/demand? (Count per H3 cell, ratio with smoothing, capped multiplier)
7. How to scale to a new city? (Separate regional cluster with its own geo index, trip service, drivers)
8. WebSocket at scale? (Connection server pool, 50K/server, registry for routing, heartbeat for liveness)

## Daily Assignment
1. Implement Snowflake ID generator: verify 64-bit, monotonic within machine, unique across machines.
2. Implement geohash encode at 6-char precision; verify nearby locations share prefix.
3. Implement in-memory `DriverIndex` with `update(driverId, lat, lng)` and `nearby(lat, lng, radiusKm) -> [driverIds]`.
4. Implement the fanout scoring function: given a rider and list of drivers, return ranked matches.
5. Sketch sequence diagram: rider request -> match -> driver accept -> trip start -> trip end.
