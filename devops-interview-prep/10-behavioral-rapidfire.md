# Part 10 — Behavioral Stories & Rapid-Fire Round

---

## A. The 6 stories you must have written down

Use **STAR**: Situation (1 sentence), Task (your specific responsibility), Action (what *you* did, with technical detail), Result (**with a number**). Then add a **Learning** — senior candidates always close with what they'd do differently.

Write these out in full. Don't improvise them in the room.

**1. The worst incident you owned.**
Must include: how you were alerted, how you scoped impact, the mitigation *before* root cause, the actual root cause, MTTR, and the prevention items that shipped. Avoid blaming a person or another team. If you caused it, say so — owning a mistake is a strong signal, not a weak one.

**2. Something you automated that removed real toil.**
Include the before/after numbers (hours per month, error rate, lead time), why you chose to automate *that* thing, and how you made the automation safe (dry-run, idempotent, alerting on its failure).

**3. A time you disagreed with a technical decision.**
Interviewers test whether you can disagree productively. Show: you gathered evidence, you framed it in terms of risk/cost rather than preference, you committed to the outcome even when you lost. "I was overruled, I supported the decision, and here's what I learned when it turned out I was partly right/wrong" is a great answer.

**4. A time you pushed back on the business / said no.**
Usually about shipping something unsafe under deadline pressure. Show: you offered an alternative (feature flag, phased rollout, reduced scope) rather than just refusing, and you made the risk explicit and let the business own the decision.

**5. Something you migrated or modernized.**
Include: why now (what pain justified it), how you de-risked it (parallel run, canary, rollback), how you brought other teams along, and what it cost. Bonus: something you deliberately *didn't* migrate.

**6. A time you were wrong / an outage you caused.**
The trap answer is "I can't think of one." Have a real one, tell it plainly, focus on what the *system* let you do wrong (no guardrail, no review, no dry-run) and what guardrail you added. Blameless framing applied to yourself.

### Also prepare short answers for:
- Mentoring a junior / raising the team's bar
- Improving on-call quality of life (real numbers on pages per shift)
- Working with a difficult stakeholder
- A project that failed or was cancelled
- How you keep up technically
- Why you're leaving / why this role

---

## B. Questions you should ask *them*

Asking good questions is graded. Pick 4–5:

- What does on-call look like — rotation size, pages per shift, and who owns which services?
- What are your DORA metrics today, and what's the biggest constraint on lead time?
- How do you handle postmortems? Can you describe a recent one?
- What's the split between platform work and firefighting for this role?
- What's the most painful piece of tech debt, and is there appetite to fix it?
- How is reliability prioritized against features — do you use error budgets?
- What would success look like for me at 3 months and 12 months?
- Who makes architecture decisions, and how are they documented?
- What's the biggest thing you'd change about the platform if you had a free quarter?

**Red flags to listen for:** no postmortems, no SLOs, "we're firefighting a lot right now," one person who knows everything, no staging environment, manual prod deploys, or an on-call rotation of two people.

---

## C. Rapid-fire round — 100 one-liners

Cover these in under 15 seconds each. If you hesitate, mark it and study it.

### Kubernetes (1–25)
1. Pod vs container — pod is the scheduling unit, shares network/IPC/storage namespaces.
2. Why do pods have one IP? Shared network namespace via the pause container.
3. What's the pause container? Holds the network namespace so containers can restart without losing the IP.
4. Deployment vs StatefulSet — interchangeable vs stable identity + per-pod storage.
5. Headless Service — `clusterIP: None`, DNS returns pod IPs.
6. What does kube-proxy do? Programs iptables/IPVS/eBPF rules for Service VIPs.
7. Readiness fails — pod removed from endpoints, not restarted.
8. Liveness fails — container restarted.
9. Exit 137 / 143 — SIGKILL (often OOM) / SIGTERM.
10. CrashLoopBackOff max backoff — 5 minutes.
11. QoS classes — Guaranteed, Burstable, BestEffort.
12. What evicts first? BestEffort.
13. `requests` affect what? Scheduling.
14. `limits` affect what? Runtime enforcement (throttle/OOMKill).
15. PDB protects against — voluntary disruptions only.
16. Taint vs affinity — node repels vs pod attracts.
17. `topologySpreadConstraints` — spread replicas across a topology key.
18. ConfigMap max size — ~1MB (etcd limit ~1.5MB).
19. Are Secrets encrypted? Base64 only unless you enable encryption at rest.
20. IRSA / Workload Identity — pod-level cloud identity via OIDC, no static keys.
21. `automountServiceAccountToken` default — true; set it false.
22. What's an operator? A controller + CRD encoding operational knowledge.
23. CRD vs aggregated API server — declarative extension vs custom API implementation.
24. Sidecar in 1.29+ — init container with `restartPolicy: Always`.
25. `kubectl debug` — ephemeral container, essential for distroless.

### Docker (26–35)
26. Image vs container — immutable layers vs running instance with a writable layer.
27. `CMD` vs `ENTRYPOINT` — default args vs the executable.
28. Multi-stage build — build deps stay out of the runtime image.
29. Why does layer order matter? Cache invalidation cascades downward.
30. Deleting a file in a later layer — doesn't reduce image size.
31. `.dockerignore` — shrinks build context, speeds builds, avoids leaking `.git`.
32. Why not run as root? Container escape and host file ownership risk.
33. What is BuildKit? Modern builder: parallelism, cache mounts, secret mounts.
34. Distroless — no shell/package manager, minimal CVE surface.
35. Signals and PID 1 — shell-form ENTRYPOINT swallows SIGTERM.

