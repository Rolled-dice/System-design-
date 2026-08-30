# Part 1 — Deep-Dive Troubleshooting Scenarios

15 scenarios in the exact shape senior interviews use. Read the scenario, answer out loud, *then* read the model answer.

---

## S1. `CrashLoopBackOff` with exit code 137 after a routine deploy

**Scenario.** 02:14 Saturday. `payment-api` on EKS 1.28: 8 desired replicas, 3 Running (all >6h old), 5 in `CrashLoopBackOff`. A deploy went out at 01:55 (`v2.4.1` → `v2.4.2`). `kubectl describe` shows `Last State: Terminated, Reason: Error, Exit Code: 137`. App logs show a clean boot — DB connected, `"Server listening on :8080"` — then logs stop. No stack trace. CPU is low right before death.

### Model answer

**Leading hypothesis: the container is being OOMKilled.** Exit 137 = 128 + 9 = SIGKILL. Combined with (a) a clean boot then abrupt silence with no stack trace, (b) low CPU, and (c) old pods surviving while new ones die — this is the classic signature of the kubelet killing the container for exceeding its memory limit, *not* an application crash. An app crash gives you a stack trace; a SIGKILL gives you silence.

The "old pods survive, new pods die" detail is the key clue: it means the *baseline* memory footprint of `v2.4.2` at startup is now above the limit, or very close to it. The old pods have already faulted in their pages and are sitting stable; new pods hit the ceiling during warm-up (JIT, cache pre-load, connection pool allocation).

**Important nuance:** exit 137 is *not always* OOM. It's SIGKILL from any source. The two other real candidates:
- A failing **liveness probe** killing the container. But that shows `Reason: Error` with probe-failure events in `describe`, and the app was healthy on `:8080`, so less likely — though I'd still check the probe's `initialDelaySeconds` against the new version's slower startup.
- `terminationGracePeriodSeconds` expiring during shutdown, which is a symptom not a cause here.

**Ordered confirmation plan:**

1. `kubectl describe pod <pod>` — read the `State`/`Last State` block for `Reason: OOMKilled` specifically, and read the `Events` at the bottom for probe failures. This one command distinguishes my top two hypotheses.
2. `kubectl get events -n <ns> --sort-by=.lastTimestamp` — cluster-level view; also reveals node-level `MemoryPressure` and evictions, which would point to a *node* problem rather than a *pod limit* problem.
3. Check the node: `kubectl describe node <node>` for `MemoryPressure=True` and check `Allocatable` vs the sum of requests. If the node is oversubscribed, my mitigation changes completely.
4. `kubectl get deploy payment-api -o yaml` → compare `resources.requests/limits` between v2.4.1 and v2.4.2. **Diff the manifests, not just the image tag** — a "routine version bump" often quietly carries a chart/values change.
5. Prometheus, the decisive evidence:
   - `container_memory_working_set_bytes{pod=~"payment-api.*"}` vs `kube_pod_container_resource_limits{resource="memory"}` — working set is what the kernel OOM killer actually measures, *not* RSS.
   - `rate(container_memory_failcnt[5m])` and `kube_pod_container_status_last_terminated_reason{reason="OOMKilled"}`.
   - Plot v2.4.1 pods vs v2.4.2 pods on the same graph. If v2.4.2's curve is a steeper ramp to the same ceiling, it's confirmed.
6. `kubectl logs --previous` I've already got — but I'd also check whether the app has JVM/Go runtime settings. If it's a JVM without `-XX:MaxRAMPercentage` (or Node without `--max-old-space-size`), the runtime doesn't know about the cgroup limit and will happily allocate past it. That's a very common root cause of exactly this pattern.
7. Diff the actual change: `git diff v2.4.1..v2.4.2` looking for new caches, larger batch sizes, a new in-memory index, a dependency bump, or a base-image change (e.g. Alpine → Debian, or a glibc/musl malloc arena difference which can inflate RSS substantially).

**Immediate mitigation — stop the bleeding first.** This is a *payment* service at 02:14 and I have 3/8 capacity, so I mitigate before I finish root-causing:

1. **`kubectl rollout undo deployment/payment-api`** — this is the primary action. The change is 20 minutes old and correlates perfectly; reverting restores a known-good state in seconds. Rollback first, investigate in daylight. I'd verify with `kubectl rollout status`.
   - Before undoing, I capture forensics so the evidence isn't lost: save `describe` output, `logs --previous`, and if I need a heap dump I'd `kubectl cp` it off one crashing pod or scale the RS to keep one pod around.
