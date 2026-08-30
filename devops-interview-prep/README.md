# DevOps / SRE Interview Prep Pack — Mid-to-Senior (4+ yrs)

Targeted at your stack: **Azure, AWS, Kubernetes, Docker, Terraform, Jenkins, GitHub Actions, Azure DevOps, Python, Bash, Prometheus, Grafana, ELK, ArgoCD.**

Every question here is written the way a Principal Engineer / SRE Manager actually asks it — scenario-first, follow-up-heavy — and each one has a **senior-level model answer**, not a textbook definition.

## Files

| File | Contents | Questions |
|---|---|---|
| `01-troubleshooting.md` | Deep-dive incident scenarios with full diagnostic walkthroughs | 15 |
| `02-kubernetes-docker.md` | K8s internals, scheduling, networking, probes, Docker optimization | 40 |
| `03-terraform-iac.md` | State, locking, modules, drift, imports, CI patterns | 30 |
| `04-cicd.md` | Jenkins, GitHub Actions, Azure DevOps, deployment strategies | 35 |
| `05-cloud-aws-azure.md` | VPC/VNet, IAM/Entra, LB, HA, managed K8s, DR | 40 |
| `06-observability.md` | PromQL, alerting, SLO/SLI, cardinality, ELK pipeline design | 30 |
| `07-python-bash.md` | Live coding tasks + toil-automation patterns, with solutions | 20 |
| `08-gitops-devsecops-finops.md` | ArgoCD, supply-chain security, secrets, cost optimization | 30 |
| `09-architecture-scenarios.md` | Whiteboard design problems with model architectures | 10 |
| `10-behavioral-rapidfire.md` | Leadership/incident stories + 100 rapid-fire one-liners | 100+ |
| `11-most-asked-2026.md` | Web-researched "most asked" analysis + **Linux troubleshooting** + core theory | 45+ |

## The two frameworks that win interviews

### 1. Troubleshooting: the "USE + Layer Walk" method

Never start with commands. Start with a **structure**, then name commands as evidence-gathering steps. Interviewers grade the structure.

```
1. SCOPE      What's the blast radius? One pod / one node / one AZ / whole region?
              Is it user-visible? What's the error rate and SLO burn?
2. TIMELINE   What changed? Deploy, config, cert rotation, scale event, infra change?
              (~70% of real incidents are change-induced. Always ask this second.)
3. MITIGATE   Stop the bleeding BEFORE root-causing. Rollback / scale / failover /
              feature-flag off. Say this out loud — juniors debug, seniors mitigate first.
4. LAYER WALK Walk the request path top-down or bottom-up, and prove each hop:
              DNS -> LB/Ingress -> Service -> Pod -> Container -> App -> Dependency
5. USE CHECK  For each suspect resource: Utilization, Saturation, Errors.
              (CPU, memory, disk I/O, network, connection pools, thread pools.)
6. ROOT CAUSE Distinguish trigger from cause. "The deploy triggered it; the cause was
              an unbounded cache with no memory limit headroom."
7. PREVENT    Action items: alert, limit, test, guardrail, runbook. Always close here.
```

**Say the phrase "let me stop the bleeding first"** in every troubleshooting answer. It's the single highest-signal sentence in an SRE interview.

### 2. Architecture: the "RRSCO" pass

For any design question, cover these five in order and you'll never look junior:

- **R**equirements — traffic, RPS, data size, latency budget, RPO/RTO, compliance
- **R**eliability — failure domains, AZ/region spread, health checks, degradation modes
- **S**calability — stateless tier, autoscaling signal, data tier bottleneck
- **C**ost — instance strategy, storage tiering, egress, what you'd cut first
- **O**perations — deploy strategy, observability, secrets, IaC, on-call burden

Always end with: *"and here's the trade-off I'm consciously accepting…"* Naming trade-offs is the clearest senior signal there is.

## Suggested 2-week plan

| Days | Focus |
|---|---|
| 1–2 | `01-troubleshooting.md` — read all 15, then re-answer from memory out loud |
| 3–4 | `02-kubernetes-docker.md` — expect 30–40% of your interview here |
| 5 | `03-terraform-iac.md` |
| 6 | `04-cicd.md` |
| 7 | `05-cloud-aws-azure.md` (pick your primary cloud, skim the other) |
| 8 | `06-observability.md` — practise writing PromQL on a whiteboard |
| 9 | `07-python-bash.md` — actually type the solutions, don't just read |
| 10 | `08-gitops-devsecops-finops.md` — highest-differentiation section for 2026 |
| 11–12 | `09-architecture-scenarios.md` — 45 min each, timed, drawn on paper |
| 13 | `10-behavioral-rapidfire.md` — write your 6 STAR stories properly |
| 14 | `11-most-asked-2026.md` — Linux scenarios + theory drill, then a full timed mock (1 troubleshooting + 1 design + 1 trend question) |

If you only have 3 days: read `01-troubleshooting.md`, the Linux section of `11-most-asked-2026.md`, and the rapid-fire list in `10-behavioral-rapidfire.md`. That's the highest-yield subset.

## How to practise these properly

1. **Read the question, close the file, answer out loud for 3–5 minutes.** Reading answers creates recognition, not recall. Interviews test recall.
2. **Record yourself once.** You'll hear the filler words and the missing structure immediately.
3. **For every answer, force yourself to add one trade-off and one "how I'd prevent it."** These are the two things mid-level candidates almost always omit.
4. **Have real numbers ready** from your own experience: cluster size, node count, RPS, deploy frequency, MTTR, cost saved, incident count. Specifics beat adjectives.

## Your "have these ready" cheat sheet

Before any interview, write down concrete answers to these about your own work — interviewers probe them constantly:

- Cluster scale: how many clusters, nodes, pods, namespaces, teams?
- Deploy frequency and lead time. Do you know your DORA metrics?
- Biggest incident you owned end-to-end, with MTTR and the actual root cause
- One thing you automated and the hours/dollars it saved (with the number)
- Your Terraform repo layout and how many state files
- How secrets are managed today, and what's wrong with it
- One piece of tech debt you argued to fix, and whether you won
