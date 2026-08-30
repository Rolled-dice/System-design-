# Part 6 — Observability: Prometheus, Grafana, ELK

---

## A. Concepts

**1. Monitoring vs observability.** Monitoring = watching known failure modes with predefined dashboards and alerts ("is CPU high?"). Observability = the property of being able to answer **new** questions about your system without shipping new code — you can debug unknown-unknowns from the telemetry you already emit. Three pillars: metrics (cheap, aggregated, low-cardinality), logs (high detail, expensive at scale), traces (causality and latency attribution across services). Add **events/profiles** for maturity points.

**2. Which signals do you alert on?** Golden Signals (latency, traffic, errors, saturation) for request-driven services; **USE** (utilization, saturation, errors) for resources; **RED** (rate, errors, duration) for services. The rule: **alert on symptoms users feel, diagnose with causes.** "CPU > 80%" is not an alert, it's a dashboard.

**3. SLI, SLO, SLA, error budget — and how they change behavior.** SLI = the measured indicator (e.g. proportion of requests <300ms and non-5xx). SLO = the target (99.9% over 30 days). SLA = the contractual version with penalties. **Error budget** = 1 − SLO = the permitted unreliability; it converts reliability into a negotiable resource. If the budget is burned, feature work pauses in favour of reliability work; if it's untouched, you're being too conservative and can ship faster. Say that last part — it shows you understand SLOs are about *decision-making*, not dashboards.

**4. What is burn-rate alerting and why is it better?** Instead of "error rate > 1% for 5 minutes," alert on how fast you're consuming the error budget, using **multi-window multi-burn-rate** alerts: a fast burn (e.g. 14.4× over 1h → paging, budget gone in ~2 days) and a slow burn (e.g. 6× over 6h → ticket). This gives high precision and recall: it pages for things that actually threaten the SLO and stays quiet for harmless blips.