2. **If rollback isn't possible** (e.g. an irreversible DB migration shipped with v2.4.2 — which I'd check immediately, because that changes everything): raise the memory limit as a stopgap, e.g. double it, and roll forward. This trades cost/node headroom for availability, which at 02:14 on payments is the right trade. I'd confirm the nodes have allocatable headroom first, otherwise I'll just trigger evictions and make it worse.
3. **Protect the 3 healthy pods.** They're carrying 100% of traffic. I'd confirm a `PodDisruptionBudget` exists so nothing evicts them, and check whether HPA is thrashing. I'd also verify those 3 aren't slowly heading for the same ceiling — if they are, I have minutes, not hours.
4. **Comms.** Declare an incident, post status, notify the payments stakeholder. A degraded payment path is a business event, not just a technical one.

**Root cause vs trigger.** The deploy is the *trigger*. The *cause* is that the service ran with no memory headroom and no guardrail against a footprint regression — v2.4.2 merely revealed it.

**Prevention (always close here):**
- Set `requests` = observed p95 working set, `limits` with real headroom (and understand that `requests != limits` for memory means Burstable QoS and eviction risk).
- Make the runtime cgroup-aware (`MaxRAMPercentage`, `GOMEMLIMIT`, `--max-old-space-size`).
- Alert on `container_memory_working_set_bytes / limit > 0.85` **before** OOM, and alert on any `OOMKilled` reason appearing at all.
- Add a memory-regression check to CI (load test the candidate image, fail the build if peak working set grows >10%).
- Use a canary or progressive rollout so 1 pod dies, not 5. With Argo Rollouts/flagger, an analysis step on restart count would have auto-aborted this.
- VPA in `recommender` mode to keep requests honest over time.

**Follow-ups the interviewer will ask:**
- *"How does the OOM killer choose the victim?"* — cgroup memory limit breach kills the process in that cgroup; system-wide pressure uses `oom_score_adj`, which the kubelet sets from QoS class (BestEffort dies first, then Burstable, Guaranteed last).
- *"Difference between OOMKilled and Evicted?"* — OOMKilled is the kernel enforcing a **container** cgroup limit (pod stays, container restarts). Eviction is the **kubelet** reclaiming a node under `MemoryPressure` (pod is terminated and rescheduled). Different mechanism, different fix.
- *"Why is working set the right metric, not RSS?"* — working set ≈ RSS + active page cache − reclaimable; it's what the kubelet compares against the limit.

---

## S2. Terraform state lock stuck after a CI job was killed

**Scenario.** A Jenkins `terraform apply` on the shared prod state was killed mid-run (agent OOM). Now every plan fails: `Error acquiring the state lock ... ID: 4f2c… Operation: OperationTypeApply ... Created: 25 minutes ago`. Three engineers are blocked and a release is waiting.

### Model answer

**First, understand what a lock actually protects.** With the S3 backend the lock is a DynamoDB item (`LockID = <bucket>/<key>-md5`); with `azurerm` it's a blob lease on the state file. Its job is to prevent two writers corrupting state. So the danger isn't the lock — it's **breaking the lock while a writer is genuinely still running.**

**So the ordered plan is: prove no writer is alive, then unlock.**

1. **Read the lock metadata.** The error message gives `Who`, `Created`, `Operation`, and the lock ID. `Operation: OperationTypeApply` is the scary one — an apply may have partially mutated real infrastructure.
2. **Prove the process is dead.** Find the Jenkins build from `Who`/timestamp, confirm the job is `ABORTED`/`FAILED` and the agent/pod no longer exists. If it's a K8s agent, confirm the pod is gone. **Never break a lock on assumption.** If the job might still be running, I wait or kill it deliberately first.
3. **Assess partial-apply damage.** Because it died mid-apply, state may not reflect reality. After unlocking I run `terraform plan` and read it as a *reconciliation report*: resources created in the cloud but absent from state will show as "to be created" and then fail with "already exists" — those need `terraform import` (or `import` blocks in TF ≥1.5). This step is what separates a senior answer; most candidates stop at "run force-unlock."
4. **Break the lock:** `terraform force-unlock <LOCK_ID>` (with `-force` in CI). Do it from the same backend config/workspace. Only if that fails do I go to the backend directly — delete the DynamoDB item with that `LockID`, or break the blob lease in Azure. Manual backend surgery is the last resort, and I'd announce it in the team channel first so nobody else applies concurrently.
5. **Verify:** `terraform plan` succeeds, state is readable, and I diff against expectation. If state itself looks damaged, recover from S3 object versioning (versioning + `prevent_destroy` should already be on) or the `.tfstate.backup`.
6. **Unblock the team** and only then do the release.

