# Part 4 — CI/CD: Jenkins, GitHub Actions, Azure DevOps

---

## A. Fundamentals

**1. CI vs CD vs Continuous Deployment.** CI = every commit merges to trunk and is built + tested automatically (the point is *integration frequency*, not the tool). Continuous **Delivery** = every green build is releasable, with a human gate for release. Continuous **Deployment** = that gate is removed and green builds go to prod automatically. Most orgs claim CD and actually do CI + manual release.

**2. What are the DORA metrics and why do they matter in an interview?** Deployment frequency, lead time for changes, change failure rate, MTTR (plus reliability as the fifth). They matter because they let you argue about pipeline work in business terms: "we cut lead time from 5 days to 4 hours." Have your own numbers ready.

**3. What makes a good pipeline?** Fast (feedback in <10 min for CI), **hermetic** (same input → same output, no shared mutable state), fail-fast ordering (lint → unit → build → integration → deploy), artifact built **once** and promoted by digest across environments, idempotent and re-runnable, secure by default (OIDC not static keys, least privilege), and observable (you can answer "why is main red?" in 30 seconds).

**4. Why "build once, promote everywhere"?** Rebuilding per environment means dev/staging/prod can differ (different base image pull, different dependency resolution), so your testing doesn't prove anything about prod. Build one immutable artifact, tag with the git SHA, and promote the same digest. Environment differences live in config, injected at runtime.

**5. Trunk-based development vs GitFlow.** Trunk-based: short-lived branches, merge to main daily, feature flags for incomplete work, release from main. GitFlow: long-lived develop/release/hotfix branches — heavy merge overhead, slow lead time, but sometimes required for versioned/on-prem products. For a SaaS platform, trunk-based + feature flags is the defensible answer.

**6. Explain feature flags and why they're a deployment strategy.** They decouple **deploy** from **release**. You can ship dark, enable for 1% of users, and kill instantly without a rollback deploy. Trade-off: flag debt and combinatorial test surface — so flags need owners and expiry dates.

---

## B. Deployment Strategies

**7. Rolling, blue/green, canary, A/B — compare them.**
- **Rolling**: replace instances incrementally. Cheap (no double capacity), but both versions serve simultaneously and rollback is another rolling update. K8s default.
- **Blue/green**: full second environment, switch traffic at the LB/DNS. Instant rollback, clean separation, but ~2× cost during the switch and hard with stateful/DB changes.
- **Canary**: route a small % to the new version, watch SLIs, progressively increase or auto-abort. Best risk/cost balance, needs good metrics and automation (Argo Rollouts, Flagger).
- **A/B**: like canary but partitioned by user attribute for *product* measurement, not safety.
Say which you'd choose and why: canary for user-facing APIs with strong metrics, blue/green when you need atomic cutover and can afford the capacity.

**8. How do you achieve genuine zero-downtime deployment?** Six things must all be true: (a) readiness probes gate traffic, (b) graceful shutdown — `preStop` drain + SIGTERM handling + grace period > LB deregistration, (c) `maxUnavailable: 0` with surge capacity, (d) **backward-compatible database migrations** (expand/contract), (e) backward-compatible API contracts so old and new versions coexist, (f) connection draining at the LB. Most candidates name (a) and (c) only; (d) and (e) are where real outages come from.

**9. Explain expand/contract (parallel change) for DB migrations.** Phase 1 **expand**: add the new nullable column/table, deploy code that writes both and reads old. Phase 2 **migrate**: backfill. Phase 3: deploy code reading new. Phase 4 **contract**: stop writing old, then drop it — in a *later* release. Every phase is independently deployable and reversible. Never combine a destructive schema change with a code deploy, and never make a rollback require a down-migration.

**10. How do you roll back a deployment that included a DB migration?** You usually *can't* roll the schema back safely, so design so you don't have to: additive-only migrations, decoupled from the app deploy, and roll *forward* to fix. If data was destroyed, you're in restore-from-backup territory — which is why the destructive step is separated by a release boundary.

---

## C. Jenkins

**11. Declarative vs scripted pipeline.** Declarative (`pipeline { }`) is structured, validated, more readable, supports `options`/`when`/`post` cleanly — the default choice. Scripted (`node { }`) is full Groovy for complex dynamic logic. You can escape into `script { }` blocks from declarative.

**12. How do you scale Jenkins properly?** Controller does orchestration only, **zero builds on the controller**. Ephemeral agents via the Kubernetes plugin: one pod per build, defined as a `podTemplate` with per-container tool images. Benefits: no snowflake agents, no cross-build contamination, elastic cost. Add: Configuration-as-Code (JCasC) so the controller is reproducible, job DSL/multibranch for pipeline provisioning, and shared libraries for reusable logic.

