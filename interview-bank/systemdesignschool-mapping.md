# systemdesignschool.io - Cross-Practice Mapping

[systemdesignschool.io](https://systemdesignschool.io) is an interactive system-design course/practice site with editorial walkthroughs and submission feedback. Use it for **alternate explanations** and **interactive practice** of the same topics already covered in this repo.

> Content was rephrased and summarized for compliance with licensing restrictions of the source material.

## Why use it alongside this repo

- This repo: hands-on C++ implementations + theory + interview Qs
- systemdesignschool.io: interactive prompts, editorial solutions with thought-process patterns, submission feedback

## Curriculum mapping (their structure -> our day)

| systemdesignschool.io section | This repo |
|------------------------------|-----------|
| API Design (intro, example, pagination, auth, authorization, API Gateway) | Add to your reading alongside Day 17 (DB schema/REST) and Day 19 (messaging) |
| Non-functional Requirements (High Availability, Latency, Throughput) | Day 18 (CAP) + Day 21 (circuit breaker) |
| Resource Estimation (QPS, real-world examples) | Day 26 (URL shortener), Day 27 (Twitter), Day 28 (WhatsApp/YouTube) - all include capacity sections |
| Microservices: Sync Communication (timeout, retries, circuit breaker, fallbacks, service discovery) | Day 19 + Day 21 |
| Microservices: Async Communication (message queues, Kafka, log-based queues) | Day 19 |
| Scaling Services (horizontal scaling, load balancing, auto scaling, stateless vs stateful) | Day 15 |
| Consistent Hashing | Day 21 |
| Caching | Day 16 |
| Rate Limiter | Day 20 |

## Problem cross-practice list

After completing the corresponding day in this repo, attempt the same problem on systemdesignschool.io for an alternate angle and editorial solution.

| Their problem | Maps to this repo |
|---------------|-------------------|
| Design Twitter | [Day 27](../week4-case-studies/day27-twitter-uber-hld/) |
| Design WhatsApp | [Day 28](../week4-case-studies/day28-whatsapp-youtube-hld/) |
| Design YouTube | [Day 28](../week4-case-studies/day28-whatsapp-youtube-hld/) |
| Design Rate Limiter | [Day 20](../week3-hld-foundation/day20-rate-limiting/) |
| Design Typeahead (Autocomplete) | New problem - bonus practice after Day 17 (uses Trie) |
| Design Google Doc | New - tackle after Day 19 (collab editing needs CRDT/OT + queues) |
| Design Job Scheduler | New - tackle after Day 19 |
| Design Realtime Monitoring System | New - after Day 19 (Kafka) + Day 21 (CB) |
| Design Comment System | New - after Day 17 (DB schema) + Day 18 (sharding) |
| Design Google Maps | New - tackle after Day 27 (geo concepts) |
| Design TicketMaster | New - similar to Day 24 BookMyShow but at HLD scale |
| Design Tinder | New - similar geo-matching to Day 27 Uber |
| Design Webhook | New - relates to Day 19 messaging |
| Design LeetCode | New - online judge / code execution sandbox |

## Suggested workflow

1. Finish the corresponding day in this repo (theory + C++ + assignment)
2. Open the matching problem on systemdesignschool.io
3. Try it from scratch on paper / whiteboard (45 min)
4. Read their editorial; note any patterns or trade-offs you missed
5. Update `weekN-notes.md` with new insights

## Topics not deeply covered here

systemdesignschool.io has good coverage of the following that the 4-week repo only touches lightly:

- **API authentication / authorization deep dive** (API keys, OAuth 2.0, JWT, mTLS)
- **API Gateway** patterns (auth offload, rate limit, request transformation)
- **Service discovery** (client-side vs server-side, DNS-based, Consul/etcd)
- **Webhook** delivery semantics (retry, signing, idempotency)

If your interviews target backend / API roles heavily, supplement this repo with their API and microservice sections.
