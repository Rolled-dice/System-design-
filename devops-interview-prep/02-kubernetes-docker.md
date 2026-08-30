# Part 2 — Kubernetes & Docker

Expect 30–40% of a DevOps interview here. Answers are written the way you should *say* them.

---

## A. Architecture & Control Plane

**1. Walk me through what happens when you run `kubectl apply -f deploy.yaml`.**
1. kubectl resolves the kubeconfig context, builds an HTTPS request to the API server. 2. **Authentication** (client cert / OIDC / IAM token via aws-iam-authenticator or Entra ID). 3. **Authorization** (RBAC). 4. **Admission**: mutating webhooks (sidecar injection, defaulting) → schema validation → validating webhooks (OPA/Gatekeeper, Kyverno) → quota. 5. Object persisted to **etcd**. 6. **Deployment controller** sees the new spec, creates a ReplicaSet. 7. **ReplicaSet controller** creates Pod objects with `nodeName` empty. 8. **Scheduler** filters (predicates) then scores (priorities) nodes, binds the pod. 9. **kubelet** on that node pulls the image via CRI (containerd), calls CNI to set up networking, CSI for volumes, starts the container. 10. kubelet reports status; when readiness passes, **endpoints/EndpointSlice controller** adds the pod IP to the Service, and kube-proxy/CNI programs the data path.
Key point to state: **everything is a level-triggered reconciliation loop comparing desired vs actual**, not an imperative command chain.

**2. What is etcd and how do you protect it?** Distributed, strongly-consistent (Raft) key-value store holding all cluster state. Odd number of members (3 or 5) for quorum; quorum loss = read-only/unavailable control plane. Protect with: encryption at rest, TLS peer/client, regular `etcdctl snapshot save` backups tested by *restore*, low-latency disks (etcd is fsync-sensitive — slow disk = cluster instability), and defragmentation/compaction. On EKS/AKS this is managed, but you're still responsible for object churn and size limits.

**3. Deployment vs StatefulSet vs DaemonSet vs Job/CronJob.** Deployment = stateless, interchangeable replicas, random names, rolling update. StatefulSet = stable identity (`app-0`, `app-1`), stable per-pod PVC, ordered creation/deletion, headless Service for per-pod DNS — for databases, Kafka, anything with a peer identity or single-writer volume. DaemonSet = one pod per node (log shippers, CNI, node exporters). Job = run-to-completion; CronJob = scheduled Job.

**4. What is a ReplicaSet and why don't you create it directly?** It maintains N replicas. Deployments own ReplicaSets to enable rolling updates and rollback (each revision = one RS). Managing RS directly loses that history.

**5. How does a rolling update actually work?** Deployment creates a new RS and shifts replicas per `maxSurge`/`maxUnavailable`. Old pods terminate only as new ones become **Ready** — which is why readiness probes are what make rolling updates safe. `maxUnavailable: 0` + `maxSurge: 1` is the safest (but slowest, and needs spare capacity). Rollback = `kubectl rollout undo`, which just scales the previous RS back up.

**6. Explain the pod lifecycle including termination.** Pending → Running → Succeeded/Failed. Termination: pod marked Terminating → **removed from Service endpoints** *and simultaneously* `preStop` hook runs → SIGTERM to PID 1 → wait `terminationGracePeriodSeconds` (default 30) → SIGKILL. The race everyone gets bitten by: endpoint removal is *eventually* consistent across kube-proxy on every node, so a pod can receive traffic after SIGTERM. Fix: `preStop: sleep 15` and app-level graceful drain.

---

## B. Probes, Health & Resources

**7. Liveness vs readiness vs startup probes — and the classic mistake.** Readiness = "can I take traffic?" (fails → removed from endpoints, pod keeps running). Liveness = "am I wedged?" (fails → container restarted). Startup = "still booting?" (disables the other two until it passes). **The classic mistake: pointing liveness at a deep health check that includes dependencies.** If the DB blips, every pod fails liveness, every pod restarts simultaneously, and you turn a dependency degradation into a full outage. Liveness must be shallow and local; readiness can check dependencies.