**13. What are Jenkins shared libraries?** Versioned Groovy in a separate repo (`vars/`, `src/`) loaded with `@Library`. They let you define an opinionated `standardBuild()` used by 100 repos, so a fix or new security scan lands everywhere at once. Version them and test them — an untested shared library is a single point of failure for all pipelines.

**14. How do you handle secrets in Jenkins?** Credentials plugin with `withCredentials` (masked in logs), scoped to folders, never `env` at global scope. Better: Vault/cloud-secret integration fetching short-lived credentials at build time, and cloud access via OIDC/IRSA rather than stored keys. Also disable `Script Approval` abuse and lock down who can edit Jenkinsfiles for prod-deploying jobs.

**15. What are Jenkins' weaknesses and when would you migrate off it?** Stateful controller, plugin sprawl and CVE surface, Groovy sandbox pain, upgrade risk, scaling the controller, and pipeline logic that becomes a bespoke platform. Migrate when your workloads fit a managed runner model — GitHub Actions or Azure DevOps — and you'd rather maintain workflows than a Jenkins estate. Be balanced: Jenkins' flexibility and self-hosting are genuine advantages in regulated/air-gapped environments.

**16. Common Jenkins interview specifics.** `agent { label }` and `agent none` with per-stage agents; `options { timeout, retry, buildDiscarder, disableConcurrentBuilds }`; `post { always/success/failure/unstable }`; `input` for approvals (use `submitter`); `lock()` for serializing access to a shared resource like a Terraform state; `parallel` stages; `stash`/`unstash` for passing artifacts between agents; `when { branch }`/`changeset` for conditional stages.

---

## D. GitHub Actions

**17. Anatomy of a workflow.** `on` (triggers: push, pull_request, schedule, workflow_dispatch, workflow_call, repository_dispatch) → `jobs` (parallel by default, `needs` for DAG ordering) → `steps` (`uses` an action or `run` a shell command) → `runs-on` runner. Jobs run on separate runners, so passing data requires artifacts, outputs, or a cache.

**18. How do you authenticate to AWS/Azure from Actions without secrets?** OIDC: `permissions: id-token: write`, then `aws-actions/configure-aws-credentials` with `role-to-assume` (trust policy conditioned on `repo:org/repo:ref:refs/heads/main` — condition on the ref/environment, or any repo in the org can assume it) or `azure/login` with workload identity federation. This is the expected answer; long-lived `AWS_ACCESS_KEY_ID` secrets are a finding.

**19. `GITHUB_TOKEN` permissions and supply-chain hardening.** Set `permissions: contents: read` at workflow level, elevate per-job only as needed. Pin third-party actions to a **full commit SHA**, not a tag (tags are mutable — this is how the `tj-actions/changed-files` class of compromise propagated). Use `pull_request_target` with extreme care (it runs with secrets against untrusted code). Enable Dependabot for actions, and consider an allow-list of permitted actions org-wide.

**20. Reusable workflows vs composite actions.** Reusable workflow (`workflow_call`) = a whole job/pipeline shared across repos, supports secrets/inputs and its own runners — use for "the standard build-and-deploy." Composite action = a bundle of steps inside one job — use for "the standard setup steps." Centralize in a `.github` or platform repo and version by tag.

**21. Environments, approvals, and concurrency.** `environment: production` gives required reviewers, wait timers, branch restrictions, and environment-scoped secrets. `concurrency: { group: deploy-prod, cancel-in-progress: false }` prevents overlapping deploys — the Actions equivalent of Jenkins `lock()`. Use `cancel-in-progress: true` for PR CI to save runner minutes.

**22. Self-hosted vs GitHub-hosted runners.** Self-hosted for private network access, custom hardware/GPU, large caches, or cost at scale — but you own patching and, critically, **isolation**: never run self-hosted runners on public repos (untrusted PR code executes on your runner). Use ephemeral runners (ARC — Actions Runner Controller on K8s) so each job gets a clean pod.

**23. Caching and matrix builds.** `actions/cache` with a key on the lockfile hash and sensible `restore-keys`. Matrix (`strategy.matrix`) for multi-version/multi-OS testing with `fail-fast: false` and `max-parallel`. Mention cache poisoning risk across branches.

---

## E. Azure DevOps

**24. Azure Pipelines structure.** YAML pipeline → stages → jobs → steps/tasks, running on agent pools (Microsoft-hosted or self-hosted). `templates` for reuse (`extends` for enforced structure, `template` for includes), `variables` and **variable groups** linked to Key Vault, and `parameters` with types for typed inputs.