**Why it happened / prevention:**
- Kill-safety: set `-lock-timeout=5m` so concurrent runs queue instead of hard-failing, and make the CI job's termination grace period long enough for Terraform to release the lock on SIGTERM. Terraform *does* release the lock on graceful shutdown — it was SIGKILL that orphaned it.
- Right-size the CI agent (this was an agent OOM — fix the actual trigger).
- **Serialize applies per state**: Jenkins `lock()` resource / GitHub Actions concurrency group / Azure DevOps exclusive-lock check. Application-level serialization means you rarely touch the TF lock at all.
- **Split the monolithic prod state.** Three engineers blocked by one lock is an architecture smell. Split by blast radius (network / data / platform / per-service) so lock contention drops and apply times shrink.
- Use `terraform plan -out=tfplan` then `apply tfplan` with approval, so applies are short and deterministic.
- Consider Terraform Cloud/Spacelift/Atlantis or an OIDC-authenticated pipeline with a queue, which handles all of this natively.

**Follow-ups:** *"When is `force-unlock` dangerous?"* — when the writer is alive: you get two concurrent writers and genuinely corrupted state. *"How do you recover a corrupted state file?"* — S3 versioning rollback, `terraform state pull/push`, `state rm` + `import` to rebuild surgically. *"Why not `-lock=false`?"* — it doesn't remove the stale lock, it just disables safety for your run; it's for read-only plans at worst, never applies.

---

## S3. Intermittent 502s behind an AWS ALB — only ~2% of requests

**Scenario.** A public API behind an ALB → EKS ingress → pods. ~2% of requests return 502, no pattern by endpoint, latency looks normal, pods aren't restarting, and it started after a traffic increase.

### Model answer

**What a 502 means precisely:** the load balancer got an invalid/empty/closed response from the target. So the fault is between LB and target, or in the target's connection handling — not usually in the LB.

**Layer walk with the specific signals:**

1. **Confirm it's the LB's view.** ALB `HTTPCode_ELB_5XX_Count` vs `HTTPCode_Target_5XX_Count`. If ELB_5XX is high while Target_5XX is ~0, the target never returned a valid response — connection was reset or closed. That's the fingerprint of an idle-timeout/keep-alive mismatch, not an app bug.
2. **Enable/read ALB access logs.** Look at `target_status_code = -` and `elb_status_code = 502`, plus `target_processing_time`. `-` means no response received. This is the single most valuable artifact and most candidates forget ALB access logs exist.
3. **The #1 real cause of intermittent 502s: keep-alive idle timeout race.** The ALB's idle timeout (default 60s) must be **strictly less than** the target's keep-alive timeout. If the app closes an idle connection at the same moment the ALB reuses it, the ALB sees a reset → 502. Fix: set app/nginx `keepalive_timeout` to ~75s, above the ALB's 60s. This produces exactly this symptom: low single-digit percentage, no endpoint pattern, worsening with traffic (more connection reuse).
4. **Second cause: pod termination races.** During rollout/scale-in, a pod stops accepting connections before the ALB target group deregisters it. In-flight requests get RST → 502. Fixes: `preStop` sleep (~15–30s) so the pod keeps serving while deregistration propagates, `terminationGracePeriodSeconds` > deregistration delay, readiness gates (`pod-readiness-gate-inject` with AWS LB Controller), and app-level graceful shutdown that drains in-flight requests. The clue "started after traffic increase" fits if HPA is now scaling in/out frequently.
5. **Third: target-side limits.** Check app connection/thread pool saturation, `nginx` `worker_connections`, upstream queue overflow, and `conntrack` table exhaustion on nodes (`nf_conntrack: table full` in `dmesg`) — the latter appears exactly when traffic grows.
6. **Ephemeral port / SNAT exhaustion** if traffic egresses via NAT — check `ErrorPortAllocation` on the NAT Gateway.
7. **Header size / protocol mismatch** — a target returning >8KB headers, or gRPC/HTTP2 misconfiguration on the target group, gives deterministic 502s per-endpoint; the lack of endpoint pattern argues against it, so I'd deprioritize it.

**Mitigation while root-causing:** raise the app keep-alive above the ALB idle timeout (cheap, safe, usually fixes it outright), add `preStop` drain, and if it's rollout-correlated, pause deploys. Add a synthetic canary to measure whether the change worked, since 2% won't be visible by eyeball.