**8. Requests vs limits, and the three QoS classes.** Requests drive **scheduling** and are the guaranteed floor; limits are the enforced ceiling. **Guaranteed** = requests == limits for all containers (last to be evicted). **Burstable** = requests < limits. **BestEffort** = neither set (evicted first). CPU limits throttle via CFS quota; memory limits kill via OOM. Senior nuance: **CPU limits are often harmful** — they cause throttling and p99 latency spikes even at low average utilization; many teams set CPU requests only, plus memory requests == limits. Say that trade-off explicitly.

**9. What is CPU throttling and how do you detect it?** The kernel gives the cgroup a quota per 100ms period; exceeding it means the process is stopped until the next period. Detect with `container_cpu_cfs_throttled_periods_total / container_cpu_cfs_periods_total`. A bursty request-serving app with a 500m limit gets throttled badly even at 20% average CPU.

**10. HPA vs VPA vs Cluster Autoscaler vs Karpenter.** HPA scales replica *count* on CPU/memory/custom/external metrics (KEDA for queue depth, which is usually the *correct* signal). VPA adjusts requests/limits — don't run VPA and HPA on the same metric. Cluster Autoscaler adds *nodes* when pods are unschedulable, within node-group bounds. Karpenter (AWS) provisions right-sized nodes directly from pending pod requirements — faster, better bin-packing, simpler than many node groups. Mention that HPA on CPU for a queue worker is a common design error.

**11. How do you make an app scale on something meaningful?** KEDA with a ScaledObject on SQS/Service Bus/Kafka lag, or Prometheus Adapter for RPS-per-pod. Scaling on the backlog you're trying to drain, not on CPU.

---

## C. Networking

**12. Explain the Kubernetes networking model.** Every pod gets a routable IP; all pods can reach all pods without NAT; nodes can reach all pods. Implemented by a CNI plugin. AWS VPC CNI gives pods real VPC IPs (great for security groups + no overlay overhead, but **IP exhaustion** and ENI-per-instance pod limits are the trade-off). Azure has kubenet (overlay, NAT) vs Azure CNI (VNet IPs) vs Azure CNI Overlay (best of both). Calico/Cilium add NetworkPolicy and eBPF dataplanes.

**13. Service types.** ClusterIP (internal virtual IP), NodePort (port on every node), LoadBalancer (cloud LB per service — expensive at scale), ExternalName (CNAME), Headless (`clusterIP: None`, returns pod IPs directly — used by StatefulSets and client-side load balancing/gRPC).

**14. How does a Service actually route traffic?** It's not a proxy process — kube-proxy programs **iptables** (or IPVS, or eBPF with Cilium) DNAT rules on every node mapping the ClusterIP to a randomly-selected backend pod IP from the EndpointSlice. Consequences worth mentioning: iptables mode is O(n) rules and degrades with thousands of services; load balancing is per-connection, so **long-lived HTTP/2 and gRPC connections pin to one pod** and skew load — fix with a service mesh, headless + client-side LB, or periodic connection recycling.

**15. Ingress vs Gateway API vs service mesh.** Ingress = L7 HTTP routing via a controller (nginx, AWS LB Controller, AGIC), but annotation-driven and vendor-fragmented. **Gateway API** is the successor: typed, role-oriented (infra vs app team separation), supports TCP/gRPC properly — the correct 2026 answer for new builds. A mesh (Istio/Linkerd) adds mTLS, retries, circuit breaking, traffic splitting, and L7 telemetry between services — different problem from north-south ingress.

**16. NetworkPolicy — what's the default and the gotcha?** Default is allow-all. A policy selecting a pod flips it to deny-by-default *for the direction(s) specified*. Gotchas: policies are additive (union, no deny rules); you must explicitly allow DNS egress to kube-dns on **UDP and TCP 53** or everything breaks; and NetworkPolicy needs a CNI that implements it (AWS VPC CNI alone historically didn't).

**17. What is a service mesh sidecar vs ambient/sidecarless?** Sidecar injects an Envoy per pod (resource overhead ×N pods, restart-to-upgrade). Ambient mesh / Cilium service mesh moves L4 to a per-node component and L7 to optional waypoints — much lower overhead, the current direction of travel.

---

## D. Storage

