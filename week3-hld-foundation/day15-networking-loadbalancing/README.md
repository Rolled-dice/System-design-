# Day 15 - Networking + Load Balancing

## Table of Contents
- [Networking Fundamentals](#networking-fundamentals)
- [TCP/IP Deep Dive](#tcpip-deep-dive)
- [TLS 1.3 Handshake](#tls-13-handshake)
- [HTTP Evolution](#http-evolution)
- [DNS Resolution](#dns-resolution)
- [WebSocket Protocol](#websocket-protocol)
- [Load Balancing](#load-balancing)
- [Load Balancing Algorithms](#load-balancing-algorithms)
- [Health Checks](#health-checks)
- [Zero-Downtime Deployments](#zero-downtime-deployments)
- [Files](#files)
- [Interview Questions](#interview-questions)
- [Daily Assignment](#daily-assignment)

---

## Networking Fundamentals

### OSI Model vs TCP/IP Model - Why Two Models Exist

The OSI (Open Systems Interconnection) model is a conceptual framework with 7 layers, while TCP/IP is the practical implementation with 4 layers. Understanding both matters because interview questions reference OSI layers, but real systems use TCP/IP.

```
OSI 7-Layer Model              TCP/IP 4-Layer Model
+------------------+           +------------------+
| 7. Application   |           |                  |
| 6. Presentation  | --------> | 4. Application   |
| 5. Session       |           |                  |
+------------------+           +------------------+
| 4. Transport     | --------> | 3. Transport     |
+------------------+           +------------------+
| 3. Network       | --------> | 2. Internet      |
+------------------+           +------------------+
| 2. Data Link     |           |                  |
| 1. Physical      | --------> | 1. Network Access|
+------------------+           +------------------+
```

**Why TCP/IP won**: OSI was designed by committee (ISO) as a theoretical model. TCP/IP was built pragmatically by researchers who needed it to work. The "rough consensus and running code" philosophy of the IETF meant TCP/IP was battle-tested before OSI was finalized.

**Key differences in practice**:
- OSI separates presentation (encryption, compression) and session (connection management) from application. In TCP/IP, TLS handles encryption at a layer between transport and application, and HTTP manages sessions within the application layer.
- OSI Layer 2 (Data Link) handles framing, error detection (CRC), and MAC addressing. TCP/IP bundles this with physical as "network access."

### HTTP Status Codes (Must Know)
| Category | Codes | Meaning |
|----------|-------|---------|
| 2xx Success | 200, 201, 204 | OK, Created, No Content |
| 3xx Redirect | 301, 302, 304 | Moved Permanently, Found, Not Modified |
| 4xx Client Error | 400, 401, 403, 404, 409, 429 | Bad Request, Unauthorized, Forbidden, Not Found, Conflict, Too Many Requests |
| 5xx Server Error | 500, 502, 503, 504 | Internal Error, Bad Gateway, Service Unavailable, Gateway Timeout |

---

## TCP/IP Deep Dive

### TCP 3-Way Handshake - Step by Step

The handshake establishes a reliable connection by synchronizing sequence numbers between client and server.

```
    Client                              Server
      |                                   |
      |  1. SYN (seq=x)                  |
      |---------------------------------->|  Client: SYN_SENT
      |                                   |  Server: SYN_RECEIVED
      |  2. SYN+ACK (seq=y, ack=x+1)     |
      |<----------------------------------|
      |                                   |
      |  3. ACK (seq=x+1, ack=y+1)       |
      |---------------------------------->|  Both: ESTABLISHED
      |                                   |
```

**TCP State Diagram (Connection Lifecycle)**:
```
                              +--------+
                   +--------->| CLOSED |<---------+
                   |          +--------+          |
              (timeout)           |           (close)
                   |         (connect)            |
                   |              |                |
              +---------+        v          +-----------+
              |TIME_WAIT|    +--------+     | LAST_ACK  |
              +---------+    |SYN_SENT|     +-----------+
                   ^         +--------+          ^
                   |              |               |
              (FIN+ACK)     (SYN+ACK)        (FIN)
                   |              |               |
              +---------+        v          +-----------+
              | FIN_WAIT|  +-----------+    | CLOSE_WAIT|
              |    _2   |  |ESTABLISHED|    +-----------+
              +---------+  +-----------+         ^
                   ^              |               |
                   |           (close)         (FIN)
               (ACK)              |               |
                   |              v               |
              +---------+   +---------+          |
              | FIN_WAIT|   |FIN_WAIT |----------+
              |    _1   |   |   _1    |
              +---------+   +---------+
```

**Why 3 handshakes, not 2?** Two handshakes cannot guarantee both sides know the connection is established. Without the third ACK, the server allocates resources for connections the client may never use (SYN flood attack exploits this).

**SYN Flood Mitigation**: SYN cookies encode connection state in the sequence number itself, so the server does not need to store state until the handshake completes.

### TCP Congestion Control

TCP dynamically adjusts sending rate to avoid overwhelming the network. The congestion window (cwnd) determines how many bytes can be in-flight.

**Phase 1 - Slow Start**:
- Initial cwnd = 1 MSS (Maximum Segment Size, typically 1460 bytes)
- After each ACK: cwnd = cwnd + 1 MSS (exponential growth: doubles every RTT)
- Continues until cwnd reaches ssthresh (slow start threshold)

**Phase 2 - Congestion Avoidance (AIMD)**:
- Additive Increase: cwnd = cwnd + (MSS * MSS / cwnd) per ACK (linear growth, ~1 MSS per RTT)
- Multiplicative Decrease: on packet loss, cwnd = cwnd / 2, ssthresh = cwnd

```
cwnd
 ^
 |          * (packet loss!)
 |        *   \
 |      *      \  MD: cwnd /= 2
 |    *         *
 |  *         *   <- AI: linear increase
 | *        *
 |*       *
 +*-----*---------> time
 Slow   Congestion
 Start  Avoidance
```

**Why AIMD works**: Additive increase gently probes for available bandwidth. Multiplicative decrease rapidly backs off, preventing collapse. This creates a sawtooth pattern that converges to fairness among competing flows.

**Modern variants**:
- **TCP Cubic** (Linux default): Uses a cubic function of time since last loss; more aggressive recovery
- **BBR** (Google): Measures bottleneck bandwidth and RTT directly; avoids loss-based signals

### TCP vs UDP - When to Use Each

| Property | TCP | UDP |
|----------|-----|-----|
| Reliability | Guaranteed delivery (retransmits) | No guarantee |
| Ordering | In-order delivery | No ordering |
| Connection | Connection-oriented (handshake) | Connectionless |
| Overhead | 20-byte header + state | 8-byte header |
| Flow control | Yes (window-based) | No |
| Use cases | HTTP, databases, file transfer | DNS, video, gaming, QUIC |

---

## TLS 1.3 Handshake

TLS 1.3 reduced the handshake from 2-RTT (TLS 1.2) to 1-RTT, and supports 0-RTT resumption.

```
    Client                                     Server
      |                                          |
      |  ClientHello                             |
      |  + supported_versions                    |
      |  + key_share (ECDHE public key)          |
      |  + signature_algorithms                  |
      |  + psk_key_exchange_modes                |
      |----------------------------------------->|
      |                                          |
      |  ServerHello                             |
      |  + key_share (server ECDHE public key)   |
      |  {EncryptedExtensions}                   |
      |  {Certificate}                           |
      |  {CertificateVerify}                     |
      |  {Finished}                              |
      |<-----------------------------------------|
      |                                          |
      |  {Finished}                              |
      |----------------------------------------->|
      |                                          |
      |  [Application Data]  <================>  |
      |                                          |
```

**Key improvements over TLS 1.2**:
1. **1-RTT handshake**: Client sends key_share in ClientHello (pre-guesses server's preferred curve)
2. **Removed insecure ciphers**: No RSA key exchange, no CBC mode, no SHA-1
3. **Forward secrecy mandatory**: Only ECDHE and DHE key exchanges allowed
4. **0-RTT resumption**: Client can send data in first flight using pre-shared key (PSK) from previous connection. Tradeoff: vulnerable to replay attacks.

**Certificate verification flow**:
1. Server sends certificate chain (leaf -> intermediate -> root)
2. Client validates: signature chain, expiry dates, revocation (OCSP/CRL)
3. Server proves key ownership via CertificateVerify (signs handshake transcript)

---

## HTTP Evolution

### HTTP/1.1 - The Baseline
- One request/response per TCP connection (pipelining exists but poorly supported)
- Head-of-line (HOL) blocking: second request waits for first response
- Workaround: browsers open 6 parallel TCP connections per origin

```
HTTP/1.1 Head-of-Line Blocking:

Connection 1:  |--Req1--|--Resp1--|--Req2--|--Resp2--|
Connection 2:  |--Req3--|--Resp3--|--Req4--|--Resp4--|
                                    ^ Req4 blocked by Resp3
```

### HTTP/2 - Binary Multiplexing
- Single TCP connection, multiple streams (multiplexed)
- Binary framing layer (not text)
- Header compression (HPACK)
- Server push (deprecated in most browsers)

```
HTTP/2 Multiplexing (single TCP connection):

Stream 1:  |--Frame--|      |--Frame--|--Frame--|
Stream 2:       |--Frame--|--Frame--|
Stream 3:  |--Frame--|--Frame--|
           ======================================
                    Single TCP Connection

Frames from different streams interleave freely.
No application-layer HOL blocking!
```

**But TCP-level HOL blocking remains**: If a single TCP packet is lost, ALL streams stall waiting for retransmission. This is the fundamental problem HTTP/3 solves.

### HTTP/3 - QUIC (UDP-Based)
- Built on QUIC protocol (runs over UDP)
- Per-stream loss recovery: lost packet only blocks its stream
- 0-RTT connection establishment (combines transport + crypto handshake)
- Connection migration (change IP/port without re-handshake, useful for mobile)

```
HTTP/3 (QUIC) - Independent Streams:

Stream 1:  |--Pkt--|  X  |--Retransmit--|--Pkt--|
Stream 2:       |--Pkt--|--Pkt--|--Pkt--|           <- NOT blocked!
Stream 3:  |--Pkt--|--Pkt--|--Pkt--|
           ==========================================
                    UDP (no TCP HOL blocking)
```

**QUIC handshake (0-RTT possible)**:
```
Client                          Server
  |  Initial (crypto)            |
  |----------------------------->|  1-RTT: Combined transport
  |  Handshake + 0-RTT data     |         + TLS handshake
  |<-----------------------------|
  |  Handshake Complete          |
  |----------------------------->|
  |  [Application Data]         |
```

### Comparison Table

| Feature | HTTP/1.1 | HTTP/2 | HTTP/3 |
|---------|----------|--------|--------|
| Transport | TCP | TCP | QUIC (UDP) |
| Multiplexing | No (6 connections) | Yes (streams) | Yes (independent streams) |
| HOL Blocking | Application + TCP | TCP only | None |
| Header compression | None | HPACK | QPACK |
| Connection setup | 1-RTT TCP + 2-RTT TLS | Same | 0-1 RTT |
| Connection migration | No | No | Yes |

---

## DNS Resolution

### Full Resolution Path

```
Client                    Recursive        Root NS         TLD NS       Authoritative NS
  |                       Resolver         (.)             (.com)       (example.com)
  |                          |                |               |               |
  | 1. "example.com?"       |                |               |               |
  |------------------------->|                |               |               |
  |                          | 2. "."?        |               |               |
  |                          |--------------->|               |               |
  |                          | 3. ".com NS"   |               |               |
  |                          |<---------------|               |               |
  |                          | 4. ".com?"                     |               |
  |                          |------------------------------->|               |
  |                          | 5. "example.com NS"           |               |
  |                          |<-------------------------------|               |
  |                          | 6. "example.com A?"                            |
  |                          |--------------------------------------------- ->|
  |                          | 7. "93.184.216.34"                             |
  |                          |<-----------------------------------------------|
  | 8. "93.184.216.34"       |                                                |
  |<-------------------------|                                                |
  |                          |                                                |
```

**Caching at each level (TTL-based)**:
1. **Browser cache**: Chrome caches DNS for 60s by default
2. **OS resolver cache**: systemd-resolved, nscd
3. **Recursive resolver cache**: ISP or public (8.8.8.8, 1.1.1.1) - caches by TTL
4. **TLD/root hints**: rarely change, cached for days

**DNS record types**:
| Type | Purpose | Example |
|------|---------|---------|
| A | IPv4 address | example.com -> 93.184.216.34 |
| AAAA | IPv6 address | example.com -> 2606:2800:220:1:... |
| CNAME | Alias | www.example.com -> example.com |
| NS | Nameserver delegation | example.com NS ns1.example.com |
| MX | Mail server | example.com MX mail.example.com |
| TXT | Verification, SPF, DKIM | "v=spf1 include:..." |
| SRV | Service discovery | _http._tcp.example.com |

**DNS-based load balancing**: Return multiple A records with short TTL. Clients pick randomly or round-robin. Limitation: no health awareness, client caching ignores TTL changes.

---

## WebSocket Protocol

### Upgrade Handshake (HTTP -> WebSocket)

```
Client Request:
GET /chat HTTP/1.1
Host: server.example.com
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Sec-WebSocket-Version: 13

Server Response:
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
```

**After upgrade**: Full-duplex binary frames over the same TCP connection. No HTTP overhead per message.

**When to use what**:
| Pattern | Use Case | Overhead |
|---------|----------|----------|
| Short polling | Legacy, simple | High (new connection per poll) |
| Long polling | Near-real-time, broad support | Medium (held connections) |
| SSE (Server-Sent Events) | One-way server push | Low (single HTTP connection) |
| WebSocket | Bidirectional real-time | Lowest (persistent, binary frames) |

---

## Load Balancing

### L4 vs L7 - Internal Implementation

**L4 (Transport Layer) Load Balancer**:
- Operates on TCP/UDP packets without parsing application payload
- Decisions based on: source IP, dest IP, source port, dest port
- Implementation: rewrites destination IP/port in packet header (NAT-based) or uses DSR (Direct Server Return)
- Extremely fast: no payload inspection, kernel-level (DPDK, eBPF, IPVS)
- Cannot route by URL path, HTTP headers, or cookies

```
L4 Load Balancing (packet-level):

Client --> [LB rewrites dest IP] --> Backend Server
        <-- [Server replies directly or via LB] <--

Techniques:
1. DNAT: LB rewrites dest IP, backend replies to LB
2. DSR (Direct Server Return): backend replies directly to client
   (LB only handles inbound - higher throughput)
```

**L7 (Application Layer) Load Balancer**:
- Terminates TCP connection from client, opens new connection to backend
- Parses HTTP: can route by URL path, headers, cookies, request body
- Enables: A/B testing routing, canary deployments, auth offloading, SSL termination
- Higher latency: full protocol parsing, double TCP handshake

```
L7 Load Balancing (application-level):

Client --[TCP+TLS]--> LB --[TCP]--> Backend
       <--[TCP+TLS]-- LB <--[TCP]-- Backend

LB maintains TWO connection pools:
1. Client-facing (TLS terminated)
2. Backend-facing (plain TCP, possibly keep-alive)
```

### How NGINX Actually Works

NGINX uses an event-driven, non-blocking architecture:

```
                     +------------------+
                     |   Master Process |
                     | (config, signals)|
                     +--------+---------+
                              |
          +-------------------+-------------------+
          |                   |                   |
  +-------v------+   +-------v------+   +-------v------+
  | Worker Proc 1|   | Worker Proc 2|   | Worker Proc N|
  | (epoll loop) |   | (epoll loop) |   | (epoll loop) |
  +--------------+   +--------------+   +--------------+
```

**Key internals**:
1. **Master process**: reads config, binds ports, spawns workers
2. **Worker processes**: each runs a single-threaded event loop (epoll on Linux, kqueue on BSD)
3. **epoll**: kernel-level I/O multiplexing, O(1) for ready events (vs select's O(n))
4. **Connection pooling**: workers maintain persistent connections to backends (keepalive)
5. **Shared memory zones**: workers share state (rate limiting counters, session data) via shared memory

**Why single-threaded per worker works**: Network I/O is the bottleneck, not CPU. epoll handles 100K+ concurrent connections per worker by never blocking on a single connection.

### Connection Pooling

```
Without pooling:          With pooling:
Client -> LB -> new TCP   Client -> LB -> reuse TCP
            -> new TCP              -> reuse TCP
            -> new TCP              -> reuse TCP
(3-way handshake each     (amortized over many
 time = latency)           requests = fast)
```

Pooling parameters:
- **max_connections**: per-backend connection limit
- **idle_timeout**: close idle connections after N seconds
- **max_requests_per_conn**: recycle after N requests (prevent memory leaks)

### GSLB (Global Server Load Balancing)

```
User (India)                     User (US)
     |                                |
     v                                v
+----------+                   +----------+
| DNS Query|                   | DNS Query|
+----+-----+                   +----+-----+
     |                                |
     v                                v
+------------------------------------------+
|         GSLB (Geo-DNS Authority)          |
| Returns closest healthy data center IP   |
+------------------------------------------+
     |                                |
     v                                v
+-----------+                  +-----------+
| DC Mumbai |                  | DC Oregon |
| 10.1.1.1  |                  | 10.2.2.2  |
+-----------+                  +-----------+
```

**GSLB methods**:
- **Geo-DNS**: Return IP of nearest data center based on resolver's IP geolocation
- **Anycast**: Same IP announced from multiple locations; BGP routes to closest
- **Latency-based**: Active probing to find lowest-latency DC for each resolver
- **Failover**: Primary DC fails health check, GSLB removes from DNS

### Session Persistence (Sticky Sessions)

Methods:
1. **Cookie-based**: LB injects cookie with backend server ID (`SERVERID=backend2`)
2. **Source IP hash**: Same client IP always routes to same backend (breaks with NAT/proxy)
3. **Application-level**: Store session in shared store (Redis), no stickiness needed

**Cost of stickiness**: Uneven load distribution, harder scaling, longer draining on deploys.

---

## Load Balancing Algorithms

### 1. Round Robin
Each request goes to the next server in sequence.

```
Servers: [A, B, C]
Request 1 -> A
Request 2 -> B
Request 3 -> C
Request 4 -> A  (wraps around)
```

**Distribution**: Perfectly uniform if all requests have equal cost. In practice, request processing time varies, leading to uneven load.

**Mathematical property**: After N requests with S servers, each server gets exactly floor(N/S) or ceil(N/S) requests.

### 2. Weighted Round Robin
Servers with higher weight get proportionally more requests.

```
Servers: A(weight=5), B(weight=3), C(weight=2)
Total weight = 10
Distribution over 10 requests: A=5, B=3, C=2

Smooth Weighted Round Robin (NGINX algorithm):
- Avoids bursts like AAAAA BBB CC
- Produces: A B A C A B A B A C (interleaved)
```

**NGINX smooth algorithm**:
1. currentWeight[i] += effectiveWeight[i] for all servers
2. Pick server with max currentWeight
3. currentWeight[selected] -= totalWeight

### 3. Least Connections
Route to the server with fewest active connections.

**When it excels**: Long-lived connections (WebSocket, database connections, large file uploads) where request duration varies significantly.

**Mathematical insight**: Approximates optimal scheduling (minimizing maximum load) in the online case. With equal-capacity servers, approaches the optimal within a constant factor.

### 4. Least Response Time
Route to server with lowest average response time (combines least connections + latency).

**Implementation**: Track EWMA (Exponentially Weighted Moving Average) of response times:
```
ewma_new = alpha * latest_response_time + (1 - alpha) * ewma_old
```
Typical alpha = 0.1-0.3. Lower alpha = smoother, slower adaptation.

### 5. IP Hash / Consistent Hashing
Deterministic routing: same input always maps to same backend.

```
server_index = hash(client_ip) % num_servers    // IP Hash
server = ring.lookup(hash(request_key))          // Consistent Hash
```

**Use cases**: Stateful backends, cache-friendly routing (maximize cache hit ratio per backend).

### 6. Random with Two Choices (Power of Two Choices)
Pick 2 random servers, route to the one with fewer connections.

**Mathematical property**: Reduces maximum load from O(log n / log log n) to O(log log n). Dramatic improvement with minimal overhead. Used in modern service meshes (Envoy).

---

## Health Checks

### Active Health Checks
The load balancer periodically probes backends:

```
LB Health Check Loop:
  every 5s:
    for each backend:
      send GET /health
      if response.status == 200 && latency < 2s:
        mark HEALTHY
      else:
        failures[backend]++
        if failures[backend] >= 3:   // threshold
          mark UNHEALTHY
          
  // Recovery: require 2 consecutive successes
  for each UNHEALTHY backend:
    send GET /health
    if response.status == 200:
      successes[backend]++
      if successes[backend] >= 2:
        mark HEALTHY
```

### Passive Health Checks
Monitor real traffic for errors:
- Track 5xx response rate per backend
- If error rate > threshold (e.g., 50% of last 10 requests), mark unhealthy
- No extra probe traffic

### Best Practice: Combine Both
- Active checks catch completely dead servers quickly
- Passive checks catch degraded servers under real load

---

## Zero-Downtime Deployments

### Blue-Green Deployment
```
         +----> [Blue Env v1.0] (current live)
LB ------+
         +----> [Green Env v2.0] (deploy here, test)

After verification:
         +----> [Blue Env v1.0] (standby/rollback)
LB ------+
         +----> [Green Env v2.0] (flip traffic here)
```
- **Pros**: Instant rollback (flip back), full environment tested
- **Cons**: 2x infrastructure cost

### Canary Deployment
```
LB (weight-based routing):
  95% traffic --> [v1.0 cluster] (stable)
   5% traffic --> [v2.0 cluster] (canary)

Monitor error rates, latency p99 on canary.
If OK: gradually increase (10%, 25%, 50%, 100%)
If bad: rollback canary to v1.0
```

### Rolling Deployment
```
Time 0:  [v1] [v1] [v1] [v1]  (4 instances)
Time 1:  [v2] [v1] [v1] [v1]  (replace 1 at a time)
Time 2:  [v2] [v2] [v1] [v1]
Time 3:  [v2] [v2] [v2] [v1]
Time 4:  [v2] [v2] [v2] [v2]  (complete)
```
- **Connection draining**: Before removing a server, stop sending NEW requests and wait for in-flight requests to complete (drain timeout, typically 30s)
- **Pros**: No extra infrastructure, gradual
- **Cons**: Both versions serve simultaneously (compatibility needed)

---

## Files
- [round_robin.cpp](round_robin.cpp)
- [weighted_round_robin.cpp](weighted_round_robin.cpp)
- [least_connections.cpp](least_connections.cpp)
- [load_balancer_full.cpp](load_balancer_full.cpp) - pluggable strategy

## Interview Questions
1. L4 vs L7 LB - when to choose which? How does L4 achieve higher throughput?
2. How does sticky session work and what does it cost (scalability, failover)?
3. How do you do zero-downtime deployments with a LB? Explain connection draining.
4. What is connection draining and why is the timeout important?
5. How does GSLB (geo-DNS) differ from in-region LB? What about anycast?
6. TCP vs UDP - when to use UDP for a service? Why is DNS over UDP?
7. HTTP/2 multiplexing - explain how it solves HTTP/1.1 HOL blocking but not TCP HOL blocking.
8. Why is HTTPS slower? What does TLS session resumption and 0-RTT do?
9. Explain TCP congestion control. What happens when a packet is lost?
10. How does QUIC (HTTP/3) solve the TCP HOL blocking problem?
11. What is the Power of Two Choices algorithm and why is it effective?
12. How does NGINX handle 100K+ concurrent connections with few worker processes?

## Daily Assignment
1. Build a `LoadBalancer` class supporting Round Robin, Weighted RR, Least Connections via Strategy pattern.
2. Add a health-check thread that marks unhealthy servers offline (active + passive).
3. Bonus: simulate 1000 requests across 5 servers and print distribution histogram. Compare Round Robin vs Least Connections under variable request durations.
4. Advanced: implement smooth weighted round robin (NGINX algorithm) and verify interleaved distribution.
