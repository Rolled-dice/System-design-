# System Design Mastery in 1 Month (C++)

A condensed 4-week roadmap to master **System Design (HLD + LLD)** in C++ with OOPS, SOLID, design patterns, daily assignments, and real interview questions.

> Assumes you already know CS basics (OS, Networking, DSA). We move fast.

For the full 12-week version with progress mapping, see [ROADMAP.md](ROADMAP.md).

**Repo stats:** 28 days of content - 82 C++ files (all compile clean with `g++ -std=c++17 -Wall -pthread`) - 38 markdown docs covering theory, interview questions, and assignments.

---

## Roadmap Overview

| Week | Focus | Topics | Status |
|------|-------|--------|--------|
| **Week 1** | LLD Foundation | OOPS, SOLID, Creational Patterns | [ ] |
| **Week 2** | LLD Patterns | Structural + Behavioral Patterns | [ ] |
| **Week 3** | HLD Foundation | Networking, Caching, DBs, Queues, Distributed Systems | [ ] |
| **Week 4** | Practice | LLD + HLD case studies, Mock interviews | [ ] |

---

## Daily Schedule (Suggested)

- **1.5 hr** theory + notes
- **1.5 hr** C++ implementation / code-along
- **1 hr** interview question practice (HLD/LLD)
- **30 min** review previous day

---

## Folder Structure

```
.
|-- week1-oops-solid-creational/
|   |-- day1-oops/
|   |-- day2-solid/
|   |-- day3-singleton/
|   |-- day4-factory/
|   |-- day5-abstract-factory/
|   |-- day6-builder/
|   |-- day7-prototype/
|-- week2-structural-behavioral/
|   |-- day8-adapter-decorator/
|   |-- day9-facade-proxy/
|   |-- day10-composite-bridge-flyweight/
|   |-- day11-observer-strategy/
|   |-- day12-command-state/
|   |-- day13-iterator-template-chain/
|   |-- day14-mediator-visitor-memento/
|-- week3-hld-foundation/
|   |-- day15-networking-loadbalancing/
|   |-- day16-caching-cdn/
|   |-- day17-databases-sql-nosql/
|   |-- day18-sharding-replication-cap/
|   |-- day19-messaging-queues/
|   |-- day20-rate-limiting/
|   |-- day21-consistent-hashing/
|-- week4-case-studies/
|   |-- day22-parking-lot-lld/
|   |-- day23-splitwise-lld/
|   |-- day24-bookmyshow-lld/
|   |-- day25-tictactoe-snakeladder-lld/
|   |-- day26-url-shortener-hld/
|   |-- day27-twitter-uber-hld/
|   |-- day28-whatsapp-youtube-hld/
|-- interview-bank/
|   |-- lld-questions.md
|   |-- hld-questions.md
|   |-- final-checklist.md
```

---

## Week 1 - LLD Foundation

| Day | Topic | Folder |
|-----|-------|--------|
| 1 | OOPS Concepts (Encapsulation, Inheritance, Polymorphism, Abstraction) | [day1-oops](week1-oops-solid-creational/day1-oops/) |
| 2 | SOLID Principles | [day2-solid](week1-oops-solid-creational/day2-solid/) |
| 3 | Singleton Pattern | [day3-singleton](week1-oops-solid-creational/day3-singleton/) |
| 4 | Factory Method | [day4-factory](week1-oops-solid-creational/day4-factory/) |
| 5 | Abstract Factory | [day5-abstract-factory](week1-oops-solid-creational/day5-abstract-factory/) |
| 6 | Builder | [day6-builder](week1-oops-solid-creational/day6-builder/) |
| 7 | Prototype | [day7-prototype](week1-oops-solid-creational/day7-prototype/) |

## Week 2 - Structural + Behavioral Patterns

| Day | Topic |
|-----|-------|
| 8 | Adapter, Decorator |
| 9 | Facade, Proxy |
| 10 | Composite, Bridge, Flyweight |
| 11 | Observer, Strategy |
| 12 | Command, State |
| 13 | Iterator, Template Method, Chain of Responsibility |
| 14 | Mediator, Visitor, Memento |

## Week 3 - HLD Foundation

| Day | Topic |
|-----|-------|
| 15 | Networking + Load Balancing |
| 16 | Caching + CDN |
| 17 | Databases - SQL vs NoSQL, Indexing |
| 18 | Sharding, Replication, CAP |
| 19 | Messaging Queues (Kafka, RabbitMQ) |
| 20 | Rate Limiting (Token Bucket, Leaky Bucket) |
| 21 | Consistent Hashing, Circuit Breaker |

## Week 4 - Case Studies

| Day | LLD/HLD | Problem |
|-----|---------|---------|
| 22 | LLD | Parking Lot |
| 23 | LLD | Splitwise |
| 24 | LLD | BookMyShow |
| 25 | LLD | Tic-Tac-Toe + Snake & Ladder |
| 26 | HLD | URL Shortener |
| 27 | HLD | Twitter + Uber |
| 28 | HLD | WhatsApp + YouTube |

---

## Build & Run

All C++ code targets **C++17**. To compile any example:

```bash
g++ -std=c++17 -O2 -Wall filename.cpp -o output && ./output
```

Some examples requiring threads:

```bash
g++ -std=c++17 -pthread filename.cpp -o output && ./output
```

---

## Interview Bank

- [LLD Questions](interview-bank/lld-questions.md) - 28 problems with patterns to use
- [HLD Questions](interview-bank/hld-questions.md) - 30 problems + capacity cheat-sheet + latency numbers + component picker
- [Extended Questions](interview-bank/extended-questions.md) - 100+ more LLD + HLD problems, company-specific patterns
- [External Resources](interview-bank/resources.md) - curated repos, books, newsletters, YouTube, practice platforms
- [Final Checklist](interview-bank/final-checklist.md) - tickable readiness self-assessment

## DevOps / SRE Interview Prep

A separate track for DevOps and SRE interviews (Azure, AWS, Kubernetes, Docker, Terraform, Jenkins, GitHub Actions, Azure DevOps, Python, Bash, Prometheus, Grafana, ELK, ArgoCD):

- [devops-interview-prep/](devops-interview-prep/) — 11 files covering deep-dive troubleshooting scenarios, Kubernetes/Docker, Terraform, CI/CD, cloud (AWS + Azure), observability, Python/Bash automation, GitOps/DevSecOps/FinOps, architecture design scenarios, behavioral stories, and a 100-question rapid-fire round.

## How to Get the Most from This Repo

See [STUDY_GUIDE.md](STUDY_GUIDE.md) for the daily/weekly loop, active-recall techniques, mock interview schedule, common pitfalls, and spaced-review timeline.