**Azure equivalent** (expect this follow-up): same class of bug on Application Gateway → AKS. Check backend health probe path/host, `backendAddressPool` membership, and that the AGIC-managed probes match the app; Front Door adds its own idle timeout. The reasoning is identical — the LB got no valid response.

**Prevention:** codify timeout ordering as an invariant (`client > LB idle > app keep-alive > upstream`), template `preStop`/graceful shutdown into the base Helm chart, alert on `HTTPCode_ELB_5XX_Count` as a *separate* SLI from target 5xx, and load-test rollouts (not just steady state).

---

## S4. Node `NotReady`, pods stuck `Terminating`

**Scenario.** One AKS node flips to `NotReady`. Its pods sit in `Terminating` for 20+ minutes. Other nodes are fine.

### Model answer

- **Understand the state machine first.** `NotReady` means the kubelet stopped heartbeating (`node-monitor-grace-period`, 40s). The node controller then taints it `node.kubernetes.io/unreachable:NoExecute`. Pods get a deletion timestamp but **can't complete deletion because the kubelet — the only thing that can confirm container teardown — is unreachable.** That's why they hang. This mechanical explanation is the answer; commands are secondary.
- **Triage:** `kubectl describe node` for conditions (`MemoryPressure`, `DiskPressure`, `PIDPressure`, `KubeletNotReady` message), then node-level: is the VM up (Azure portal / `az vm get-instance-view`)? Can I SSH / `kubectl debug node/<node>`? Check `systemctl status kubelet`, `journalctl -u kubelet`, containerd status, disk full (`df -h` — `/var/lib/containerd` filling is a top cause), and network path to the API server.
- **Common root causes:** disk exhaustion from image/log sprawl; kubelet OOM because system daemons had no reserved resources (`--kube-reserved`/`--system-reserved` unset); a node-level network/NSG/route change; underlying host maintenance; containerd deadlock; certificate expiry on the kubelet client cert.
- **Mitigation:** the workload matters more than the node. `kubectl cordon` + `kubectl drain --ignore-daemonsets --delete-emptydir-data` to force rescheduling; if pods still hang, the correct action is to **delete the node object** (`kubectl delete node`) — that lets the controller give up on graceful teardown and reschedule. Only use `--force --grace-period=0` on the pods if I accept that containers may still be running on a zombie node (dangerous for StatefulSets/single-writer volumes — split-brain risk). Then replace the node (scale the node pool / `az aks nodepool` upgrade-reimage).
- **The StatefulSet caveat** (strong senior signal): for a StatefulSet with an attached disk, force-deleting the pod can allow two pods to believe they own the same volume. Wait for the volume to detach, or confirm the node is truly dead first.
- **Prevention:** set kube/system reserved, alert on node disk >75% and on `kubelet_volume_*`/heartbeat lag, enable image GC tuning and log rotation, spread replicas with `topologySpreadConstraints` so one node loss isn't user-visible, use PDBs, and enable node auto-repair/`node-problem-detector`.

---

## S5. A CI/CD pipeline that "works on my machine" and fails in Jenkins

**Scenario.** A Docker build + test pipeline passes locally, fails in Jenkins with flaky, non-reproducible errors: sometimes a missing dependency, sometimes a test timeout, sometimes a permission denied.

### Model answer

The pattern "three different symptoms, non-deterministic" says the problem is **environmental non-determinism**, not the code. So I'd attack determinism as a category rather than chase each symptom.

Checklist of the real causes, in likelihood order:
1. **Unpinned dependencies** — `latest` base image, unpinned pip/npm versions, no lockfile committed. Local has a warm, older resolution; CI resolves fresh. Fix: pin base images **by digest**, commit lockfiles, use `pip install -r requirements.txt --require-hashes` / `npm ci` not `npm install`.
2. **Dirty vs clean workspace** — local has build caches and prior artifacts; CI starts empty (or worse, a *stale shared* agent workspace). Fix: ephemeral agents (Kubernetes plugin pods, one build = one pod), explicit `cleanWs()`, and never depend on state from a previous build.
3. **Permission/UID mismatch** — container runs as a different UID than the mounted workspace owner, or Docker socket permissions differ. Fix: consistent `USER`, correct `fsGroup`, or rootless/Buildkit.
4. **Resource starvation causing "timeouts"** — the agent has fewer CPUs/less memory than the laptop, so tests race or time out. That also explains the intermittent nature. Fix: set requests/limits on agent pods, raise test timeouts to be resource-independent, and check for tests that depend on wall-clock timing.
5. **Missing env/secrets** — locally in a `.env`, in CI injected differently. Fix: fail fast with an explicit config validation step at pipeline start.
6. **Network egress differences** — CI behind a proxy/firewall, package mirror unreachable → the "missing dependency" symptom. Fix: internal artifact mirror (Artifactory/Nexus/ACR), and treat external registries as unavailable by design.
7. **Test pollution / ordering / parallelism** — shared DB or fixed ports across parallel executors on one agent. Fix: randomize test order to expose it, isolate with per-build containers (testcontainers), use dynamic ports.
8. **Architecture drift** — Apple Silicon `arm64` locally vs `amd64` in CI. Fix: `docker buildx` with explicit `--platform`, and build the same platform locally.

