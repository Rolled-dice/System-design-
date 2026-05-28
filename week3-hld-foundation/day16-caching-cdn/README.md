# Day 16 - Caching + CDN

## Why Cache?
Reduce latency, reduce DB load, save bandwidth. Trade-off: stale data risk.

## Caching Layers (top to bottom)
1. Browser cache
2. CDN edge cache
3. Reverse proxy (NGINX, Varnish)
4. Application-level cache (in-process LRU)
5. Distributed cache (Redis, Memcached)
6. Database query cache

## Cache Strategies
| Strategy | How it Works |
|----------|--------------|
| **Cache-Aside** (Lazy load) | App reads cache, misses -> read DB, write cache. Most common. |
| **Read-Through** | Cache fetches from DB on miss transparently |
| **Write-Through** | Write to cache and DB synchronously - consistent, slow writes |
| **Write-Behind** (Write-Back) | Write to cache, async flush to DB - fast, risk of data loss |
| **Refresh-Ahead** | Pre-fetch hot keys before expiry |

## Eviction Policies
- **LRU** - Least Recently Used (most common)
- **LFU** - Least Frequently Used
- **FIFO**
- **TTL**-based expiry
- **Random** - simple, surprisingly OK

## Cache Invalidation
- TTL-based
- Event-driven (DB triggers, change data capture)
- Versioned keys (`user:42:v3`)

## CDN
- Geographically distributed edge servers caching static + sometimes dynamic content
- **Pull CDN**: lazy fetch on first request (e.g., Cloudflare)
- **Push CDN**: pre-publish content (e.g., for video VOD)
- Cache key tuning: query strings, headers, cookies

## Files
- [lru_cache.cpp](lru_cache.cpp)
- [lfu_cache.cpp](lfu_cache.cpp)
- [cache_aside.cpp](cache_aside.cpp) - simulates DB + cache
- [ttl_cache.cpp](ttl_cache.cpp)

## Interview Questions
1. Cache-Aside vs Write-Through vs Write-Behind - trade-offs?
2. Implement LRU in O(1) - explain hashmap + doubly linked list.
3. How do you avoid thundering herd / cache stampede?
4. Cache penetration vs avalanche vs hot key - definitions and mitigations.
5. Why is invalidation hard? "There are 2 hard things in CS..."
6. CDN - how does TTL work with `Cache-Control` headers?
7. When NOT to cache?
8. Distributed cache consistency - sharding strategies?

## Daily Assignment
1. Implement LRU cache `O(1)` get/put.
2. Implement LFU cache (harder).
3. Build a `UserService` with cache-aside on top of a fake `UserDb`.
4. Add TTL expiry + background sweep to your LRU.