**18. PV, PVC, StorageClass, and the dynamic provisioning flow.** PVC = the app's request; StorageClass = the "how" (driver, type, IOPS, reclaim policy); the CSI driver provisions a PV and binds it. `reclaimPolicy: Retain` vs `Delete` — Delete on production data is how people lose databases.

**19. Why `volumeBindingMode: WaitForFirstConsumer`?** With `Immediate`, the volume is created in some AZ before the pod is scheduled; if the scheduler then picks a node in another AZ, the pod is permanently `Pending`. `WaitForFirstConsumer` delays provisioning until scheduling is decided. This is the single most common multi-AZ storage bug.

**20. Access modes.** RWO (one node — EBS, Azure Disk), ROX, RWX (many nodes — EFS, Azure Files, NFS). You can't scale a Deployment past 1 replica across nodes on an RWO volume.

**21. How do you back up stateful workloads?** Velero for cluster objects + volume snapshots, CSI `VolumeSnapshot` for point-in-time, plus application-consistent backups (DB dumps / WAL shipping) because a crash-consistent block snapshot of a database isn't guaranteed restorable. Test restores on a schedule — an untested backup is a hypothesis.

---

## E. Configuration, Security & Scheduling

**22. ConfigMap vs Secret — and are Secrets secure?** Secrets are only **base64-encoded**, not encrypted, and readable by anyone with RBAC get on them plus stored in etcd. Make them real: encryption at rest (KMS provider), tight RBAC, external stores (External Secrets Operator / CSI Secrets Store with AWS Secrets Manager, Azure Key Vault, Vault), and ideally **short-lived workload identity** so there's no static secret at all.

**23. How do pods get cloud credentials without static keys?** AWS: **IRSA** (ServiceAccount annotated with an IAM role; the projected OIDC token is exchanged via STS) or EKS Pod Identity. Azure: **Workload Identity** federating a Kubernetes ServiceAccount to an Entra ID app/managed identity. Never node instance profiles for app permissions — that grants every pod on the node the same access.

**24. Pod Security Standards / admission control.** PSP is dead; use **Pod Security Admission** (privileged/baseline/restricted per namespace) plus a policy engine (Kyverno/Gatekeeper) for anything custom. Baseline hardening: `runAsNonRoot`, drop all capabilities, `readOnlyRootFilesystem`, `allowPrivilegeEscalation: false`, no `hostNetwork`/`hostPID`, seccomp `RuntimeDefault`.

**25. RBAC design.** Role/RoleBinding (namespaced) vs ClusterRole/ClusterRoleBinding. Least privilege, groups over users, no wildcard verbs, and audit with `kubectl auth can-i --list`. Watch for the escalation paths: `create pods` in a namespace with a privileged ServiceAccount ≈ cluster admin; so is `escalate`/`bind` on roles, or `exec` into a pod holding good credentials.

**26. Taints/tolerations vs node affinity vs topology spread.** Taints **repel** pods from nodes (node-side opt-out); tolerations let a pod ignore a taint. Affinity **attracts** pods to nodes (pod-side preference). `topologySpreadConstraints` distributes replicas across zones/nodes for HA — the correct tool for "don't put all 3 replicas on one node," better than the older anti-affinity approach.

**27. What's a PodDisruptionBudget and when does it *not* protect you?** PDB constrains **voluntary** disruptions (drain, autoscaler, node upgrade) via `minAvailable`/`maxUnavailable`. It does **not** protect against involuntary events — node crash, OOMKill, kernel panic. Also, a too-strict PDB can block node upgrades indefinitely; that's a real operational trap.

**28. Init containers vs sidecars vs ephemeral containers.** Init runs to completion sequentially before app containers (migrations, waiting on dependencies). Sidecars run alongside (K8s 1.29+ has native sidecar init containers with `restartPolicy: Always`, which fixes the old shutdown-ordering and Job-never-completes problems). Ephemeral containers = `kubectl debug` into a running pod — essential for distroless images with no shell.

**29. How do you debug a container with no shell?** `kubectl debug -it pod/x --image=busybox --target=app` attaches an ephemeral container sharing the process namespace. For nodes, `kubectl debug node/<node>`.

**30. Namespaces, ResourceQuota, LimitRange.** Namespaces are scope/RBAC boundaries, not security boundaries (no network isolation by default). ResourceQuota caps aggregate namespace consumption; LimitRange sets per-pod defaults and bounds — pair them so untuned workloads still get sane requests.