**How I'd actually debug it:** reproduce *in* CI rather than locally — run the pipeline steps inside the exact agent image on my machine (`docker run` the agent image), turn on `set -euxo pipefail` (a pipeline that silently continues past a failed step is its own bug), add `--no-cache` to isolate cache effects, and bisect by disabling stages. Capture artifacts/logs on failure so flakes are analyzable after the fact instead of re-run away.

**The senior framing:** the goal isn't to fix three symptoms — it's to make builds **hermetic and reproducible**: same inputs → same output, no shared mutable state, everything pinned, everything ephemeral. I'd also quantify the flake rate and treat flaky tests as production bugs (quarantine + owner + deadline), because a pipeline nobody trusts gets bypassed, and that's how bad code reaches prod.

---

## S6. Deploy succeeded, ArgoCD says `Synced`, but users see the old version

### Model answer

- Distinguish **`Synced`** (live manifests match Git) from **`Healthy`** (workload is actually up) — ArgoCD can be Synced while the rollout is stuck or while what's *serving* is unchanged.
- **Top cause: mutable image tag.** Manifest says `image: app:latest` (or a reused tag). The tag in Git didn't change, so ArgoCD sees no diff and never restarts pods; the digest behind the tag changed but nothing triggered a rollout. Fix: deploy by **immutable digest or unique tag per commit** (`app:git-<sha>`), which makes the manifest change on every release. This is the #1 real-world GitOps footgun.
- Check `imagePullPolicy: IfNotPresent` with a reused tag — nodes serve a cached layer.
- Verify what's actually running: `kubectl get pods -o jsonpath` on `spec.containers[*].image` and `status.containerStatuses[*].imageID` (the digest). Compare digest to the registry. Also `kubectl rollout history`.
- Check the rollout isn't stuck: `kubectl rollout status`, `kubectl describe rs` — new RS created but 0 available (failing readiness probe, image pull error, quota, PDB blocking, unschedulable due to resources) means old pods keep serving. This is the second most common cause.
- Check you're looking at the right place: Service selector still pointing at old labels, an Ingress/route sending traffic elsewhere, a CDN/Front Door/CloudFront cache serving stale assets, or a canary weight of 0%.
- Check ArgoCD itself: correct target revision/branch, `ignoreDifferences` masking the field, a `kustomize`/Helm value overriding the tag, auto-sync disabled, or the repo-server serving a cached manifest. `argocd app diff` and `argocd app get --refresh` (hard refresh) resolve most of these.
- **Prevention:** immutable tags enforced in CI, `Healthy`-gated promotion (not just Synced), sync waves + health checks, alerting on `argocd_app_info{health_status!="Healthy"}` and on sync-vs-serving drift, and a smoke test that asserts the *deployed build SHA* via a `/version` endpoint. Asserting the version endpoint post-deploy is the check that makes this class of bug impossible to miss.

---

## S7. Prometheus is down / OOMing and you've lost alerting

### Model answer

