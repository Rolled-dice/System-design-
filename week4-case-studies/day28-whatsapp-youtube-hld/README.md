# Day 28 - HLD: WhatsApp + YouTube

## HLD Interview Framework (Quick Reference)

Refer to [Day 26](../day26-url-shortener-hld/README.md) for the full 4-step HLD framework. Applied here:

1. **Requirements**: Real-time encrypted messaging + video upload/streaming at scale
2. **Capacity Estimation**: WhatsApp: 100B messages/day; YouTube: 500 hours uploaded/minute, 1B hours watched/day
3. **High-Level Design**: Connection-oriented messaging with offline queues; upload pipeline with CDN delivery
4. **Deep-Dive**: Signal Protocol encryption, adaptive bitrate streaming, view counting at scale

---

## Part A - WhatsApp (Messaging)

### Requirements

**Functional**:
1. 1:1 messaging and group chat (up to 1024 members)
2. Message states: sent, delivered, read (single tick, double tick, blue tick)
3. Online/last-seen presence
4. Media sharing: images, video, voice notes, documents
5. End-to-end encryption (server cannot read messages)
6. Multi-device sync

**Non-Functional**:
- ~2B users, ~100B messages/day -> ~1.2M messages/sec avg, peak ~5-10M/sec
- Median message ~100 bytes; media offloaded to object store
- Mobile-first: lossy networks, battery-sensitive
- Message delivery guarantee: at-least-once (with dedup on client)
- Privacy: server must NOT be able to read message content

### Capacity Estimation

```
Messages:
  100B messages/day / 86400 = ~1.16M msg/sec average
  Peak (New Year midnight): ~10M msg/sec
  Storage per message (metadata only, content encrypted): ~200 bytes
  Daily metadata: 100B * 200B = 20 TB/day

Media:
  ~5% of messages have media attachment
  5B media/day * avg 500KB = 2.5 PB/day (object store)
  CDN egress for media delivery: dominant cost

Connections:
  ~500M concurrent connections at peak
  Connection state per user: ~2KB (session, keys, queue pointer)
  Connection state total: 500M * 2KB = 1 TB (distributed across servers)
```

---

### End-to-End Encryption: Signal Protocol

**Why E2E encryption matters for design**: The server is a RELAY, not a reader. It cannot inspect, index, or search message content. This fundamentally shapes the architecture.

#### X3DH Key Exchange (Extended Triple Diffie-Hellman)

```
Initial key exchange between Alice and Bob (first-ever message):

Alice has:                         Bob has (pre-uploaded to server):
  - Identity Key (IK_A)             - Identity Key (IK_B)  [long-term]
  - Ephemeral Key (EK_A)            - Signed Pre-Key (SPK_B) [rotated weekly]
                                     - One-Time Pre-Key (OPK_B) [consumed once]

Key agreement (Alice initiates):
  DH1 = DH(IK_A, SPK_B)      // Alice's identity, Bob's signed pre-key
  DH2 = DH(EK_A, IK_B)       // Alice's ephemeral, Bob's identity
  DH3 = DH(EK_A, SPK_B)      // Alice's ephemeral, Bob's signed pre-key
  DH4 = DH(EK_A, OPK_B)      // Alice's ephemeral, Bob's one-time pre-key

  Shared Secret = KDF(DH1 || DH2 || DH3 || DH4)
```

**Why X3DH?**
- Enables messaging even when Bob is offline (uses pre-uploaded keys)
- Provides forward secrecy (compromising long-term keys does not reveal past messages)
- One-time pre-key ensures each initial session has unique secret

#### Double Ratchet Algorithm (Forward Secrecy)

```
After X3DH establishes initial shared secret, Double Ratchet provides:
  1. New encryption key for EVERY message (forward secrecy)
  2. Self-healing: even if one key is compromised, future messages are safe

Two ratchets:
  - Symmetric ratchet: KDF chain derives new message key from previous
    key_n+1 = KDF(key_n, "message_key")
    Each message uses a unique derived key, then key is deleted
    
  - DH ratchet: periodic new DH exchange generates fresh root key
    On each reply: sender generates new DH pair
    New root_key = KDF(old_root_key, DH(sender_new, receiver_current))
    
Result: compromise of key_n reveals only message_n, nothing before or after
```

