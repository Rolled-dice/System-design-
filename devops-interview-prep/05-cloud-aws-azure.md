# Part 5 — Cloud: AWS & Azure

Know one deeply, be conversant in the other. Interviewers respect "in Azure I'd use X, the AWS equivalent is Y."

---

## A. Networking

**1. Design a production VPC / VNet.** Multi-AZ (3 AZs), split into public subnets (ALB/NAT only), private app subnets (compute), and isolated data subnets (DB, no route to internet). One NAT Gateway per AZ for HA (a single NAT is both an AZ-failure SPOF and a cross-AZ data charge). Route tables per tier, no default route in the data tier, VPC endpoints for S3/ECR/Secrets Manager to keep traffic off the internet and cut NAT cost. Flow logs on. Non-overlapping CIDRs planned centrally for future peering/Transit Gateway. **Size subnets generously** — with the EKS VPC CNI, pods consume subnet IPs, and you cannot resize a subnet.

**2. Security group vs NACL.** SG = stateful, instance/ENI-level, allow-rules only, can reference other SGs (use this — reference the ALB's SG instead of a CIDR). NACL = stateless, subnet-level, allow *and* deny, needs explicit return-traffic rules (the classic gotcha — ephemeral ports must be open outbound). Azure: NSGs are stateful and combine both roles; ASGs (Application Security Groups) are the SG-referencing equivalent.

**3. How do you connect on-prem to cloud?** Site-to-site VPN (fast to set up, internet-dependent, ~1.25 Gbps per tunnel) vs Direct Connect / ExpressRoute (dedicated, predictable latency, expensive, weeks to provision). Production pattern: ExpressRoute/DC with a VPN as backup path. Hub-and-spoke topology: Transit Gateway (AWS) or VNet peering to a hub with Azure Firewall/Route Server, centralizing egress inspection and DNS.

**4. Load balancer options.** AWS: ALB (L7, HTTP/gRPC, path/host routing, WAF integration), NLB (L4, static IP, extreme throughput, TLS passthrough, preserves source IP), CLB (legacy), CloudFront (CDN/edge), Global Accelerator (anycast). Azure: Application Gateway (L7 + WAF), Load Balancer (L4), Front Door (global L7 + CDN + WAF), Traffic Manager (DNS-based global routing). Pick by layer, scope (regional vs global), and whether you need WAF/TLS termination.

**5. What is cross-AZ traffic and why does it matter?** It costs money in both directions and adds latency. Mitigate with topology-aware routing (K8s `topologyKeys`/Cilium local routing, Istio locality LB), and be aware a naive service mesh or random Service load balancing sends ~2/3 of traffic cross-AZ. This is both a FinOps and a latency answer — good crossover signal.

**6. Private DNS and hybrid resolution.** Route 53 private hosted zones / Azure Private DNS zones, resolver endpoints or forwarders for on-prem↔cloud resolution. Private Link/Private Endpoints for reaching PaaS services over private IPs — and remember Private Endpoints require DNS overrides to resolve to the private IP, which is the most common Private Link failure.

---

## B. Identity & Security

**7. IAM fundamentals (AWS).** Users/groups/roles/policies. Roles + `sts:AssumeRole` for everything; no long-lived access keys. Policy evaluation: explicit **Deny** always wins, then Allow, default deny. Layers: identity policies, **resource** policies (S3 bucket policy, KMS key policy), **permission boundaries** (max privilege for a principal), **SCPs** (org-wide guardrails — e.g. deny leaving the org, deny unapproved regions), and session policies. Use IAM Access Analyzer and Access Advisor to right-size permissions.

**8. Azure RBAC and Entra ID.** Scope hierarchy: management group → subscription → resource group → resource, with inheritance. Built-in vs custom roles; **Azure Policy** for guardrails (deny/audit/deployIfNotExists — the SCP analogue plus remediation). Managed identities (system- vs user-assigned) for workload auth, PIM for just-in-time elevation of privileged roles, Conditional Access for human sign-in.

**9. How do workloads get credentials with no secrets?** AWS: instance profiles for VMs, **IRSA / EKS Pod Identity** for pods, task roles for ECS. Azure: managed identities, **Workload Identity** for AKS. Cross-cloud/CI: OIDC federation. Always name the "no static keys" principle explicitly.

**10. Encryption.** At rest: KMS/Key Vault, customer-managed keys where compliance requires key control and rotation, envelope encryption (data key encrypted by a master key). In transit: TLS everywhere including internal (mTLS via mesh). Secrets: Secrets Manager/Key Vault with rotation. Key questions interviewers probe: who can decrypt (key policy!), rotation strategy, and whether you'd notice a key deletion (7–30 day pending window — set an alarm).

**11. How do you secure a multi-account/multi-subscription estate?** AWS Organizations with OUs, SCPs, Control Tower, a dedicated log-archive account, a security-tooling account with GuardDuty/Security Hub delegated admin, and account-per-environment so blast radius and quotas are isolated. Azure: management groups, Azure Landing Zones, subscription-per-environment, Defender for Cloud, central Log Analytics workspace. The principle is **isolation by account/subscription boundary, governed centrally**.

---

## C. Compute & Containers

**12. EKS vs AKS vs self-managed Kubernetes.** Managed control plane (patching, HA etcd, upgrades) vs full control. EKS: you own node groups/Karpenter, addons, VPC CNI IP planning, IRSA. AKS: node pools, tighter Azure integration (AGIC, Azure CNI Overlay, Workload Identity), free control plane (paid SLA tier available). Self-managed only for very specific compliance/customization needs — the operational cost is rarely worth it.

**13. How do you upgrade a Kubernetes cluster with no downtime?** Read the release notes for removed APIs (`kubectl` deprecation checks / `pluto` / `kubent`). Upgrade control plane first (one minor version at a time — no skipping), then node pools via **surge/blue-green node pools**: add new-version nodes, cordon+drain old ones respecting PDBs, verify, remove old pool. Pre-checks: PDBs exist and aren't too strict, replicas ≥ 2, topology spread, addon/CSI/CNI version compatibility, and a tested rollback (you generally cannot downgrade a control plane — so validate in staging first). Do it in a low-traffic window with a canary namespace.

**14. Spot/Preemptible strategy.** Use for stateless, interruption-tolerant, retryable work (batch, CI runners, stateless web with enough capacity buffer). Requirements: diversify instance types/AZs, handle the 2-minute interruption notice (node-termination-handler cordons and drains), PDBs, and keep a baseline on-demand/reserved capacity for critical replicas. Typical 60–90% savings — a strong FinOps talking point with real risk management attached.

**15. Serverless — when and when not?** Lambda/Functions for event-driven, spiky, low-baseline workloads; you stop managing servers and pay per invocation. Against: cold starts (mitigate with provisioned concurrency/pre-warmed instances), execution time and package limits, VPC networking complexity, harder local testing, and **cost inversion at high steady throughput** where containers are cheaper. Also mention observability and per-invocation tracing.

---

## D. Data & Storage

**16. How do you design a highly available database tier?** Managed service (RDS/Aurora, Azure SQL/Flexible Server) with **synchronous multi-AZ standby** for HA (automatic failover, RPO ≈ 0) plus **read replicas** for scale (asynchronous — so reads may be stale; the app must know which queries tolerate that). Automated backups + PITR, cross-region replica or backup copy for DR, connection pooling (RDS Proxy/PgBouncer) because failovers and Lambda-style fan-out exhaust connections, and parameter groups in IaC. Test failover deliberately — an untested failover is a guess.

**17. Storage classes and lifecycle.** S3 Standard → Intelligent-Tiering → IA → Glacier tiers; Azure Hot → Cool → Cold → Archive. Lifecycle policies to transition and expire, versioning + MFA delete for critical buckets, Object Lock/immutable blobs for ransomware and compliance. Note retrieval time and minimum-duration charges — moving noisy short-lived objects to Archive can cost *more*.

**18. EBS/Azure Disk performance concepts.** gp3 (decoupled IOPS/throughput — almost always cheaper and better than gp2), io2 for high sustained IOPS, Premium/Ultra Disk on Azure. Watch burst-balance exhaustion on smaller volumes — a latency cliff that looks like an app problem (tie back to the latency scenario in Part 1).

**19. When would you choose NoSQL over relational?** Access-pattern-driven single-table designs (DynamoDB/Cosmos DB) for predictable key-based access at massive scale with single-digit-ms latency; relational for complex joins, ad-hoc queries, and transactional integrity. Cosmos DB gives five tunable consistency levels — mention that consistency, latency, availability, and cost are the dials.

---

## E. Reliability & DR

**20. Define RTO and RPO and how they drive architecture.** RTO = max acceptable downtime; RPO = max acceptable data loss. They pick the pattern: backup/restore (hours, cheapest) → pilot light (minutes-to-hours) → warm standby (minutes) → active/active multi-region (near-zero, most expensive and complex). Always ask the business for the numbers before designing — that's the senior move.

**21. Multi-AZ vs multi-region — when do you actually need multi-region?** Multi-AZ handles the overwhelming majority of real failures and is table stakes. Multi-region is for regional outage tolerance, data residency, or global latency. Costs: data replication, cross-region egress, doubled infrastructure, and **hard problems in data consistency and failover orchestration**. Recommend multi-region only when RTO/RPO or regulation demands it, and say so — over-engineering is a red flag.

**22. How do you test resilience?** Game days and chaos engineering: kill pods/nodes, fail an AZ, inject latency/errors into dependencies (AWS FIS, Chaos Mesh, Azure Chaos Studio), and practise failovers and restores on a schedule. Steady-state hypothesis first, blast radius limited, and abort conditions defined. Untested DR is not DR.

**23. What's a health check you'd regret?** One that returns 200 as long as the process is alive (so a broken app stays in rotation), or a deep check that fails on any dependency blip (so the whole fleet drops out). Also: a health check that isn't on the same path as real traffic. Distinguish shallow-liveness from deep-readiness.

---

## F. FinOps in cloud terms (see also Part 8)

**24. Where does cloud spend actually go and how do you attack it?** Usually: over-provisioned compute, idle non-prod, un-attached/over-sized storage and snapshots, cross-AZ and NAT egress, and old-generation instances. Attack order: (1) **visibility and tagging** — you can't cut what you can't attribute; enforce tags with policy. (2) **Waste elimination** — schedules for non-prod, delete orphaned resources, gp2→gp3, lifecycle policies. (3) **Right-sizing** — Compute Optimizer/Advisor, VPA recommendations, real utilization data. (4) **Commitment discounts** — Savings Plans/Reserved Instances/CUDs once the baseline is stable (never commit before right-sizing, or you lock in waste). (5) **Architecture** — spot, ARM/Graviton (~20–40% better price-performance), serverless for spiky loads, caching to reduce egress, VPC endpoints to cut NAT. (6) **Culture** — showback/chargeback per team, cost in PR (Infracost), unit-cost metrics like cost-per-1000-requests. Lead with unit economics, not absolute spend.

**25. What are the most common surprise cloud bills?** NAT Gateway data processing, cross-AZ traffic, S3 requests (not storage) on chatty workloads, CloudWatch/Log Analytics ingestion and retention, unused EIPs and idle load balancers, orphaned snapshots and disks, inter-region replication, and non-prod left running over a weekend. Naming logging ingestion is a strong signal — it's frequently the second-largest line item and almost never budgeted.

---

## G. Rapid-fire cloud equivalents

| AWS | Azure |
|---|---|
| EC2 | Virtual Machines |
| EKS / ECS / Fargate | AKS / Container Apps / ACI |
| Lambda | Functions |
| S3 | Blob Storage |
| EBS / EFS | Managed Disks / Azure Files |
| RDS / Aurora | Azure SQL / Flexible Server |
| DynamoDB | Cosmos DB |
| VPC | VNet |
| Security Group | NSG |
| ALB / NLB | Application Gateway / Load Balancer |
| CloudFront + Global Accelerator | Front Door |
| Route 53 | Azure DNS + Traffic Manager |
| IAM | Entra ID + Azure RBAC |
| Secrets Manager / Parameter Store | Key Vault |
| KMS | Key Vault / Managed HSM |
| CloudWatch | Azure Monitor + Log Analytics |
| CloudTrail | Activity Log |
| SQS / SNS | Service Bus / Event Grid |
| Kinesis / MSK | Event Hubs |
| CodePipeline | Azure Pipelines |
| ECR | ACR |
| Organizations + SCP | Management Groups + Azure Policy |
| Transit Gateway | Virtual WAN |
| PrivateLink | Private Link / Private Endpoint |
| Step Functions | Logic Apps / Durable Functions |
| Systems Manager | Azure Arc / Automation / Run Command |