- **Recognize the severity:** losing monitoring is a Sev-2 in itself because you're now blind. First action is to restore *alerting* (Alertmanager can survive independently) and tell on-call they're flying blind.
- **Diagnose the OOM:** Prometheus memory ≈ f(active series, sample ingestion rate, query concurrency). Check `prometheus_tsdb_head_series` — a step change means a **cardinality explosion**, which is the cause in the large majority of cases. Find the culprit with `topk(10, count by (__name__)({__name__=~".+"}))` and `prometheus_tsdb_symbol_table_size_bytes`.
- **Usual trigger:** someone added a label with unbounded values — user ID, request ID, pod name in a high-churn deployment, full URL path with IDs, or an exemplar-like field. Every unique label combination is a new time series.
- **Also check:** an expensive dashboard/recording rule doing a range query over months (`query_samples_total`, `prometheus_engine_query_duration_seconds`), a new `ServiceMonitor` scraping something enormous (e.g. cAdvisor on a huge cluster, or kube-state-metrics with all labels), too-short `scrape_interval`, or WAL replay OOM on restart (which makes it crash-loop and look unrecoverable).
- **Mitigation:** raise the memory limit temporarily to get it to finish WAL replay and become queryable; then `metric_relabel_configs` to `drop` the offending series at scrape time (this is the real fix and it's immediate); reduce retention; cap concurrency; if it can't replay at all, move the WAL aside to recover the process and accept the data loss.
- **Prevention:** cardinality budget per team with an alert on `prometheus_tsdb_head_series` growth rate; enforce `sample_limit`/`label_limit` per scrape config so a bad exporter can't take down the platform; review new metrics in PR; use recording rules for expensive queries; run HA Prometheus pairs so one OOM isn't total blindness; move long-term storage to Thanos/Mimir/Azure Monitor managed Prometheus; and monitor the monitoring (dead-man's-switch `Watchdog` alert so silence itself pages you).

---

## S8. DNS resolution failing intermittently inside the cluster

### Model answer

- Symptoms: intermittent `Name or service not known`, ~5s latency spikes (the classic 5s = DNS retry timeout), only some pods affected.
- **Walk the resolution path:** pod `/etc/resolv.conf` → CoreDNS service → CoreDNS pods → upstream resolver. Test each: `kubectl exec` + `nslookup kubernetes.default`, then `dig @<coredns-pod-ip>` to bypass the Service and isolate whether it's CoreDNS or the path to it.
- **Top causes:**
  - **CoreDNS under-provisioned** — check `coredns_dns_request_duration_seconds`, throttling, replica count vs cluster size, and CPU limits causing throttling. Fix: scale CoreDNS / cluster-proportional autoscaler.
  - **`ndots:5` + search-domain amplification** — every external lookup tries 5 suffixes first, so `api.example.com` becomes 5+ queries. Fix: use FQDNs with a trailing dot, or set `dnsConfig.options ndots: 2`. Huge, cheap win at scale.
  - **conntrack race on UDP** (the notorious `DNAT` race causing exactly 5s timeouts) — fix with NodeLocal DNSCache and/or `single-request-reopen`.
  - Node-level: `nf_conntrack` table full, kube-proxy iptables rules stale, NetworkPolicy accidentally blocking egress to `kube-dns` on UDP/53 (very common after "lock down egress" work — the policy must allow UDP **and** TCP 53).
  - Upstream VPC/VNet resolver throttling (AWS: 1024 packets/s per ENI — `linklocal_allowance_exceeded` in ethtool stats is the smoking gun).
- **Mitigation:** deploy NodeLocal DNSCache, scale CoreDNS, reduce `ndots`, add caching in the app's connection pooling. **Prevention:** alert on CoreDNS error rate/latency and on `linklocal_allowance_exceeded`, and treat DNS as a tier-0 dependency with its own SLO.

---

## S9. Disk full on a production node at 03:00

### Model answer

- `df -h` and `du -xh --max-depth=1 /` to find it. The four usual suspects in K8s: container images (`/var/lib/containerd`), container logs (`/var/log/pods` — an app logging a stack trace in a hot loop can fill 100GB in minutes), `emptyDir` volumes, and application data/core dumps.
- **Immediate:** identify the top consumer, then reclaim safely — `crictl rmi --prune` for unused images, truncate (don't `rm`) the active log file (`truncate -s 0`, because `rm` on an open fd doesn't free space until the process closes it — a detail interviewers love), rotate logs, delete core dumps. Cordon the node if it's still filling.
- **Never** delete something a running container has open expecting space back; check with `lsof +L1`.
- **Root cause:** missing log rotation (`containerd`/`kubelet` `containerLogMaxSize`/`containerLogMaxFiles`), no image GC thresholds, an app with debug logging left on in prod, or a runaway `emptyDir` with no `sizeLimit`.
- **Prevention:** kubelet `imageGCHighThresholdPercent`/`LowThreshold`, `evictionHard` on `nodefs`, log rotation enforced, `sizeLimit` on all `emptyDir`, ship logs off-node (Fluent Bit → ELK) with backpressure handling, alert at 70/85%, separate volume for container runtime so a log flood can't take down the OS disk, and a log-volume budget per namespace.

---

## S10. Kubernetes pod stuck in `Pending`

### Model answer

`Pending` = the scheduler couldn't place it, or it's placed but not yet initialized. `kubectl describe pod` events give the answer directly — read them, don't guess. Then map to cause:

- `0/N nodes are available: insufficient cpu/memory` → requests exceed allocatable. Check whether requests are absurd (someone asked for 32 CPU), whether Cluster Autoscaler/Karpenter is scaling (and why not — check its logs, node group max, instance quota, or a pod that's unschedulable on *any* possible node shape).
- `node(s) had untolerated taint` → taint/toleration mismatch (very common with GPU/spot/system node pools).
- `node(s) didn't match Pod's node affinity/selector` → affinity too strict, or a zone with no capacity.
- `pod has unbound immediate PersistentVolumeClaims` → StorageClass missing, CSI driver broken, or **zone mismatch between the PV and the schedulable nodes** (fix: `volumeBindingMode: WaitForFirstConsumer` — the single most common EBS/Azure Disk multi-AZ trap).
- `too many pods` → node's max-pods limit; on EKS with the VPC CNI this is ENI/IP-driven, and IP exhaustion in the subnet is a frequent real cause.
- ResourceQuota/LimitRange rejection, or a failing admission webhook (`failurePolicy: Fail` with a dead webhook blocks all scheduling — a great "whole cluster is frozen" story).
- `topologySpreadConstraints` with `DoNotSchedule` that can't be satisfied.
- Pending *after* scheduling: `ImagePullBackOff` (bad tag, missing `imagePullSecret`, registry throttling), or an init container that never completes.

**Prevention:** capacity alerts on unschedulable pods (`kube_pod_status_unschedulable`), autoscaler headroom / overprovisioning pods, quota dashboards, `WaitForFirstConsumer` by default, and IP-space planning reviewed before cluster build.

---

## S11. Terraform plan wants to destroy and recreate a production database

**Scenario.** A routine PR shows `-/+ destroy and then create replacement` on `aws_db_instance.prod`. Nobody intended that.

### Model answer

- **Stop. Never apply this.** State it plainly — recognizing that a plan output is a business risk is the point of the question.
- Read *why*: the plan annotates `# forces replacement` on the specific attribute. Find that attribute; it's the whole answer.
- Typical triggers: a changed immutable field (`engine_version` major, `identifier`, `availability_zone`, `db_subnet_group_name`, `character_set`), a `name` change causing a new resource address, provider version upgrade changing defaults, a module refactor that **moved the resource address** (state thinks the old address is gone), or someone changed something out-of-band and the config now conflicts.
- Resolution paths: `terraform state mv` / `moved` blocks for pure address refactors (no infrastructure change); align config to reality for out-of-band drift; `terraform plan -target` to isolate; `import` if the resource was recreated manually. For a genuinely required immutable change, plan a **migration** (snapshot, replica promotion, blue/green with `create_before_destroy`), not a Terraform-driven replace.
- **Guardrails that should already exist:** `lifecycle { prevent_destroy = true }` on stateful resources, mandatory plan review in PR with the plan posted as a comment, `deletion_protection = true` on RDS, separate state for data-tier resources, OPA/Conftest/Sentinel policy that *fails CI* on any destroy of a tagged-critical resource, and applies gated on manual approval for prod. Automated destroy-detection in CI is the answer that gets you the offer.

---

## S12. Application latency spiked 10x but CPU and memory look fine

### Model answer

The framing to lead with: "CPU and memory fine" rules out compute saturation, so I look at **queueing, locking, and dependencies** — latency is usually caused by waiting, not computing.

- **Start at the top of the stack with RED metrics**, and find *where* the time goes: distributed tracing (or at minimum per-dependency latency histograms). Is the added time in the app, the DB, a cache, or a downstream API?
- Candidates: **database** — slow query from a missing index after data growth, lock contention, connection pool exhaustion (requests queue *before* the DB, so DB looks idle!), a bad query plan after stats change, replica lag causing read fallback.
- **CPU throttling despite low utilization** — the K8s classic. `container_cpu_cfs_throttled_seconds_total` high with low average CPU: a low CPU *limit* throttles bursts, so p99 explodes while the average looks tame. Extremely common and almost always missed by mid-level candidates.
- **Thread/worker pool saturation** — check queue depth; average CPU can be low while every worker waits on I/O.
- **GC pressure** — long pauses appear as latency, not CPU.
- **Network** — DNS (see S8), TLS handshake storms from disabled keep-alive, cross-AZ hops added by a topology change, packet loss/retransmits, ENI bandwidth or PPS allowance exceeded.
- **Noisy neighbours / node-level saturation** — disk I/O wait (`iowait`, EBS burst-balance exhaustion is a classic cliff), or another pod hogging the node.
- **External:** a downstream provider degraded; check their status and your circuit-breaker/retry behavior — aggressive retries turn a small dependency blip into a self-inflicted latency storm (retry amplification).
- **Mitigation:** scale out, raise the CPU limit if throttled, increase pool sizes carefully, enable caching, shed load / degrade gracefully. **Prevention:** SLO-based alerting on p99 latency (not CPU), tracing coverage, throttling and saturation dashboards, load testing at realistic concurrency, and budgets for downstream timeouts + circuit breakers.

---

## S13. Secret rotated, half the fleet broke

### Model answer

- Cause: pods read secrets **at startup** into env vars. Rotating the secret in the store doesn't restart pods, so you get a split fleet — pods started after rotation have the new value, old ones the stale one (or vice versa if the old credential was revoked immediately).
- **Immediate:** if the old credential still works, re-enable it to restore service, then roll pods deliberately. If it's already revoked, do a controlled `kubectl rollout restart` of affected deployments, prioritizing user-facing ones. Verify with a health/auth check, not assumption.
- **Root cause:** rotation without a reload path, and revoke-before-roll ordering.
- **Prevention:** **two-phase rotation** — create new credential, deploy consumers to accept both, cut over, *then* revoke old. Never revoke before all consumers are confirmed on the new value. Mount secrets as **files** (projected volumes / CSI Secrets Store with `enableSecretRotation`) so they can be reloaded without restart; add SIGHUP or file-watch reload in the app; use `reloader`/`stakater` annotations to trigger rollouts on secret change; prefer **short-lived identity** (IRSA / Azure Workload Identity / Vault dynamic secrets) so there's nothing static to rotate — that's the real senior answer. Add a "credential expiring in 14 days" alert.

---

## S14. Docker image build ballooned from 200MB to 2.5GB and builds take 20 minutes

### Model answer

- **Diagnose:** `docker history <image>` to find the fat layers; `dive` for layer-by-layer waste; check the build context size (a missing `.dockerignore` shipping `.git`, `node_modules`, and test fixtures into context is instant, common, and slows every build).
- **Root causes:** no multi-stage build so compilers/SDKs/test deps ship to prod; `apt-get install` without `--no-install-recommends` and without cleaning `/var/lib/apt/lists` **in the same layer** (cleaning in a later layer frees nothing — layers are additive); `COPY . .` before dependency install, which busts the cache on every source change; baking secrets/artifacts in; a heavyweight base image.
- **Fixes:** multi-stage with a distroless/alpine/slim runtime stage; order layers by change frequency (deps first, source last) for cache hits; `--mount=type=cache` with BuildKit for package caches; `.dockerignore`; pin base by digest; combine RUN steps that must share a layer; build once and promote the same digest across environments; enable registry-backed layer caching in CI (`--cache-from`/`--cache-to`) so the cache survives ephemeral agents — that's usually where the 20 minutes goes.
- **Why it matters beyond size** (say this): smaller images = faster pull = faster scale-out and rollback, plus a smaller CVE surface, which directly affects both MTTR and security posture. Add image size and vuln count as CI gates so it can't regress again.

---

## S15. "Everything is fine but users say it's broken"

### Model answer

- This is a **monitoring gap**, and the correct instinct is to trust the users, not the dashboards.
- Ask what "broken" means: which journey, which region, which client, since when? Get one concrete failing request ID/trace.
- Recognize the pattern: your dashboards measure **server-side availability** while users experience **client-side journeys**. Everything green + users broken usually means the failure is somewhere you don't measure: CDN/edge, DNS, TLS cert expiry, a third-party script, a mobile-app-specific API version, one AZ, one ISP, a cache serving stale content, or an auth provider.
- **Verify externally:** synthetic checks from outside your network, `curl -v` with timing breakdown, check cert expiry, check edge/WAF rules (a new WAF rule blocking legitimate traffic returns 403s that never reach your app metrics — so your error rate looks perfect).
- Check whether your SLI is measured at the wrong place: load-balancer-level metrics miss anything failing before the LB.
- **Prevention:** measure from the user's perspective — RUM, synthetic probes per region, SLOs defined on user journeys not component uptime, cert-expiry alerts, and alerting on *absence* of traffic (a sudden drop in request volume is often the earliest signal of an outage upstream of you).