**Why the server cannot read messages**:
- Server only sees ciphertext (encrypted with keys it never possesses)
- Keys are generated on-device via DH exchange
- Server stores and forwards encrypted blobs blindly
- Even metadata (who messages whom) is minimized (no message content logging)

---

### Message Delivery: State Machine for Receipts

```
Message lifecycle (sender perspective):

    send()           server_ack()        recipient_ack()      read_event()
+----------+  -->  +----------+  -->  +-------------+  -->  +--------+
| PENDING  |       | SENT     |       | DELIVERED   |       | READ   |
| (queued) |       | (single  |       | (double     |       | (blue  |
|          |       |  tick)   |       |  tick)      |       |  tick) |
+----------+       +----------+       +-------------+       +--------+
      |
      | network_failure
      v
+----------+
| FAILED   | -> retry with exponential backoff
+----------+
```

**State transitions**:
| From | To | Trigger | Who sends? |
|------|----|---------|------------|
| PENDING | SENT | Server acknowledges receipt of ciphertext | Server -> Sender |
| SENT | DELIVERED | Recipient's device downloads message | Recipient device -> Server -> Sender |
| DELIVERED | READ | Recipient opens chat and message is visible | Recipient device -> Server -> Sender |
| PENDING | FAILED | Network timeout after retries | Local detection |

**Offline queue with TTL**:
```
When recipient is offline:
  1. Server stores encrypted message in per-user queue (Cassandra)
  2. Queue has TTL (30 days for messages, 24 hours for media URLs)
  3. When recipient comes online: drain queue (batch delivery)
  4. After successful delivery: delete from server queue (privacy)
  5. If TTL expires: message is lost (inform sender "message expired")
```

**Message ordering**:
```
Problem: messages sent in rapid succession may arrive out of order
  (different network paths, retries, server processing delays)

Solution options:
  1. Server timestamps (simple but server can manipulate)
  2. Vector clocks (complex for 1:1 chat, overkill)
  3. Sender sequence number per conversation (chosen by WhatsApp)
     - Each device maintains a monotonic counter per chat
     - message.seq = last_seq + 1
     - Recipient reorders by sequence number
     - Gap detection: if seq 5 arrives but 4 is missing, wait briefly then request retransmit
```

---

### Group Messaging: Fan-out Strategies

**Problem**: Alice sends a message to a group of 256 members. How does the server deliver it?

**Sender fan-out (WhatsApp approach)**:
```
Alice encrypts message once with group key (Sender Keys protocol)
Alice sends ONE encrypted copy to server
Server fans out to 256 members' queues

Why Sender Keys (not individual encryption)?
  - Individual: Alice encrypts 256 times (one per member's key) -> expensive
  - Sender Keys: Alice encrypts ONCE with shared group key
  - All members hold the group's decryption key
  - Re-key needed when member leaves (forward secrecy for departing members)
```

**Server fan-out**:
```
For each member in group:
  1. Look up member's connection server
  2. If online: push immediately via WebSocket
  3. If offline: enqueue in member's offline queue
  4. Send delivery receipt back to sender (per-member)

Optimization for large groups (1024 members):
  - Batch: group delivery receipts (one "delivered to 200/256" update, not 200 individual)
  - Lazy delivery: offline members get messages on next connect (pull-based)
  - Priority: deliver to online members first, queue offline members
```

**Group key distribution**:
```
When new member joins:
  1. Admin generates new group key (or existing member shares current key)
  2. New key is encrypted with each member's individual key and distributed
  3. New member cannot read messages from before they joined (no key for old messages)

When member leaves:
  1. Generate new group key
  2. Distribute to remaining members (encrypted per-member)
  3. Departed member cannot read future messages (no new key)
  
Cost: re-key event = N encryptions (one per remaining member)
For 1024-member group: 1024 key-share messages on each membership change
```