---

## F. Docker & Images

**31. Container vs VM.** Containers share the host kernel, isolated by namespaces (pid/net/mnt/uts/ipc/user) and limited by cgroups. VMs virtualize hardware with their own kernel. Containers: seconds to start, MBs, weaker isolation boundary. That's why multi-tenant untrusted workloads use gVisor/Kata/Firecracker.

**32. Multi-stage builds — why?** Build stage has compilers, SDKs, test deps; final stage copies only the artifact onto a minimal base. Smaller image = faster pulls, faster scale-out and rollback, far smaller CVE surface. Show you know `--target` for building just the test stage in CI.

**33. Dockerfile best practices that actually matter.** Pin base images by **digest**; order instructions least→most volatile so the layer cache survives source changes (copy lockfile + install deps *before* copying source); one logical concern per RUN with cleanup in the *same* layer (cleaning in a later layer frees nothing); `.dockerignore`; non-root `USER`; `COPY` not `ADD`; exec-form `ENTRYPOINT` so signals reach PID 1; `HEALTHCHECK`; no secrets in layers (use BuildKit `--mount=type=secret`); build once, promote the same digest across envs.

**34. `CMD` vs `ENTRYPOINT`.** ENTRYPOINT is the executable, CMD provides default args (and is overridden by `docker run` args). In K8s, `command` overrides ENTRYPOINT and `args` overrides CMD — a frequent source of confusion.

**35. Why does PID 1 matter in containers?** PID 1 doesn't get default signal handlers and doesn't reap zombies. Shell-form ENTRYPOINT wraps your app in `/bin/sh -c`, which doesn't forward SIGTERM → your app never shuts down gracefully → the 30s grace period expires → SIGKILL → dropped connections. Fix: exec form, or `tini`/`--init`.

**36. `docker COPY --chown`, layers and image size.** Every instruction is a layer; layers are immutable and additive, so deleting a file in a later layer keeps the bytes in the image. Squash/multi-stage or don't add it in the first place.

**37. How do you scan and sign images?** Trivy/Grype in CI (fail on fixable HIGH/CRITICAL), generate an **SBOM** (Syft, CycloneDX/SPDX), sign with **cosign** (keyless via OIDC), and enforce at admission with Kyverno `verifyImages` or Ratify so unsigned/unscanned images can't run. Add provenance attestations (SLSA) for supply-chain maturity.

**38. Docker networking modes.** bridge (default, NAT), host (no isolation, no port mapping), none, overlay (multi-host), macvlan. Mention `--network host` breaks port isolation and is a common "quick fix" that leaks into prod.

**39. How do you reduce image pull time in a large cluster?** Small images, registry in the same region, registry mirror/pull-through cache (ECR/ACR), pre-pulled images baked into node AMIs, `imagePullPolicy: IfNotPresent` with immutable tags, and lazy-pulling (eStargz/SOCI) for very large images.

**40. Docker Compose vs Kubernetes — when is Compose still right?** Local dev and single-host simplicity. It has no self-healing, no rolling updates, no multi-node scheduling. Don't run prod on it, and don't translate Compose 1:1 into K8s — the abstractions differ.

---

## Rapid-fire K8s facts interviewers love

- `kubectl get events --sort-by=.lastTimestamp` is your first command 80% of the time.
- CrashLoopBackOff backoff is exponential, capped at **5 minutes**.
- `ImagePullBackOff` → tag typo, missing `imagePullSecret`, or Docker Hub rate limiting.
- `Terminating` forever → finalizers (`kubectl patch ... -p '{"metadata":{"finalizers":null}}'` as last resort) or an unreachable kubelet.
- `Evicted` → node resource pressure, not a limit breach.
- `OOMKilled` → container cgroup limit breach. Exit 137.
- Exit 143 = SIGTERM (graceful), 137 = SIGKILL, 1/2 = app error, 127 = command not found.
- Max object size in etcd ≈ 1.5MB — huge ConfigMaps fail.
- Node drain respects PDBs; `--force` deletes unmanaged pods.
- `kubectl top` needs metrics-server; HPA on custom metrics needs an adapter.