**25. Environments, approvals, and gates in ADO.** `environment` resources give deployment history, approvals, and **checks** (branch control, business hours, Azure Monitor alert gate, invoke REST API, exclusive lock). `deployment` jobs support strategies: `runOnce`, `rolling`, `canary`, with `preDeploy`/`deploy`/`routeTraffic`/`postRouteTraffic`/`on: failure` lifecycle hooks — a genuinely nice built-in for progressive delivery.

**26. Service connections — the security answer.** Prefer **workload identity federation** service connections (OIDC, no stored secret) over service-principal-with-secret. Scope to a resource group, not a subscription. Restrict which pipelines can use a connection ("Grant access permission to all pipelines" = off), and require approval for connection use in prod.

**27. Classic release pipelines vs YAML.** Classic is UI-defined, not version-controlled, and hard to review — migrate to YAML so pipelines are code, reviewed, branch-aware, and reproducible.

**28. Azure Artifacts / feeds and upstream sources.** Host internal packages and proxy public registries as **upstream sources** — gives you availability if upstream is down, plus a chokepoint for scanning and version policy.

---

## F. Scenario / Design Questions

**29. Design a CI/CD pipeline for 40 microservices on Kubernetes.** Structure the answer: (1) **CI per service** — lint, unit tests, build image tagged with git SHA, SBOM, scan, sign with cosign, push to registry. (2) **CD via GitOps** — CI's last step opens a PR (or commits) updating the image digest in a config repo; ArgoCD syncs. This gives an audit trail and separates build permissions from cluster permissions (CI never needs cluster credentials — a major security win worth stating). (3) **Standardization** — one reusable workflow / shared library so all 40 services get the same gates; a Helm base chart with sane defaults (probes, PDB, resources, topology spread). (4) **Progressive delivery** — Argo Rollouts canary with automated analysis on error rate/latency, auto-rollback. (5) **Environments** — dev auto-deploy, staging auto, prod gated. (6) **Observability** — deploy markers in Grafana, DORA metrics dashboard. (7) **Scale concern** — 40 services × N envs means templating and drift control matter more than any individual pipeline.

**30. Your pipeline takes 45 minutes. How do you get it under 10?** Measure first — instrument stage durations, don't guess. Then: parallelize independent jobs; split the test suite and shard it; run only affected tests/services (`nx affected`, `bazel`, path filters) in a monorepo; cache dependencies and Docker layers (registry-backed so ephemeral agents benefit); move slow integration/e2e tests off the PR path into a post-merge or nightly gate with clear ownership; use bigger runners for the critical path (runner cost is cheaper than engineer wait time — make that argument explicitly); build with BuildKit and `--cache-from`; and remove serialized approval waits from the *measured* CI time.

**31. How do you prevent a bad build from reaching production?** Layered gates: unit + integration coverage thresholds, contract tests between services, security scans (SAST/dependency/IaC/image) failing on policy, canary with automated SLI analysis and auto-rollback, and post-deploy smoke tests asserting the deployed SHA. Plus: change freeze rules for high-risk windows and a rollback that's a single command everyone knows. Emphasize that speed of *recovery* matters as much as prevention.

**32. How do you handle a monorepo vs polyrepo in CI?** Monorepo: path filters / affected-graph builds, one shared toolchain, atomic cross-service changes, but needs investment in build tooling and can hit CI scale limits. Polyrepo: simple independent pipelines, but version skew, duplicated pipeline logic (solve with reusable workflows), and painful cross-repo changes. Choose by team topology and whether you need atomic cross-service changes.

**33. How do you manage environment configuration across dev/staging/prod?** Config outside the artifact: Helm values per environment (in a GitOps repo), or ConfigMaps generated by Kustomize overlays, secrets from Key Vault/Secrets Manager via External Secrets. Same image digest everywhere. Validate config in CI (schema-check values files) so a typo doesn't reach prod, and diff environments regularly to catch drift.

**34. A developer says "CI is blocking me, let me deploy manually just this once." What do you do?** Short-term: pair with them, use the documented break-glass path (which exists, is audited, and requires a second approver), and get the release out. Then treat the root cause as a platform bug — measure why CI was blocking, fix it, and remove the incentive to bypass. The senior framing: if the compliant path is slower than the non-compliant one, people will route around your controls; the answer is to make the safe path fast, not to add more process.

**35. How would you introduce CI/CD into a team that deploys manually once a month?** Start with observability of the current process (how long, how many steps, what fails), automate the build first (lowest risk, immediate value), then automate deploy to a *non-prod* environment to build trust, add tests around the riskiest path, then introduce a gated prod deploy that's identical to the staging one. Ship the change incrementally with a visible metric (lead time) and a rollback story. Resistance is usually about trust, not tooling — so demonstrate reversibility early.