**5. How do you fix alert fatigue?** Audit every alert against: did it require human action? If not, delete or downgrade it to a ticket/dashboard. Then: alert on symptoms not causes, use burn rates, add inhibition (don't page for pod alerts when the whole cluster is down) and grouping in Alertmanager, set severity honestly, ensure every alert links to a runbook and has an owner, and track pages-per-on-call-shift as a metric you manage. An alert nobody acts on trains people to ignore the ones that matter.

---

## B. Prometheus

**6. How does Prometheus work?** Pull-based: scrapes HTTP `/metrics` endpoints on an interval, stores samples in a local TSDB (WAL + 2h blocks, compacted), evaluates recording and alerting rules, and fires to **Alertmanager** which handles grouping, deduplication, silences, inhibition, and routing. Service discovery (Kubernetes SD, EC2, Azure) keeps targets current. Prometheus itself is a single node — no clustering — which drives the HA and long-term-storage discussion.

**7. Pull vs push — and how do you handle short-lived jobs?** Pull gives you target health for free (`up`), avoids overwhelmed receivers, and makes scrape config the single source of truth. For batch/cron jobs that die before a scrape, use the **Pushgateway** (with care — it's a persistent store and its metrics don't expire, so it's easy to misuse) or better, write results to a durable metric via an exporter, or use the OTel push path.

**8. The four metric types.** Counter (monotonic, always use with `rate()`), Gauge (up/down snapshot), Histogram (bucketed observations → quantiles computed server-side, aggregatable across instances), Summary (client-computed quantiles — **not aggregatable**, so prefer histograms for anything you'll sum across pods). Native/exponential histograms are the modern improvement worth mentioning.

**9. Why `rate()` and not raw counters?** Counters only increase and reset on restart; `rate()` computes per-second average over a window and handles resets. Use `irate()` for fast-moving graphs (last two samples only), `increase()` for absolute change. Rule: **`rate()` before `sum()`** — `sum(rate(x[5m]))` is right, `rate(sum(x)[5m])` is wrong and breaks on restarts.

**10. PromQL you must be able to write on a whiteboard.**
```promql
# Request error ratio (RED)
sum(rate(http_requests_total{status=~"5.."}[5m]))
  / sum(rate(http_requests_total[5m]))

# p99 latency from a histogram
histogram_quantile(0.99, sum by (le) (rate(http_request_duration_seconds_bucket[5m])))

# Pods restarting
increase(kube_pod_container_status_restarts_total[15m]) > 0

# Memory usage vs limit (the OOM early-warning from Part 1)
container_memory_working_set_bytes{container!=""}
  / on(pod,container) kube_pod_container_resource_limits{resource="memory"} > 0.85

# CPU throttling ratio
rate(container_cpu_cfs_throttled_periods_total[5m])
  / rate(container_cpu_cfs_periods_total[5m]) > 0.25

# Disk full in 4 hours (predictive)
predict_linear(node_filesystem_avail_bytes{mountpoint="/"}[6h], 4*3600) < 0

# Top 10 metrics by series count (cardinality hunting)
topk(10, count by (__name__)({__name__=~".+"}))

# Availability SLI over 30d
1 - (sum(increase(http_requests_total{status=~"5.."}[30d]))
       / sum(increase(http_requests_total[30d])))
```
Also know: `by`/`without`, `offset`, `on()`/`ignoring()` for vector matching, `absent()` and `absent_over_time()` for "target disappeared" alerts, and `label_replace`.

**11. What is cardinality and why is it the thing that kills Prometheus?** Every unique combination of metric name + label values is a separate time series held in memory. Adding a label with unbounded values (user ID, request ID, full URL, email, pod name in a high-churn deployment) multiplies series count and OOMs the server. Controls: never put unbounded values in labels; use `metric_relabel_configs` to drop; enforce `sample_limit`/`label_limit` per scrape job so one bad exporter can't take down the platform; review new metrics in code review; alert on `prometheus_tsdb_head_series` growth. This is the #1 Prometheus operational question.

**12. Recording rules — when and why?** Pre-compute expensive or frequently-used queries into new series (e.g. `job:http_error_ratio:rate5m`). Makes dashboards fast, keeps alert evaluation cheap, and gives you stable, reusable SLI definitions. Naming convention: `level:metric:operation`.

**13. How do you make Prometheus HA and long-term?** Run two identical replicas scraping the same targets (dedup at the query layer or via Alertmanager's dedup for alerts). For long retention and global query: **Thanos** (sidecar → object storage, Querier fan-out, Compactor for downsampling) or **Mimir**/Cortex for a horizontally-scalable multi-tenant backend, or managed (AWS AMP, Azure Monitor managed Prometheus, Grafana Cloud). Also mention **VictoriaMetrics** as a lower-resource alternative. The core point: Prometheus local storage is not durable long-term storage.

**14. What is the Watchdog / dead man's switch?** An always-firing alert routed to an external system that pages you if it *stops* arriving — this is how you detect that monitoring itself died. Interviewers love this because it shows you think about failure of the observability stack.

**15. How do you monitor Kubernetes specifically?** kube-state-metrics (object state: replicas desired vs ready, pod phase, restarts), cAdvisor via kubelet (container resource usage), node-exporter (host metrics), and control-plane metrics (apiserver latency, etcd, scheduler). Deploy with kube-prometheus-stack; `ServiceMonitor`/`PodMonitor` CRDs for discovery. Know the difference between kube-state-metrics (what K8s *thinks*) and cAdvisor (what's actually consumed).

---

## C. Grafana

**16. What makes a dashboard actually useful?** Designed for a question, not for decoration. Top row = the SLIs a responder checks first (error rate, latency, traffic, saturation), then drill-downs. Consistent units and thresholds, template variables for env/cluster/service so one dashboard serves everything, annotations for deploys and incidents (so correlation is visible instantly), and links to runbooks. Anti-patterns: 60 panels nobody reads, per-pod graphs at scale, and dashboards that require tribal knowledge to interpret.

**17. Dashboards as code.** Provision via JSON/Jsonnet (grafonnet), Terraform provider, or the Grafana Operator so dashboards are versioned and reviewable, not hand-edited and lost. Also: folder permissions, and library panels for reuse.

**18. What is exemplars / correlation between pillars?** Exemplars attach a trace ID to a metric sample so you can click a latency spike and jump straight to a representative trace, then to that request's logs. Metrics → traces → logs correlation is the practical definition of a mature observability setup, and Grafana's Explore + Tempo/Loki integration is how you demo it.

---

## D. Logging & ELK

**19. Design a centralized logging pipeline for Kubernetes.** Apps log **structured JSON to stdout** (never to files inside containers, never invent your own rotation). A DaemonSet collector (Fluent Bit — lightweight — or Fluentd/Vector/OTel Collector) tails `/var/log/containers`, enriches with Kubernetes metadata (namespace, pod, labels), parses and drops noise, then ships to a buffered transport (Kafka or Redis for backpressure and spike absorption) → Logstash for heavier transforms → Elasticsearch/OpenSearch → Kibana. Then: ILM policies (hot/warm/cold/delete), index templates with explicit mappings, and per-team index patterns with RBAC.

**20. Why put Kafka between the collector and Elasticsearch?** Backpressure. A log spike or an Elasticsearch slowdown shouldn't cause the collector to drop logs or OOM the node. Kafka decouples producer rate from consumer rate, allows replay, and lets multiple consumers (e.g. security SIEM + ELK) read the same stream.

**21. Elasticsearch concepts you should know.** Index → shards (primary + replica) → segments. Shard sizing matters: aim for tens of GB per shard; too many small shards wastes heap ("oversharding") and too few limits parallelism. Node roles (master, data hot/warm, ingest, coordinating), and heap ≤ 50% of RAM and under ~31GB (compressed object pointers). ILM to roll over by size/age and move to cheaper tiers. Explicit mappings to prevent field explosion from dynamic mapping — a JSON log with variable keys can blow up your cluster the same way high cardinality kills Prometheus.

**22. How do you control logging costs?** Sample high-volume low-value logs (health checks, successful 200s), log levels controlled at runtime, drop debug in prod, retention tiering (7 days hot, 30 warm, 90 archived to object storage), don't log full request/response bodies, and dedupe stack traces. Also: put a per-team volume budget and a dashboard on ingestion GB/day — logging is often the #1 or #2 observability line item and the cheapest thing to fix.

**23. Loki vs Elasticsearch — trade-off?** Loki indexes only labels and stores compressed chunks in object storage: far cheaper, simpler operationally, great when you know the labels to filter by (it fits the Prometheus label model). Elasticsearch does full-text search and complex aggregations well but costs more to run. If your workflow is "filter by service, then grep," Loki wins; if it's forensic full-text search and analytics, Elasticsearch does.

**24. What is OpenTelemetry and why does it matter now?** A vendor-neutral standard (API, SDK, protocol OTLP, and Collector) for traces, metrics, and logs. It decouples instrumentation from backend, so you can switch vendors without re-instrumenting. The **Collector** is the practical win: one agent to receive, process (batch, filter, redact PII, tail-sample), and export to multiple backends. This is the correct "modern observability" answer in 2026 — say you'd standardize on OTel for new instrumentation.

**25. Explain distributed tracing.** A trace is a tree of spans across services, correlated by a propagated **trace context** (W3C `traceparent`). It answers "where did the 900ms go?" — which service, which DB call. Requires context propagation through every hop *including* async/queue boundaries (the common gap). Sampling: head-based (decide at ingress, cheap, may miss rare errors) vs **tail-based** (decide after seeing the whole trace, so you can keep all errors and slow traces — much better signal, needs a Collector). Traces are how you debug latency; metrics tell you *that* it's slow, traces tell you *where*.

---

## E. Scenario Questions

**26. You're on-call and get 40 alerts in 5 minutes. What do you do?** Don't triage alert-by-alert — look for the **common cause** and the top of the dependency graph. Check what all alerts share (one cluster? one AZ? one dependency? one deploy?). Identify user impact first (are SLOs burning?), declare an incident, assign roles (IC, comms, ops), and silence the downstream noise so you can think. Then check the timeline for changes. Afterwards: the 40 alerts are themselves a bug — add inhibition rules so a single root cause pages once.

**27. How would you instrument a service that currently has no observability?** Start with the outside: LB/ingress metrics give you RED signals with zero code change, plus synthetic probes for availability. Then add in-app: OTel auto-instrumentation for HTTP/DB spans, a `/metrics` endpoint with request counter + duration histogram labelled by route/method/status (bounded labels only), structured JSON logs with trace IDs, and a `/health` (shallow) + `/ready` (deep) split. Define one SLO with the product owner. Then dashboards and burn-rate alerts. Order matters: **SLI first, then dashboards, then alerts** — most teams do it backwards.

**28. How do you monitor a batch job / cron?** Alert on *absence* of success (`absent_over_time(job_last_success_timestamp[26h])` or a heartbeat/dead-man's-switch), on duration exceeding a threshold, and on records-processed anomalies. A cron that silently stops running produces no error metrics at all — which is why absence alerting matters.

**29. What would you do if Grafana showed a metric gap for 10 minutes?** Distinguish "the system was down" from "collection was down" — check `up`, scrape duration, Prometheus restarts/WAL replay, target discovery changes, and network policy changes. Then fix the observability gap and add an alert on scrape failure, because a gap that looks like an outage wastes on-call time and a gap that *hides* an outage is worse.

**30. How do you observe a service mesh / very large cluster without blowing up costs?** Drop unused metrics at scrape time (`metric_relabel_configs`), reduce Envoy's default enormous metric set, use recording rules + downsampling (Thanos compactor), longer scrape intervals for slow-moving metrics, tail-based sampling for traces (keep 100% of errors, 1% of successes), and log sampling. Then measure the cost of observability per service and treat it as a budget line, not an infinite good.
