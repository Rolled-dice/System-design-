# Day 15 - Networking + Load Balancing

## Networking Refresher

### Layers (TCP/IP)
| Layer | Examples |
|-------|----------|
| Application | HTTP, gRPC, DNS, WebSocket |
| Transport | TCP (reliable, ordered), UDP (fast, unordered) |
| Network | IP, ICMP, routing |
| Link | Ethernet, MAC |

### Key Concepts
- **TCP 3-way handshake**: SYN -> SYN/ACK -> ACK
- **TLS handshake**: 1-2 RTT for HTTPS
- **HTTP/1.1 vs HTTP/2 vs HTTP/3 (QUIC)**
- **DNS resolution**: recursive resolver -> root -> TLD -> authoritative
- **WebSocket**: full-duplex over single TCP connection
- **Long polling vs Server-Sent Events vs WebSocket**

### HTTP Status Codes (must know)
- 2xx Success: 200, 201, 204
- 3xx Redirect: 301, 302, 304
- 4xx Client: 400, 401, 403, 404, 409, 429
- 5xx Server: 500, 502, 503, 504

## Load Balancing

### Layers
- **L4 (transport)**: TCP/UDP, fast, opaque to payload (HAProxy in TCP mode, AWS NLB)
- **L7 (application)**: parses HTTP, can route by URL/header (NGINX, AWS ALB, Envoy)

### Algorithms
| Algorithm | Use Case |
|-----------|----------|
| Round Robin | Equal capacity servers |
| Weighted Round Robin | Heterogeneous capacity |
| Least Connections | Long-lived connections (DB, WebSocket) |
| Least Response Time | Latency-sensitive |
| IP Hash / Sticky Session | Session affinity |
| Consistent Hashing | Cache servers, sharded systems |

### Health Checks
- Active (LB pings `/health`) vs Passive (track failed requests)
- Tune intervals & failure thresholds

## Files
- [round_robin.cpp](round_robin.cpp)
- [weighted_round_robin.cpp](weighted_round_robin.cpp)
- [least_connections.cpp](least_connections.cpp)
- [load_balancer_full.cpp](load_balancer_full.cpp) - pluggable strategy

## Interview Questions
1. L4 vs L7 LB - when to choose which?
2. How does sticky session work and what does it cost?
3. How do you do zero-downtime deployments with a LB?
4. What is connection draining?
5. How does GSLB (geo-DNS) differ from in-region LB?
6. TCP vs UDP - when to use UDP for a service?
7. HTTP/2 multiplexing - explain head-of-line blocking solution.
8. Why is HTTPS slower? What does TLS session resumption do?

## Daily Assignment
1. Build a `LoadBalancer` class supporting Round Robin, Weighted RR, Least Connections via Strategy pattern.
2. Add a health-check thread that marks unhealthy servers offline.
3. Bonus: simulate 1000 requests across 5 servers and print distribution.
