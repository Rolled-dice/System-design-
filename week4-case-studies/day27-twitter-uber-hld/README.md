# Day 27 - HLD: Twitter + Uber

## Part A - Twitter

### Functional Requirements
1. Post a tweet (text, optional media, max 280 chars)
2. Follow / unfollow users
3. Home timeline (tweets from people you follow)
4. User timeline (tweets you posted)
5. Search, hashtags, mentions, likes, retweets

### Scale
- ~500M users, ~200M DAU
- ~500M tweets/day -> ~5800 tweets/sec avg, peak ~50k/sec
- Read:Write ~ 100:1 (timelines read way more than tweets posted)
- Celebrity problem: a user with 100M followers

### Storage Estimate
- 500M tweets/day * 1KB = 500GB/day -> 180TB/year
- Media in object store (S3) + CDN

### Architecture

```
[Client] -> [LB] -> [API Gateway]
                       |
        +--------------+---------------+
        |              |               |
   [Tweet Svc]   [Timeline Svc]   [User Graph Svc]
        |              |               |
   [Tweet DB]     [Redis Timeline]  [Graph DB / Cassandra]
        |
   [Kafka] -> [Fanout Worker] -> [Redis per-user timeline]
        |
   [Search Indexer] -> [Elasticsearch]
        |
   [Media] -> [S3 + CDN]
```

### Timeline Generation - 3 Approaches

| Approach | How | Pros | Cons |
|----------|-----|------|------|
| **Pull (read-time fanout)** | Compute timeline on read by querying tweets of followees | Cheap writes, fresh | Slow reads, hot followees |
| **Push (write-time fanout)** | On tweet, write to all followers' redis timelines | Fast reads | Expensive for celebs (100M writes) |
| **Hybrid** | Push for normal users; Pull for celebrities | Balanced | More complex |

Twitter uses **hybrid**.

### Tweet Storage
- ID: Snowflake (64-bit: timestamp + machine + seq)
- Sharded by `user_id` for user timeline
- Cassandra/Manhattan for tweets

### Caching
- Hot home timelines in Redis (last 800 tweets per user)
- Trends, who-to-follow precomputed

### Files
- [snowflake_id.cpp](snowflake_id.cpp) - 64-bit unique IDs
- [timeline_fanout.cpp](timeline_fanout.cpp) - hybrid fanout simulator

### Twitter Interview Questions
1. Pull vs Push vs Hybrid - when to use each? Trade-offs?
2. How do you handle celebrities? (don't fan out at write time)
3. Snowflake ID - why not UUID? (sortable, smaller, k-sorted, embeds time)
4. How does search work? (Indexer pulls from Kafka -> Elasticsearch shards)
5. How to compute trending topics? (count + decay over windows, MinHash for novelty)
6. How would you implement retweets without duplicating data?
7. How does timeline pagination work? (cursor-based with tweet ID, not offset)

---

## Part B - Uber (Ride-Hailing)

### Functional Requirements
1. Driver online/offline + location updates (every 4-5s)
2. Rider requests ride -> match nearest driver
3. Track live location during trip
4. Pricing (surge), payments, rating

### Scale
- ~10M drivers, ~100M riders
- Driver pings: 10M * 0.25/s = **2.5M location updates/sec** at peak
- Match latency: < 2s
- Geo queries: "find drivers within 5km of (lat, lng)" at 100k QPS

### Architecture

```
[Driver App] --4G/5G/WS--> [Location Ingest Svc] -> [Geo Index (in-memory, sharded)]
[Rider App]  --HTTPS-->     [Booking Svc]
                                  |
                            [Match Svc] <-> [Geo Index]
                                  |
                         [Trip Svc] -> [Cassandra/Postgres]
                                  |
                         [Pricing Svc] (surge)
                                  |
                         [Payment Svc] -> external gateway
                                  |
                         [Notification Svc]
```

### Geo-Spatial Indexing

| Approach | Description |
|----------|-------------|
| **GeoHash** | Hash 2D coords into a base32 string; prefix match -> nearby |
| **QuadTree** | Recursive 4-way space partition |
| **S2 Cells (Google)** | Hilbert curve mapping; preserves locality |
| **R-Tree** | Tree of bounding rectangles |
| **Redis GEO** | Sorted set with geohash score; built-in `GEORADIUS` |

Uber uses **H3** (hexagonal hierarchical index) - hexagons have uniform neighbors, no diagonal weirdness.

### Driver Location Pipeline
1. Driver sends (lat, lng, ts) every 4s over WebSocket
2. Ingest service writes to:
   - **Hot store**: in-memory geo index, partitioned by city/region (Redis or custom)
   - **Cold store**: Kafka -> Cassandra for historical/replay
3. Match service queries geo index for radius search

### Matching Algorithm
1. Rider requests ride at (lat, lng)
2. Match svc queries: `nearby drivers in 2km, online, available`
3. Rank by ETA + rating; offer to top driver(s)
4. Driver accepts -> trip created, both parties notified

### Surge Pricing
- Demand/supply ratio per geo-cell, recomputed per minute
- Multiplier capped (e.g., 5x), monotonic over short bursts

### Files
- [geohash.cpp](geohash.cpp) - encode/decode + nearby cells
- [driver_index.cpp](driver_index.cpp) - in-memory geo index for nearby query

### Uber Interview Questions
1. GeoHash vs QuadTree vs H3 - trade-offs?
2. How to handle 2.5M location updates/sec? (sharded ingest, regional partitioning)
3. Why not store every driver location in a SQL DB?
4. How to match the **best** driver, not just nearest? (weighted: distance + rating + acceptance)
5. How do you ensure exactly one driver gets the request? (locking, idempotent state machine)
6. Surge pricing - real-time supply/demand calc - how?
7. How to scale to a new city? (separate region cluster)
8. How to handle WebSocket scale? (sticky LB, connection-affinity, per-region clusters)

## Daily Assignment
1. Implement Snowflake ID generator (compile + ensure 64-bit + monotonic).
2. Implement geohash encode at 6-char precision; verify nearby cells share prefix.
3. Implement in-memory `DriverIndex` with `update(driverId, lat, lng)` and `nearby(lat, lng, radiusKm) -> [driverIds]`.
4. Sketch sequence diagram: rider request -> match -> driver accept -> trip start.