### Terraform (36–50)
36. What is state? Mapping of config addresses to real resource IDs.
37. Why remote state? Team access, locking, versioning, no secrets on laptops.
38. S3 backend locking — DynamoDB item (or native S3 lockfile).
39. `force-unlock` risk — two concurrent writers corrupt state.
40. `count` vs `for_each` — positional (fragile) vs keyed (surgical).
41. `moved` block — change resource address without touching infra.
42. `prevent_destroy` — guardrail on stateful resources.
43. `ignore_changes` — for fields another system owns.
44. `create_before_destroy` — zero-downtime replacement.
45. `depends_on` — only for hidden dependencies.
46. Why avoid provisioners? Non-idempotent, untracked, breaks the model.
47. `terraform import` — bring existing infra under management.
48. Drift — reality diverging from state; detect with scheduled plan.
49. Lock file — pins provider versions and checksums; commit it.
50. Workspaces caveat — easy to apply to the wrong environment.

### CI/CD (51–62)
51. CI vs CD vs Continuous Deployment — integration / releasable / auto-released.
52. DORA metrics — deploy frequency, lead time, change failure rate, MTTR.
53. Build once, promote — same digest through every environment.
54. Blue/green vs canary — atomic switch vs progressive percentage.
55. Zero-downtime prerequisites — readiness, graceful shutdown, backward-compatible schema and API.
56. Expand/contract — additive migration across multiple releases.
57. Feature flags — decouple deploy from release.
58. Pin GitHub Actions to — a full commit SHA, not a tag.
59. OIDC in CI — short-lived cloud credentials, no stored keys.
60. Jenkins scaling — ephemeral K8s agents, no builds on the controller.
61. Jenkins shared library — versioned reusable pipeline code.
62. ADO exclusive lock / GHA concurrency — serialize deploys.

### Cloud (63–75)
63. SG vs NACL — stateful/instance vs stateless/subnet.
64. Why one NAT per AZ? HA plus avoiding cross-AZ charges.
65. ALB vs NLB — L7 features vs L4 throughput and static IP.
66. Multi-AZ vs read replica — HA (sync) vs read scale (async).
67. RTO vs RPO — downtime tolerance vs data-loss tolerance.
68. DR patterns — backup/restore, pilot light, warm standby, active/active.
69. Spot requirements — diversify, handle interruption notice, keep on-demand baseline.
70. gp2 vs gp3 — gp3 decouples IOPS from size and is usually cheaper.
71. VPC endpoint benefit — private path to PaaS, cuts NAT cost.
72. Private Endpoint gotcha — DNS must resolve to the private IP.
73. SCP vs IAM policy — org guardrail vs identity permission.
74. Azure Policy — audit/deny/remediate at management-group scope.
75. Managed identity — Azure workload auth with no secret.

### Observability (76–88)
76. Golden Signals — latency, traffic, errors, saturation.
77. USE — utilization, saturation, errors.
78. SLI/SLO/SLA — measure / target / contract.
79. Error budget — 1 − SLO; makes reliability a spendable resource.
80. Burn-rate alerting — multi-window, symptom-based, high precision.
81. Counter vs gauge — monotonic vs up-and-down.
82. Histogram vs summary — server-side aggregatable vs client-side quantiles.
83. `rate()` before `sum()` — always.
84. Cardinality — unique label combinations; unbounded labels kill Prometheus.
85. Recording rule — pre-computed series for speed and reuse.
86. Thanos/Mimir — long-term storage and global query for Prometheus.
87. Watchdog alert — dead man's switch that detects monitoring failure.
88. Tail-based sampling — decide after seeing the whole trace; keeps all errors.

### GitOps / Security / FinOps (89–100)
89. GitOps principles — declarative, versioned, pulled, continuously reconciled.
90. Synced vs Healthy — manifests match Git vs workload actually working.
91. ApplicationSet — templated Applications from generators.
92. Sync waves — ordering (CRDs before CRs, migrations before app).
93. GitOps rollback — `git revert`, not a manual patch.
94. Secrets in GitOps — External Secrets Operator (reference only in Git).
95. Argo Rollouts — canary/blue-green with metric-driven auto-rollback.
96. SAST vs SCA vs DAST — code / dependencies / running app.
97. SBOM — inventory that answers "are we affected by this CVE?" in minutes.
98. cosign / SLSA — artifact signing and build provenance, verified at admission.
99. Biggest K8s cost waste — gap between requests and actual usage.
100. FinOps phases — Inform, Optimize, Operate; report **unit cost**, not just spend.

---

## D. Final 60 seconds before you walk in

- Lead every troubleshooting answer with **scope → what changed → mitigate → then diagnose.**
- Lead every design answer with **clarifying questions**, and close with **trade-offs**.
- Every answer ends with **how you'd prevent it** or **how you'd measure it**.
- Use real numbers from your own experience.
- If you don't know something: say so, then say how you'd find out. That scores far better than bluffing — and experienced interviewers can always tell.
