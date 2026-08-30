# Part 9 — Architecture & Automation Design Scenarios

Whiteboard problems. Give yourself 30–45 minutes each, drawn on paper, spoken out loud. Use the **RRSCO** pass from the README: Requirements → Reliability → Scalability → Cost → Operations, and always end on trade-offs.

---

## A1. Design a highly available platform for a B2C API on Azure

**Prompt.** A payments-adjacent API, 5,000 RPS peak, 300ms p99 budget, must survive an AZ failure with no downtime and a region failure within 15 minutes. Regulated — data must stay in the EU, and every change must be auditable.

### Model answer

**Requirements first (always start here).** Confirm: 5k RPS peak / what's the average (drives cost strategy)? Read/write ratio? Payload size? RTO 15 min, RPO — ask explicitly; "no data loss" and "15 minute recovery" imply synchronous replication and a warm standby, which is a very different bill from backup/restore. Data residency = EU regions only. Auditable = GitOps + immutable logs.

**Topology.**
- **Edge:** Azure **Front Door** (global anycast, WAF, TLS termination, caching for static, health-probe-based failover between regions). It's the piece that makes the 15-minute regional RTO achievable without DNS TTL games.
- **Regions:** primary West Europe, secondary North Europe. Active/passive **warm standby** — the standby runs at reduced capacity and scales on failover. I'd argue against full active/active initially because of the data-consistency cost, and say so explicitly as a trade-off I'm choosing.
- **Compute:** AKS with node pools spread across **3 availability zones**, `topologySpreadConstraints` on zone so no single AZ loss removes a majority of replicas, minimum 3 replicas per service, PDBs, cluster autoscaler/Karpenter-equivalent with headroom for the failover surge.
- **Ingress inside the cluster:** Application Gateway + AGIC (or Gateway API), with WAF at the edge already handling the bulk.
- **Data:** Azure SQL Business Critical or PostgreSQL Flexible Server with **zone-redundant HA** (synchronous standby in another AZ → RPO ≈ 0, automatic failover for AZ loss) plus a **geo-replica** in North Europe for regional DR (asynchronous → small RPO, which I'd quantify and get signed off). Connection pooling (PgBouncer) because failover storms exhaust connections.
- **Caching:** Azure Cache for Redis, zone-redundant, for session/read-through — protects the DB at peak and helps the latency budget.
- **Async work:** Service Bus (geo-DR paired namespace) so writes can be queued and drained rather than lost during a partial failure. Idempotent consumers.
- **State that isn't in the DB:** none in the app tier — keep it stateless so scaling and failover are trivial. Say this explicitly; it's the enabling decision for everything else.

**Reliability specifics.** Health probes at every layer, shallow liveness / deep readiness split, circuit breakers and timeouts on every dependency with budgets that sum to less than the 300ms p99, graceful degradation (serve cached/read-only if the write path is down rather than returning 500s), retries with jitter and caps.

**Scalability.** Stateless tier scales on RPS-per-pod (via KEDA/Prometheus adapter), not CPU. Bottleneck is the database — plan read replicas for read scaling and identify the sharding key *before* you need it. 5k RPS is comfortably within a single well-tuned PostgreSQL for writes, so I would not shard prematurely.

**Cost.** Reserved/savings plans on the steady baseline, spot for batch and CI, standby region kept minimal (control plane + 1 replica) and scaled by the failover runbook, Front Door caching to cut origin traffic, lifecycle policies on logs. Quantify: standby at ~20% capacity is roughly 20% extra rather than 100%.

**Operations.** Terraform for infrastructure (split state: network / data / platform / apps), ArgoCD for workloads with prod behind manual sync, canary via Argo Rollouts with automated analysis, OTel → Azure Monitor/Prometheus + Grafana, burn-rate SLO alerts, Key Vault + Workload Identity for secrets with no static credentials, quarterly **tested** regional failover game day.

**Trade-offs to state out loud.** Warm standby not active/active (accepting a small RPO and a 15-min RTO in exchange for far lower cost and no multi-master consistency problems). Single database region for writes (accepting cross-region write latency on failover). No service mesh initially (accepting less L7 control to avoid the operational overhead until the service count justifies it).

**Follow-ups to expect.** "How do you actually fail over?" — Front Door health probes shift traffic; geo-replica promotion is a deliberate, runbooked, human-approved action because promoting on a false positive causes split-brain. "How do you test it?" — game days, and a monthly non-prod full failover. "What breaks first at 10× traffic?" — the database write path and connection limits.

---

## A2. Zero-downtime deployment for a service with a database schema change

**Prompt.** A team needs to rename a column and add a NOT NULL field, on a table with 400M rows, with zero downtime and a working rollback.

### Model answer

Lead with the principle: **the deploy and the schema change must be decoupled, and every step must be independently reversible.** A column rename is never a single step in a live system.

**Expand / contract (parallel change), across four releases:**
1. **Expand.** Add the new column as **nullable with a default at the application layer**, not a table-rewriting `NOT NULL DEFAULT` (on 400M rows in older Postgres/MySQL that takes an exclusive lock and is an outage). Deploy code that **writes both** old and new columns and **reads the old one**. Fully backward compatible — safe to roll back.
2. **Backfill.** Batched, throttled, resumable backfill (e.g. 10k rows per batch with a sleep, driven by a Job with progress tracking and the ability to stop). Monitor replica lag and lock waits; abort if lag grows. Never one giant `UPDATE`.
3. **Flip reads.** Deploy code that reads the new column and still writes both, behind a **feature flag** so you can flip back instantly without a deploy. Verify with a consistency check comparing old vs new for a sample.
4. **Contract.** In a *later* release, stop writing the old column; then, after a bake period and a verified backup, drop it. Adding the `NOT NULL` constraint comes last, using `NOT VALID` + `VALIDATE CONSTRAINT` where the engine supports it to avoid a long lock.

**Deployment mechanics.** Migrations run as an ArgoCD `PreSync` hook or a separate pipeline stage — never in the app's startup path (N replicas racing to migrate is a classic failure). Migrations must be idempotent and forward-only. Rolling update with `maxUnavailable: 0`, readiness gating, `preStop` drain, and graceful shutdown.

**Rollback story.** Because every step is additive and backward-compatible, rollback is always "deploy the previous image" — never "run a down-migration." State this plainly: **irreversible schema changes must be separated from code deploys by at least one release boundary.**

**How you'd verify.** Canary the read-flip to 1% of traffic with error-rate and latency analysis, dual-read comparison metrics, and alerts on constraint violations. Load-test the migration on a production-sized copy first, measuring lock duration.

---

## A3. Automate a manual toil process

**Prompt.** Onboarding a new microservice currently takes 3 days of manual work across 6 teams: repo creation, CI setup, cloud resources, DNS, monitoring, on-call registration. Automate it.

### Model answer

**Frame it as a platform problem, not a script problem** — that's the whole point of the question.

1. **Quantify the toil.** 6 teams × 3 days × N services/month = the business case. Also count the error rate (how many services ship with no alerts or no PDB?) — inconsistency is a bigger cost than the hours.
2. **Standardize before automating.** You cannot automate a process with 6 different opinions. Define a **golden path**: an opinionated service template with sane defaults (probes, resources, PDB, topology spread, dashboards, SLO, log format, security context).
3. **Build it as self-service.** A `service.yaml` manifest in a platform repo (name, owner, tier, dependencies, SLO) is the single input. A pipeline/controller then: scaffolds the repo from a template (Backstage software template or Cookiecutter) with CODEOWNERS and branch protection; commits Terraform for cloud resources (database, queue, bucket, IAM role/managed identity) into the infra repo as a PR; generates the ArgoCD ApplicationSet entry; creates DNS records; provisions Grafana dashboards and Prometheus rules from templates; registers the on-call rotation in PagerDuty; and adds the service to the CMDB/catalog.
4. **Design for failure.** Idempotent (re-running produces the same result), resumable, dry-run mode, and every generated change lands as a **reviewable PR** rather than a direct mutation — so the automation is auditable and a bad template doesn't silently break 40 services.
5. **Interface.** Backstage/developer portal or a simple PR-driven workflow. Keep the human approval for the resources that cost money.
6. **Measure success.** Lead time 3 days → 30 minutes, and — more importantly — **100% of new services have alerts, SLOs, and PDBs on day one.** Report both.
7. **Own it.** The platform team owns the template; changes to it roll out to all services via versioned updates (Renovate PRs), not by editing 40 repos.

**Trade-off to name.** A golden path constrains choice. Handle that by making the path optional-but-easy and providing a documented escape hatch, otherwise strong teams will bypass the platform and you'll fragment anyway.

---

## A4. Design a multi-tenant Kubernetes platform for 20 teams

### Model answer

- **Tenancy model decision first:** namespace-per-team (cheap, shared control plane, weaker isolation) vs cluster-per-team (strong isolation, N× operational cost and cost overhead) vs virtual clusters (vcluster — a good middle ground). Recommend **namespace-per-team with hard multi-tenancy controls**, and cluster-per-team only for regulated or noisy-neighbour-sensitive workloads. Justify by cost and operational load.
- **Isolation controls:** ResourceQuota + LimitRange per namespace; default-deny NetworkPolicy with explicit allow-lists; Pod Security Admission `restricted`; RBAC scoped to the namespace; separate node pools with taints for sensitive workloads; ArgoCD Projects restricting which repos/clusters/namespaces a team can deploy to.
- **Fairness:** PriorityClasses so platform components and prod workloads pre-empt batch; quotas to prevent one team starving the cluster; and node-level protections (`kube-reserved`, `system-reserved`).
- **Self-service:** golden Helm base chart, CRD-based abstractions (or Crossplane compositions) so teams request a database without learning Terraform, and a paved CI path.
- **Cost:** OpenCost showback per namespace, shared-cost allocation rules, and requests-vs-usage dashboards to drive right-sizing conversations.
- **Upgrades:** the hard part at 20 teams — publish a deprecation calendar, run API-deprecation scanning against team manifests, maintain a canary cluster one version ahead, and never upgrade without a tested rollback for node pools.
- **Support model:** clear responsibility split (platform owns cluster + addons + SLO of the platform; teams own their workloads), documented runbooks, and an escalation path. Say this — multi-tenancy fails on organizational boundaries more often than technical ones.

---

## A5. Migrate a monolith on VMs to containers with no downtime

### Model answer

- **Assess and slice.** Inventory dependencies, state, filesystem assumptions, licensing, and startup behaviour. Don't lift-and-shift a stateful monolith blindly — first make it **12-factor-ish**: config from environment, logs to stdout, no local session state (move to Redis), no writing to local disk.
- **Strategy:** containerize as-is first (a "boring" lift to get CI/CD and observability benefits), *then* decompose with the **strangler fig** pattern — put a proxy in front, extract one capability at a time behind it, and route incrementally. Avoid a big-bang rewrite and say why.
- **Traffic migration:** run VM and container fleets **in parallel** behind the same load balancer/target group, shift weight 1% → 10% → 50% → 100% with SLI monitoring at each step and instant rollback by re-weighting. Keep the VM fleet warm until you've been stable for a full business cycle (including month-end batch).
- **Data:** the database usually stays put initially — do not migrate compute and data simultaneously. That halves the risk surface.
- **Validation:** shadow/mirror traffic to the container fleet before it serves real users, compare responses and latency, and run the batch/cron paths explicitly (they're the thing everyone forgets).
- **Rollback plan** at every phase, plus a defined "abort criteria" agreed in advance so the decision isn't made under pressure.

---

## A6. Build a CI/CD + infrastructure platform from scratch for a 30-person startup

### Model answer

Prioritize ruthlessly and justify the order — this question tests judgement, not knowledge breadth.

**Week 1–2 (foundation):** cloud accounts with separation (prod/non-prod/shared), OIDC-based CI identity, Terraform with remote state, one VPC/VNet, source control with branch protection.
**Week 3–4 (delivery):** GitHub Actions reusable workflow for build/test/scan/push, container registry, one environment deployed via GitOps. Get a deploy from commit to prod working end-to-end before adding sophistication.
**Week 5–6 (safety):** observability baseline (metrics, logs, one SLO, burn-rate alerts, on-call rotation), automated backups **with a tested restore**, and a rollback that anyone can execute.
**Then:** progressive delivery, cost visibility, policy-as-code, developer portal.

**What I'd deliberately *not* do early:** service mesh, multi-region, Kafka, a custom internal platform, or Kubernetes at all if the workload is 3 services (Container Apps/ECS Fargate/App Runner may be the honest answer). Say this — recognizing that Kubernetes is often premature is a strong senior signal at a startup.

**Guiding principle:** optimize for the smallest number of moving parts that gives you safe, frequent deploys and the ability to debug production. Complexity you can't operate at 3am is a liability.

---

## A7. Design a log/metrics pipeline for 500 services

### Model answer

- **Standardize the contract:** structured JSON logs to stdout with required fields (timestamp, level, service, trace_id, message), OTel instrumentation for traces and metrics, and a naming convention for metrics. Without a contract, no pipeline design saves you.
- **Collection:** OTel Collector as a DaemonSet (node-local, cheap) plus a gateway deployment for tail-sampling and enrichment.
- **Transport:** Kafka for buffering and fan-out (SIEM, analytics, and the observability backend all consume the same stream), giving backpressure tolerance and replay.
- **Storage tiering:** metrics → Prometheus + Thanos/Mimir with downsampling; logs → Loki or Elasticsearch hot/warm/cold with ILM; traces → Tempo with tail-based sampling (100% of errors and slow traces, 1% of the rest).
- **Cost control as a first-class design goal:** per-team ingestion budgets and dashboards, drop rules for known-noisy metrics/logs at the collector, retention tiers, and cardinality limits enforced per scrape job. At 500 services the observability bill can exceed the compute bill — design for that from day one.
- **Multi-tenancy:** per-team labels/tenants, RBAC on data access (logs contain sensitive data), PII redaction in the collector.
- **Reliability:** the pipeline needs its own SLO and a dead-man's-switch, because losing telemetry during an incident is when you need it most.

---

## A8. A single AZ fails. Walk me through what happens to your system.

### Model answer

Answer as a **walk-through of each tier**, showing you've thought about the failure rather than just listing redundancy:
- **Edge/LB** — health probes remove AZ targets within seconds; cross-zone load balancing must be *enabled* or the remaining AZs won't absorb the traffic.
- **Compute** — one third of pods vanish. Do the surviving nodes have capacity for a 50% traffic increase? If you sized at exactly 3 replicas across 3 AZs with no headroom, you now have 2 replicas serving 100% of load. **This is the question most candidates miss:** HA requires N+1 *capacity*, not just spread. Autoscaler needs quota and time to add nodes in the surviving AZs.
- **Data** — synchronous standby promotes (~30–120s), connections break; the app must reconnect gracefully and pools must handle the storm. Async replicas may briefly serve stale reads.
- **In-flight work** — queue consumers die mid-message; idempotent consumers plus visibility timeouts mean messages redeliver rather than vanish.
- **Stateful pods** — RWO volumes are AZ-bound, so those pods cannot reschedule into a surviving AZ. This is a genuine gap people forget: the pod stays `Pending` until the AZ returns.
- **Second-order effects** — retry storms, cold caches causing a DB thundering herd, cross-AZ costs spiking, and alert floods.
- **What I'd verify beforehand:** a game day that actually kills an AZ in staging, capacity headroom modelling, and a documented "we are in degraded 2-AZ mode" runbook.

---

## A9. Design secrets management for 200 services across AWS and Azure

### Model answer

- **Principle: eliminate static secrets wherever possible.** Workload identity (IRSA / AKS Workload Identity) for cloud API access, and dynamic short-lived credentials (Vault database secrets engine) for databases. What you can't eliminate, you centralize and rotate.
- **Storage:** cloud-native stores (Secrets Manager / Key Vault) as the system of record, or Vault if you need one control plane across both clouds and dynamic secrets. Choose based on whether the operational cost of running Vault is justified — say the trade-off.
- **Delivery to workloads:** External Secrets Operator or CSI Secrets Store driver, mounted as **files** (not env vars) so rotation without restart is possible; app-side file-watch reload.
- **GitOps compatibility:** only *references* live in Git (`ExternalSecret` CRs), never ciphertext-that-must-be-decrypted-by-a-cluster-key if you can avoid it.
- **Rotation:** automated, **two-phase** (issue new → consumers accept both → cut over → revoke old), with expiry alerting that fires independently of the automation.
- **Access control and audit:** least-privilege per service identity, per-secret policies, full audit log of every read, and alerting on anomalous access patterns (a service reading a secret it never used before).
- **Break-glass:** a documented, audited, time-boxed emergency access path with two-person approval — because "no human can ever read prod secrets" fails at 3am and people then create shadow copies.

---

## A10. Your company wants to deploy 50 times a day. What has to be true?

### Model answer

This is a maturity question — answer with prerequisites, not tooling.

**Technical prerequisites:** trunk-based development with short-lived branches; a fast (<10 min), trustworthy, low-flake CI; automated tests you'd bet prod on, including contract tests between services; artifact built once and promoted; **decoupled deploy and release via feature flags**; backward-compatible APIs and additive-only migrations; progressive delivery with automated metric analysis and auto-rollback; and infrastructure as code so environments match.

**Observability prerequisites:** SLOs with error budgets (so "can we deploy?" has a data answer), deploy markers correlated with SLIs, and alerting that catches a bad release within minutes.

**Organizational prerequisites:** teams own their services in production (you build it, you run it), no change-advisory-board gate per deploy — instead a *standard change* process where the pipeline **is** the control, small batch sizes as a cultural norm, and blameless postmortems so people aren't afraid to ship.

**The honest caveat to add:** 50 deploys a day is not a goal in itself — it's a *symptom* of low batch size and high confidence. I'd measure lead time and change failure rate, and use the error budget to decide when to slow down. If change failure rate rises, the answer is better testing and smaller changes, not more deploys.