---

### Media Handling

```
Media upload flow:
  1. Sender encrypts media with random symmetric key (AES-256)
  2. Upload encrypted blob to media server (chunked upload for large files)
  3. Media server stores in object store (S3), returns media_url
  4. Sender sends message containing:
     { media_url, encryption_key, media_type, thumbnail_hash }
  5. Recipient receives message, downloads encrypted blob from media_url
  6. Decrypts with included key, displays media

Why this design?
  - Server stores encrypted blobs (cannot view media)
  - Media URL can be CDN-cached (encrypted blob is opaque to CDN)
  - Thumbnail generated client-side, encrypted, sent inline (low-res preview)
  - Media has separate TTL from message metadata (storage cost management)
```

**Thumbnail generation**:
```
Client-side (sender):
  1. Compress image to 100x100 thumbnail (~5KB)
  2. Encrypt thumbnail with message key
  3. Include encrypted thumbnail IN the message body (inline, not separate upload)

Recipient displays thumbnail immediately while full media downloads in background.
This avoids a second network roundtrip for the preview.
```

---

### Connection Management

**Protocol choice**: WhatsApp uses a modified XMPP protocol over TLS, with custom binary framing for efficiency. For new systems, WebSocket or MQTT are common choices.

```
+------------------------------------------------------------+
| Protocol   | Connection | Message Size | Battery | Use Case |
+------------+------------+--------------+---------+----------|
| HTTP/2     | Persistent | Large header | Medium  | Web apps |
| WebSocket  | Persistent | Minimal (2B) | Good    | Real-time|
| MQTT       | Persistent | Minimal      | Best    | IoT/Chat |
| XMPP       | Persistent | XML (verbose)| OK      | Legacy   |
| Custom     | Persistent | Binary/minimal| Best   | WhatsApp |
+------------------------------------------------------------+
```

**Push for offline devices**:
```
When user is offline (no active connection):
  - iOS: APNs (Apple Push Notification Service)
  - Android: FCM (Firebase Cloud Messaging)
  
Push contains:
  - NOT the message content (E2E encryption means server cannot include it)
  - Only: "You have a new message from Alice" (generic)
  - Device wakes, connects to server, pulls encrypted messages from queue
```

---

### Presence System (Online/Last-Seen)

**Problem**: Show "online" or "last seen at 3:42 PM" for contacts.

**Implementation**:
```
Heartbeat-based:
  - Client sends heartbeat every 30 seconds while app is in foreground
  - Server updates last_heartbeat timestamp per user
  - "Online" = last_heartbeat < 30 seconds ago
  - "Last seen" = last_heartbeat timestamp (rounded to minutes)

Privacy controls:
  - User can set: "Show to everyone" / "Show to contacts" / "Show to nobody"
  - If hidden: server stores presence but does not share it
  - Read receipts (blue ticks) can also be disabled per-user
```

**Scaling presence**:
```
Naive: every contact checks every contact's presence -> O(N^2) queries
Better: publish-subscribe per user
  - When Alice comes online: notify Alice's online contacts
  - Subscription limited to contacts currently in the app (foreground)
  - Users in background do not receive real-time presence updates
  - On app open: batch-fetch presence for visible contacts

Scale: 500M concurrent * 200 avg contacts = 100B subscriptions (impossible to maintain all)
Practical: only subscribe to contacts whose chat is OPEN (max 5-10 per user)
```

---

### WhatsApp Architecture Summary

```
[Phone] <--WebSocket/MQTT--> [Connection Server (sticky session)]
                                      |
                               [Message Router]
                                      |
           +--------------------------+----------------------+
           |                          |                      |
      [Online: push]           [Offline: queue]        [Group fanout]
      via WebSocket            (Cassandra, TTL)        (fan to members)
                                      |
                               [Media Server]
                               (Upload/Download)
                                      |
                               [Object Store (S3)]
                               (Encrypted blobs)
                                      |
                               [CDN] (media delivery)

[Push Service: APNs/FCM] <-- offline notification
[Presence Service] <-- heartbeat tracking
[Key Server] <-- pre-key bundles for X3DH
```

