# Extended Question Bank

Additional questions sourced from publicly-available interview question banks (see [resources.md](resources.md) for source repos). Content was rephrased for compliance with licensing restrictions of source material.

## Additional LLD Problems (by category)

### Concurrency-heavy
1. Design a thread-safe in-memory cache with TTL + LRU eviction
2. Design a thread pool (fixed-size, dynamic, scheduled)
3. Design a producer-consumer with bounded blocking queue
4. Design a `ReadWriteLock` (multiple readers, single writer)
5. Design a `CountDownLatch` and `CyclicBarrier`
6. Design a rate-limiter library (token bucket + sliding window)
7. Design a leader election library (Bully or Raft basics)
8. Design a distributed lock (single node Redis emulation)

### Real-world product LLDs
9. Design Inventory Management for an e-commerce site
10. Design a Coupon/Discount system (stackable rules, expiry)
11. Design a Loyalty / Rewards points system
12. Design a Payment processing system (gateways, retries, refunds)
13. Design a Survey/Polling system (multiple-choice, NPS)
14. Design a Quiz / Online assessment platform
15. Design a Pizza ordering system
16. Design a Ride-share carpool matching (within one server)
17. Design Kindle / e-book reader (downloads, offline reading, sync progress)
18. Design a Job board / Indeed clone
19. Design a Music player with playlists, queue, shuffle, repeat
20. Design Auction system with bidding + auto-bid
21. Design a Whiteboard collaboration tool
22. Design a Document editor with collaborative editing (CRDT-light)
23. Design a Restaurant table reservation system
24. Design a Gym / Class booking system
25. Design a Medication reminder / pill tracker
26. Design Trello-like Kanban board
27. Design a Habit tracker
28. Design a LinkedIn "Skill Endorsement" + "Recommendation" system

### Data structure heavy
29. Design Trie + Autocomplete with top-K
30. Design a Skip List
31. Design a Bloom Filter
32. Design a Segment Tree library
33. Design Disjoint Set Union (Union-Find)
34. Design an LRU cache that supports `get`, `put`, `delete`, `expire`
35. Design a circular buffer / ring buffer
36. Design a priority queue with `decreaseKey`

## Additional HLD Problems

### FAANG/Top-tier favorites
1. Design Google Drive / Dropbox sync
2. Design Google Search (very ambitious - focus on one component: indexer, ranking, query)
3. Design Google Calendar (recurrence, reminders, free/busy, sharing)
4. Design Google Docs (operational transform vs CRDT, presence)
5. Design Google Maps (tile server, routing, real-time traffic)
6. Design Gmail (storage, search, labels, spam filter integration)
7. Design Amazon S3 (object storage, durability 11 nines, multipart upload)
8. Design Amazon shopping cart + checkout (inventory locking, recommendations)
9. Design Amazon Prime Video / Netflix (CDN, encoding, recommendations)
10. Design Facebook Photo Tagging
11. Design Stripe / Razorpay payment processing
12. Design Spotify / Apple Music (ranking, recommendations, offline mode)
13. Design Airbnb (search, booking, reviews, calendar)
14. Design DoorDash / UberEats
15. Design TikTok / Reels (video upload, recommendation, view counter)
16. Design LinkedIn (graph, news feed, search)
17. Design Reddit / HackerNews (upvotes, ranking, moderation)
18. Design Pinterest (visual feed, image storage)
19. Design Tinder (swipes, matching, geo)
20. Design Slack / Discord (channels, real-time messaging)
21. Design Zoom / Google Meet (signaling, media servers, SFU vs MCU)
22. Design Stack Overflow (search, voting, reputation)
23. Design Quora (Q&A, follows, feed)
24. Design Medium (publishing, follows, recommendations)

### Infrastructure-style
25. Design a Distributed cron / scheduler
26. Design a Distributed counter / analytics pipeline
27. Design a Distributed key-value store (Dynamo-style)
28. Design a Distributed lock manager
29. Design a Configuration service (etcd-like)
30. Design a Service Discovery system
31. Design a Distributed Tracing system (Jaeger/Zipkin)
32. Design a Log aggregation pipeline (ELK-like)
33. Design a Metrics + Monitoring system (Prometheus-like)
34. Design a Feature flag / A-B testing platform
35. Design a CI/CD pipeline orchestrator
36. Design a Distributed cache (Memcached-like)
37. Design a Container orchestrator (mini Kubernetes)
38. Design a Mesh network for IoT devices
39. Design a Pub-Sub at scale (Kafka clone)
40. Design a CDN

### Niche / domain-specific
41. Design Stock trading order book (low-latency matching engine)
42. Design Crypto wallet + transaction broadcast
43. Design Online judge (sandboxed code execution at scale)
44. Design Ad serving / bidding (RTB)
45. Design a Recommendation system (collaborative + content-based)
46. Design a Fraud detection pipeline
47. Design a Search autocomplete + spelling correction
48. Design a Translation API (queueing + caching)
49. Design IoT sensor data ingestion + analytics
50. Design a multiplayer real-time game backend (FPS or MMO)

## Company-Specific Patterns

| Company | Asks Often |
|---------|-----------|
| **Google** | Distributed systems internals, MapReduce-style, scale to billions |
| **Amazon** | Capacity estimation, leadership principles in design choices, cost |
| **Meta** | Feed/timeline, social graph, photo storage |
| **Apple** | Privacy + device sync, edge compute |
| **Microsoft** | Office collaboration, Outlook/Teams scale |
| **Uber/Lyft** | Real-time location, matching, pricing |
| **Airbnb** | Search, booking, recommendations |
| **Netflix** | CDN, streaming, A/B testing |
| **Stripe** | Idempotency, exactly-once, financial correctness |
| **Indian unicorns (Flipkart, Swiggy, Razorpay, Zepto)** | LLD-heavy machine coding rounds (parking lot, splitwise, food order, library) |

## Practice Cadence Suggestion

After completing this repo's 28 days:

- **Days 29-35:** pick 1 HLD per day from "FAANG/Top-tier favorites" - 45-min whiteboard
- **Days 36-42:** pick 1 LLD per day from "Real-world product LLDs" - 45-min C++ machine coding
- **Days 43+:** mock interviews + targeted weak-area review

Aim for **30+ HLDs and 30+ LLDs** practiced before high-stakes interviews.
