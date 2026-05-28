# Day 28 - HLD: WhatsApp + YouTube

## Part A - WhatsApp (Messaging)

### Functional Requirements
1. 1:1 messaging, group chat (up to 256/1024)
2. Message states: sent, delivered, read (single, double, blue ticks)
3. Online/last-seen presence
4. Media: images, video, voice, files
5. End-to-end encryption (Signal protocol)
6. Multi-device sync

### Scale
- ~2B users, ~100B messages/day -> ~1.2M msg/sec avg, peak ~5-10M/sec
- Median message ~100 bytes; media offloaded to object store + CDN
- Mobile-first: lossy networks, battery-sensitive

### Architecture

```
[Phone] <--WebSocket/XMPP--> [Connection Server (sticky)]
                                       |
                                  [Message Router]
                                       |
            +--------------------------+----------------------+
            |                          |                      |
       [Online: direct push]      [Offline: queue]      [Group fanout]
                                       |
                                 [Message Store]
                                 (Cassandra/Mnesia)
                                       |
                                 [Push to device]
                                 (APNs / FCM)
```

### Message Flow (1:1)
1. Alice connects to Connection Server A (sticky session)
2. Bob connects to Connection Server B
3. Alice sends msg -> Server A -> Router (lookup Bob's connection server) -> Server B -> Bob
4. Server B confirms delivery -> Router -> Alice (double tick)
5. If Bob offline: queue + APNs push
6. Bob reads -> Server B -> Router -> Server A -> Alice (blue tick)

### Why XMPP / Custom over HTTP?
- Persistent connection (battery, latency)
- Server-initiated push (no polling)
- Lower bandwidth (binary frames)
- WhatsApp uses **modified XMPP** + **Noise Protocol** for encryption

### Storage
- Erlang/Mnesia historically; messages stored briefly until delivered
- Once delivered to all devices, **message is deleted from server** (privacy + cost)
- Media: S3-like store with TTL

### Group Chat
- Group state: list of members + admin
- Sender encrypts once with **group key** (Sender Keys protocol)
- Server fans out to N members (up to 1024 in WA)
- Each member's device decrypts

### Multi-Device
- Linked devices share keys via Signal protocol
- Server stores ciphertext per device until delivered

### Files
- [chat_connection_registry.cpp](chat_connection_registry.cpp) - track who's connected to which server
- [message_state_machine.cpp](message_state_machine.cpp) - sent/delivered/read states

### WhatsApp Interview Questions
1. Why WebSocket / XMPP over HTTP?
2. How to scale connection servers to 2B concurrent? (sharded by user ID, sticky LB, regional)
3. How does presence (online/last-seen) work without overloading?
4. Group of 1024: how do you fan out efficiently? (push notification batching, sender keys)
5. Why "delete message after delivery" works for them?
6. Multi-device sync - what's tricky? (cross-device key distribution)
7. How does end-to-end encryption work briefly? (X3DH + Double Ratchet)
8. Receipt updates (ticks) - how to avoid noise? (batch read receipts, only send on focus)

---

## Part B - YouTube (Video Streaming)

### Functional Requirements
1. Upload video
2. Encode to multiple resolutions/bitrates
3. Stream adaptive (HLS/DASH)
4. Search, recommendations, comments, like/subscribe
5. View counts, watch time analytics

### Scale
- ~500 hours uploaded per minute
- ~1B hours watched per day, ~5B views/day
- Ingest: ~30TB/hour video (raw)
- Edge bandwidth: dominant cost; CDN-heavy

### Upload + Encoding Pipeline

```
[Upload Client]
     |  resumable upload (chunked, ~8MB chunks)
     v
[Upload Service] -> [Object Store (raw)]
                          |
                    [Encoding Queue]
                          |
              +-----------+-----------+
              |           |           |
        [Worker 1]   [Worker 2] ...  fan-out per resolution
        (FFmpeg)
              |
        +-----+-----+--------+-------+
        v     v     v        v       v
       144p  360p  720p    1080p   4K
        |     |     |        |       |
        v     v     v        v       v
                [Object Store / CDN origin]
                          |
                  [Edge CDN POPs]
                          |
                       [Viewers]
```

### Adaptive Bitrate Streaming
- **HLS** (Apple) - `.m3u8` playlist + `.ts` segments
- **DASH** (MPEG) - `.mpd` manifest + segments
- Player switches resolution based on buffer / bandwidth

### Storage
- **Hot videos** (top 1%): replicated to many CDN edges
- **Long tail**: origin-only, fetched lazily

### View Count
- Naive `UPDATE views = views + 1` is a hot row -> ranked-DB write storm
- Solution: **stream events to Kafka** -> batch aggregate -> update Cassandra/BigTable per minute
- Spam/bot detection via signals (IP, watch %, fingerprint)

### Recommendations
- Two-stage:
  1. **Candidate generation** (collaborative filtering / embeddings) -> top 1000
  2. **Ranking** (deep model with watch time) -> top 20

### Files
- [video_upload_service.cpp](video_upload_service.cpp) - chunked upload simulation + encoding queue
- [view_counter.cpp](view_counter.cpp) - sharded counter with batch flush

### YouTube Interview Questions
1. Adaptive bitrate streaming - HLS vs DASH?
2. Why encode to multiple resolutions on server side?
3. CDN strategy - push vs pull for new videos?
4. View count at scale - hot row problem and solution?
5. Recommendations - candidate vs ranking model split, why?
6. How to detect view fraud / bots?
7. How to design comments at scale (sharding by video_id, fanout fan-in)?
8. Live streaming vs VOD - what changes? (low-latency HLS, transcoding pipeline)
9. How does resumable upload work? (chunk + offset state)

## Daily Assignment
1. Build chat connection registry: `connect(userId, serverId)`, `serverFor(userId)`, `disconnect`.
2. Implement message state machine: SENT -> DELIVERED -> READ with allowed transitions.
3. Implement chunked uploader: client uploads 5MB chunks; service tracks completion, marks ready when last chunk arrives.
4. Implement sharded view counter (16 shards) with periodic flush to "DB".
5. Sketch sequence diagram: video upload -> transcode -> CDN -> playback.