---

## Part B - YouTube (Video Streaming)

### Requirements

**Functional**:
1. Upload video (any format, any resolution, up to 12 hours)
2. Transcode to multiple resolutions and codecs
3. Stream with adaptive bitrate (HLS/DASH)
4. Search, recommendations, comments, likes, subscriptions
5. View counts, watch time analytics
6. Live streaming support

**Non-Functional**:
- ~500 hours of video uploaded per minute
- ~1B hours watched per day, ~5B video views/day
- Ingest: ~30 TB/hour of raw video
- Dominant cost: CDN bandwidth for video delivery
- Transcoding latency: < 30 minutes for a 10-minute video (user expects video available quickly)

### Capacity Estimation

```
Upload:
  500 hours/min * 60 min = 30,000 hours/day uploaded
  Average raw video: 5 GB/hour -> 150 TB/day raw ingest
  After transcoding (multiple resolutions): ~3x raw = 450 TB/day total storage

Streaming:
  1B hours watched/day
  Average bitrate: 5 Mbps (1080p)
  Peak concurrent viewers: ~100M simultaneous
  Peak bandwidth: 100M * 5 Mbps = 500 Pbps (served from CDN, not origin)

Storage (growing):
  450 TB/day * 365 = ~164 PB/year
  With dedup and cold storage tiering: ~100 PB/year net growth

View counting:
  5B views/day / 86400 = ~58,000 views/sec
  Peak: ~500,000 view events/sec (viral video + global events)
```

---

### Upload Pipeline: From Raw Video to Playable Content

```
[Creator's Device]
       |
       | Resumable chunked upload (8MB chunks)
       v
[Upload Service]
       |
       | Validate: format, size, virus scan
       | Store raw file in object store
       v
[Object Store (S3)] -- raw video stored
       |
       | Publish event: "new video ready for processing"
       v
[Transcoding Queue (Kafka/SQS)]
       |
       | Fan-out to multiple workers (one per output format)
       v
[Transcoding Workers (FFmpeg clusters)]
       |
       +-- 144p  (250 Kbps)  -> Object Store
       +-- 360p  (800 Kbps)  -> Object Store
       +-- 720p  (2.5 Mbps)  -> Object Store
       +-- 1080p (5 Mbps)    -> Object Store
       +-- 4K    (20 Mbps)   -> Object Store
       |
       | Each resolution also segmented into 2-10 second chunks
       | Generate manifest file (.m3u8 for HLS, .mpd for DASH)
       v
[CDN Origin]
       |
       | Propagate to edge POPs on first request (pull-based)
       v
[CDN Edge POPs worldwide]
       |
       v
[Viewers]
```

**Resumable upload (why chunked?)**:
```
Problem: 2-hour 4K video = ~50 GB. Single upload would fail on any network interruption.
Solution: Split into 8MB chunks, upload each with offset tracking.

Client:
  1. Initiate upload: POST /upload/init -> returns upload_id
  2. Upload chunks: PUT /upload/{upload_id}/chunk?offset=0 (8MB)
                    PUT /upload/{upload_id}/chunk?offset=8388608 (8MB)
                    ...
  3. On network failure: resume from last acknowledged offset
  4. Finalize: POST /upload/{upload_id}/complete

Server tracks: which chunks received, detects missing ranges, confirms completion.
```

---

### Adaptive Bitrate Streaming: HLS and DASH Explained

**The problem**: Viewers have different bandwidths (4G vs fiber) and devices (phone vs 4K TV). A single bitrate wastes bandwidth on fast connections and buffers on slow ones.

**Solution**: Encode at multiple bitrates. Player switches between them dynamically.

**HLS (HTTP Live Streaming - Apple)**:
```
Master playlist (.m3u8):
  #EXTM3U
  #EXT-X-STREAM-INF:BANDWIDTH=250000,RESOLUTION=426x240
  240p/playlist.m3u8
  #EXT-X-STREAM-INF:BANDWIDTH=800000,RESOLUTION=640x360
  360p/playlist.m3u8
  #EXT-X-STREAM-INF:BANDWIDTH=2500000,RESOLUTION=1280x720
  720p/playlist.m3u8
  #EXT-X-STREAM-INF:BANDWIDTH=5000000,RESOLUTION=1920x1080
  1080p/playlist.m3u8

Per-resolution playlist (720p/playlist.m3u8):
  #EXTM3U
  #EXT-X-TARGETDURATION:10
  #EXTINF:10.0,
  segment_001.ts
  #EXTINF:10.0,
  segment_002.ts
  ...

Player behavior:
  1. Fetch master playlist
  2. Estimate bandwidth (measure download speed of first segment)
  3. Select highest quality that fits in estimated bandwidth
  4. Continuously monitor: if buffer drops below 5s, switch to lower quality
  5. If buffer exceeds 15s, try upgrading to higher quality
```

**DASH (Dynamic Adaptive Streaming over HTTP - MPEG standard)**:
```
Similar concept to HLS but uses:
  - .mpd manifest (XML) instead of .m3u8
  - Generic segment format (not Apple-specific .ts)
  - More flexible: segment timeline, multiple periods, content protection

Comparison:
  HLS: Apple devices native, widely supported, slightly simpler
  DASH: Open standard, DRM integration (Widevine, PlayReady), more flexible
  YouTube uses BOTH: HLS for Apple devices, DASH for everything else
```

---

### Recommendation System Architecture

**Two-stage approach** (used by YouTube, Netflix, and most large-scale recommenders):

```
Stage 1: Candidate Generation (retrieve ~1000 from millions)
  Input: user history, demographics, context (time, device)
  Models:
    - Collaborative filtering: "Users similar to you watched X"
    - Content-based: "Videos similar to ones you watched"
    - Trending/popular: "What's popular in your region now"
  Output: ~1000 candidate video IDs
  Latency budget: < 50ms (approximate nearest neighbor search)

Stage 2: Ranking (score and sort 1000 -> top 20)
  Input: candidate videos + user features + context
  Model: Deep neural network predicting:
    - P(click): will user click this video?
    - P(watch): will user watch > 50%?
    - E(watch_time): expected minutes watched
  Objective: maximize E(watch_time) (YouTube's primary metric)
  Output: ranked list of 20 videos for the feed
  Latency budget: < 100ms

Total: < 150ms end-to-end for personalized recommendations
```

**Why two stages?**
- Scoring all 800M videos with a deep model is too expensive (~100ms per video)
- Candidate generation uses cheap approximate methods to narrow to 1000
- Deep ranking model only runs on 1000 candidates (feasible in 100ms)

**Cold start problem**:
```
New user (no history):
  - Use demographic signals (age, country, language)
  - Show popular/trending content
  - Rapidly adapt after 5-10 interactions

New video (no views):
  - Use content signals (title, description, thumbnail, creator history)
  - Bootstrap with creator's existing audience
  - Explore/exploit: show to small audience, measure engagement, expand if good
```

---

### View Counting at Scale

**The problem**: A viral video gets 1M views per minute. Naive `UPDATE views SET count = count + 1` on every view creates a hot row that overwhelms any single database.

**Solution: Multi-layer aggregation**:

```
Layer 1: Client-side batching
  - Client buffers view events, sends batch every 30 seconds
  - Reduces request volume by ~30x

Layer 2: Stream processing
  [View Events] -> [Kafka Topic: video_views]
                         |
                   [Flink/Spark Streaming]
                         |
                   Aggregate per video per 1-minute window
                         |
                   [Write batch: video_123: +50,000 views]
                         |
                   [Cassandra/BigTable]

Layer 3: Approximate real-time display
  - Display "1.2M views" (not exact 1,234,567)
  - Update displayed count every 30 seconds
  - Exact count computed hourly for creator analytics
```

**HyperLogLog for unique viewers**:
```
Problem: "1M views" might be 100 users refreshing 10,000 times. Need UNIQUE view count.

HyperLogLog:
  - Probabilistic data structure for cardinality estimation
  - Fixed 12 KB memory regardless of cardinality
  - ~0.81% standard error (close enough for display)
  - Operations: add(viewer_id), count() -> approximate unique count

Per-video HyperLogLog:
  - On each view event: HLL.add(hash(user_id + video_id))
  - Unique viewer count: HLL.count()
  - Memory: 12 KB per video * 800M videos = ~9.6 TB total
  - Alternative: only maintain HLL for videos with > 10K views (most are long-tail)

Exact dedup alternative:
  - Redis SET per video with all viewer IDs
  - 1M unique viewers * 8 bytes = 8 MB per popular video
  - Not feasible for 800M videos; only for top 0.01% viral videos
```

---

### CDN Strategy for Video Delivery

**Problem**: 100M concurrent viewers worldwide. Origin server cannot handle this bandwidth. CDN distributes load to edge servers.

```
Content placement strategy:

Tier 1: Edge POP (closest to user, ~300 locations)
  - Cache hot segments (currently being watched)
  - Eviction: LRU with frequency weighting
  - Hit rate: ~90% for popular content

Tier 2: Regional hub (~30 locations)
  - Cache all content accessed in region in last 7 days
  - Feeds multiple edge POPs
  - Hit rate for long-tail: ~95%

Tier 3: Origin (2-3 locations)
  - Complete video archive
  - Only serves cache misses from regional hubs
  - Handles < 5% of total read traffic
```

**Popularity-based pre-positioning**:
```
For viral/trending content:
  1. Detect virality early (view velocity > threshold)
  2. Pre-push segments to ALL edge POPs (don't wait for cache miss)
  3. Reduces first-view latency for viral content

For new uploads by popular creators:
  1. Creator has 50M subscribers
  2. On upload complete: pre-push to top 20 geographic edge POPs
  3. Subscribers likely to watch within first hour -> content already at edge
```

---

### Live Streaming Architecture

```
[Streamer] --RTMP/SRT--> [Ingest Server]
                               |
                         [Transcode (real-time)]
                         - Multiple resolutions
                         - < 2 second processing delay
                               |
                         [Segmenter]
                         - Produce 2-second HLS/DASH segments
                         - Update manifest every 2 seconds
                               |
                         [CDN Origin]
                               |
                         [CDN Edge POPs]
                               |
                         [Viewers (100K+ concurrent)]
```

**Key differences from VOD (Video on Demand)**:
```
+--------------------------------------------------------------+
| Aspect          | VOD                   | Live               |
+-----------------+-----------------------+--------------------+
| Latency         | N/A (pre-encoded)     | 2-30 seconds       |
| Segment size    | 6-10 seconds          | 2-4 seconds        |
| Transcoding     | Offline (parallel)    | Real-time (serial) |
| CDN caching     | Long TTL (days)       | Short TTL (seconds)|
| Manifest        | Static file           | Updated every 2s   |
| Failure impact  | Retry/buffer          | Visible stutter    |
+-----------------+-----------------------+--------------------+
```

**Ultra-low-latency live (< 3 seconds)**:
- Use CMAF (Common Media Application Format) with chunked transfer
- Segments as small as 0.5 seconds
- Trade-off: smaller segments = lower latency but more HTTP requests, less efficient caching

---

### Content Moderation Pipeline

```
[Upload Complete]
       |
       v
[Moderation Queue]
       |
       +-- [Automated ML Pipeline]
       |     |
       |     +-- Video frame sampling (1 frame/second)
       |     +-- Image classification (NSFW, violence, copyright)
       |     +-- Audio transcription + hate speech detection
       |     +-- Known-content matching (hash-based, for CSAM/copyright)
       |     |
       |     +-- Decision:
       |           PASS (95% of uploads) -> publish immediately
       |           BLOCK (2% - clear violation) -> reject + notify creator
       |           REVIEW (3% - uncertain) -> human review queue
       |
       +-- [Human Review Queue]
             |
             +-- Priority: high-view-count first, reported content first
             +-- SLA: < 24 hours for review queue items
             +-- Decision: approve / age-restrict / remove + strike
```

**Why ML first, humans second?**
- 500 hours uploaded per minute = impossible for humans to review all
- ML handles 95% confidently (clear pass or clear violation)
- Humans focus on the 3% ambiguous cases where context matters
- Feedback loop: human decisions retrain ML model weekly

---

### YouTube Architecture Summary

```
[Creator] -> [Upload Service] -> [Object Store (raw)]
                                       |
                                 [Transcoding Pipeline]
                                       |
                                 [Object Store (encoded)] -> [CDN Origin] -> [CDN Edge]
                                                                                |
[Viewer] -> [API Gateway] -> [Video Service] -> [Manifest + Segments from CDN] -> [Player]
                  |                |
            [Search Svc]    [Recommendation Svc]
            (Elasticsearch)  (ML Model Serving)
                  |                |
            [View Counter]   [Analytics Pipeline]
            (Kafka -> Flink)  (BigQuery)
                  |
            [Content Moderation]
            (ML + Human Review)
```

---

## Files
- [chat_connection_registry.cpp](chat_connection_registry.cpp) - Track who is connected to which server
- [message_state_machine.cpp](message_state_machine.cpp) - Sent/delivered/read state transitions
- [video_upload_service.cpp](video_upload_service.cpp) - Chunked upload simulation + encoding queue
- [view_counter.cpp](view_counter.cpp) - Sharded counter with batch flush

## Interview Questions

### WhatsApp
1. Why WebSocket/MQTT over HTTP? (Persistent connection, server push, lower battery, lower bandwidth)
2. How to scale connection servers to 2B concurrent? (Sharded by user ID, sticky LB, regional clusters)
3. How does presence (online/last-seen) work without overloading? (Heartbeat, subscribe only to open chats)
4. Group of 1024: how do you fan out efficiently? (Sender Keys encryption, server-side fan-out to queues)
5. Why "delete message after delivery" works for WhatsApp? (Privacy + storage cost; messages are on device)
6. Multi-device sync - what is tricky? (Key distribution per device, message ordering across devices)
7. How does E2E encryption work? (X3DH for key exchange, Double Ratchet for per-message keys)
8. Receipt updates (ticks) - how to avoid noise? (Batch read receipts, only send when chat is focused)
9. How to handle offline messages? (Per-user queue with TTL, drain on reconnect, push notification to wake)
10. How to prevent message loss on network partition? (At-least-once delivery, client-side dedup by message ID)

### YouTube
1. Adaptive bitrate streaming - HLS vs DASH? (HLS for Apple, DASH open standard; YouTube serves both)
2. Why encode to multiple resolutions on server side? (Client cannot transcode; bandwidth varies per viewer)
3. CDN strategy - push vs pull for new videos? (Pull by default, push for predicted viral content)
4. View count at scale - hot row problem and solution? (Kafka -> batch aggregate -> write once per minute)
5. Recommendations - candidate vs ranking model split, why? (Cost: cannot run deep model on 800M videos)
6. How to detect view fraud/bots? (Watch duration < 5s, same IP burst, fingerprint analysis, HyperLogLog for unique)
7. How to design comments at scale? (Shard by video_id, tree structure for replies, separate count cache)
8. Live streaming vs VOD - what changes? (Real-time transcode, short segments, frequent manifest updates)
9. How does resumable upload work? (Chunked upload with server-tracked offsets, resume from last ack)
10. Content moderation at upload-time? (ML pipeline: frame sampling + classification, human review for edge cases)

## Daily Assignment
1. Build chat connection registry: `connect(userId, serverId)`, `serverFor(userId)`, `disconnect`.
2. Implement message state machine: SENT -> DELIVERED -> READ with allowed transitions only.
3. Implement chunked uploader: client uploads 5MB chunks; service tracks completion, marks ready when last chunk arrives.
4. Implement sharded view counter (16 shards) with periodic flush to "DB" (simulated map).
5. Sketch sequence diagrams: (a) WhatsApp message delivery (online + offline), (b) YouTube video upload to playback.
