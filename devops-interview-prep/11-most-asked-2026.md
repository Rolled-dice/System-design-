# Part 11 — Most-Asked Questions (2026 Research) + Linux Troubleshooting

This file was built from current interview-prep sources (linked at the bottom) to confirm *what is actually being asked* in 2026, and to fill the biggest gap in the rest of this pack: **Linux troubleshooting**, which appears in almost every DevOps/SRE loop.

> *Content from external sources was rephrased and summarized for compliance with licensing restrictions. All sources are linked inline and listed at the end.*

---

## A. What the 2026 market is actually asking

The consistent signal across current guides:

- **Scenario prompts have replaced definitions.** Multiple 2026 guides note that interviews now ask what happens when something is *misconfigured in production* and how you'd locate it, rather than "what is a liveness probe" — see [Verve Copilot on Kubernetes troubleshooting interviews](https://www.vervecopilot.com/blog/kubernetes-troubleshooting-interview-questions) and [Dataquest](https://www.dataquest.io/blog/kubernetes-interview-questions/), which stresses that memorizing definitions isn't enough and interviewers want evidence you can debug real failures and make architectural calls.
- **A fixed set of pod failure states comes up constantly.** [DataCamp's 2026 Kubernetes list](https://www.datacamp.com/blog/kubernetes-interview-questions) identifies scenario debugging of `CrashLoopBackOff`, `OOMKilled`, `ImagePullBackOff` and resource exhaustion as standard in senior interviews. [roadmap.sh](https://roadmap.sh/questions/kubernetes) adds node failure handling, broken Ingress/Services and RBAC misconfiguration, and advises being able to narrate your debugging sequence step by step.
- **Depth on specific mechanics.** [GoLinuxCloud](https://www.golinuxcloud.com/kubernetes-interview-questions/) lists the exact probing areas: pod lifecycle, Service types, probe differences, HPA prerequisites, the NetworkPolicy DNS gotcha, and debugging `Pending`/`CrashLoopBackOff` under pressure — all covered in Parts 1 and 2 of this pack.
- **Terraform questions turn into production incidents.** [Reponotes](https://reponotes.com/blog/terraform-interview-questions-devops-engineers/) describes the pattern precisely: state is locked, a plan wants to replace a database, a module hides provider config, someone imported a resource without reviewing the diff. Those are exactly scenarios S2 and S11 in Part 1.
- **Interviews sweep across the whole lifecycle.** [KodeKloud's 2026 DevOps set](https://kodekloud.com/blog/devops-interview-questions/) frames the typical loop as culture + CI/CD, containers and orchestration, IaC, monitoring, and at least one live incident.
- **Seniority is judged on trade-offs and numbers.** [Hireflow's senior DevOps guide](https://hireflow.net/interview-questions/devops-engineer/senior) recommends STAR answers that name the constraint, walk through a system you owned including trade-offs and what broke in production, and finish with a measurable result.
- **Observability questions test semantics, not syntax.** [GoLinuxCloud's Prometheus set](https://www.golinuxcloud.com/prometheus-interview-questions/) lists pull-based scraping, label cardinality traps, counter vs gauge semantics, `rate()` vs `irate()`, recording rules for expensive queries, and Alertmanager grouping/routing as the real assessment areas.
- **GitOps is now assumed knowledge.** [TechPrep's GitOps question set](https://www.techprep.app/blog/gitops-interview-questions) notes interviewers expect fluent explanations of reconciliation, drift, and pull-based deployment. [GoodSpace](https://goodspace.ai/interview-questions/argocd) highlights ArgoCD's UI and multi-tenancy (AppProjects, ApplicationSets) as the common enterprise choice.
- **DevSecOps is about embedding, not bolting on.** [GoLinuxCloud's DevSecOps set](https://www.golinuxcloud.com/devsecops-interview-questions/) lists shift-left, pipeline gates, container/Kubernetes hardening, Terraform policy, and supply-chain controls as the standard areas — matching Part 8.
- **Linux scenarios are where senior signal shows.** [KodeKloud's Linux guide](https://kodekloud.com/blog/linux-interview-questions/) says explicitly that the hard part is the scenario questions — a slow box, a service that won't start, a disk that reports full but isn't. [PracHub](https://prachub.com/resources/linux-troubleshooting-interview-questions-for-sre-cpu-memory-disk-and-network-scenarios) frames the grading criteria as judgment over command recall: scope an ambiguous symptom, read resource signals correctly, isolate the failing layer, mitigate safely, verify recovery.

**Conclusion for your prep:** the structure this pack already teaches (scope → what changed → mitigate → layer walk → prevent) is exactly what's being graded. The gap to close is Linux, below.

---

## B. Linux Troubleshooting Scenarios (the missing section)

### L1. "The server is slow." Where do you start?

Give a **systematic sweep**, not a random command list. The classic 60-second triage:

```bash
uptime                 # load average vs core count - is it saturated at all?
dmesg -T | tail -30    # OOM kills, disk errors, network resets
vmstat 1 5             # r (runnable) vs b (blocked), si/so (swapping), wa (I/O wait)
mpstat -P ALL 1 3      # per-core: is one core pinned? %steal (noisy neighbour)?
pidstat 1 3            # which process is consuming what
iostat -xz 1 3         # %util, await - disk saturation
free -m                # available vs cached; is it actually memory pressure?
ss -s / ss -tulpn      # connection counts, listeners
top -H                 # thread-level view
```

**Interpretation is the answer, not the commands:**
- **High load + high `%us`** → genuine CPU work; find the process, check for a hot loop or a runaway thread.
- **High load + high `%wa`** → I/O bound; check `iostat` `await` and `%util`, and look for a full disk, a failing device, or a burst-balance cliff on cloud storage.
- **High load + low CPU + low I/O** → processes blocked on locks, or `D`-state processes stuck on NFS/storage.
- **High `%sy`** → syscall storm, context switching, or network interrupt load (`softirq`).
- **High `%st` (steal)** → the hypervisor is throttling you; on cloud that means a noisy neighbour or a burstable instance out of credits.
- **`si`/`so` non-zero** → swapping, which makes everything catastrophically slow; look for a memory leak.

Close with: "then I'd correlate with what changed — deploy, config, traffic, cron — and mitigate before optimizing."

### L2. `df` says the disk is full but `du` says there's space. Explain.

The signature question. Three causes, in order of likelihood:
1. **A deleted file still held open by a process.** `rm` removes the directory entry but the inode and blocks stay allocated until the last file descriptor closes. `du` walks the filesystem tree and can't see it; `df` reads the superblock and can. Find it with `lsof +L1` (or `lsof | grep deleted`), then either restart the process or truncate via `/proc/<pid>/fd/<n>`. Very common with a log file rotated by `rm` instead of `truncate`/`copytruncate`.
2. **Inode exhaustion** — `df -i` shows 100% inodes with free blocks. Millions of tiny files (session files, cached emails, unrotated small logs). You cannot fix this by deleting a few large files.
3. **A mount shadowing data** — something was written to a directory *before* a filesystem was mounted over it, so those blocks are consumed but invisible. Check by mounting the parent elsewhere (`mount --bind`).

Also mention **reserved blocks** (ext4 reserves ~5% for root, so a non-root process gets ENOSPC while `df` shows 5% free) — `tune2fs -m 1` to reduce it.

### L3. A service won't start after a reboot. Walk me through it.

```bash
systemctl status myapp          # state, exit code, recent log lines
journalctl -u myapp -b --no-pager   # full logs this boot
journalctl -xe                  # explanations + recent errors
systemctl cat myapp             # the actual unit file + drop-ins
systemctl list-dependencies myapp
```
Then reason by exit code and cause class:
- **Missing dependency ordering** — the unit starts before the network/database is up. Fix with `After=`/`Wants=`/`Requires=` and `network-online.target` (not just `network.target`).
- **Not enabled** — it ran because someone started it manually; it was never `systemctl enable`d. This is the most common "works until reboot" cause.
- **Permissions/SELinux** — `ausearch -m avc -ts recent` or check AppArmor; a file that worked interactively fails under the service's confined context.
- **Missing runtime state** — the app expected a directory in `/var/run` or `/tmp` that doesn't survive reboot; fix with `RuntimeDirectory=`/`tmpfiles.d`.
- **Port already in use** — `ss -tulpn | grep :8080`.
- **Environment differences** — the unit has a minimal `PATH` and no shell profile; things that work in your interactive shell fail here.
- **Config error** — validate config explicitly (`nginx -t`, `sshd -t`) before blaming systemd.

### L4. You can't SSH to a box. Diagnose without console access.

Layer walk outward-in: **is it a network path problem or a host problem?**
- Ping/ICMP (may be blocked), then `nc -vz host 22` to test the port specifically — distinguishes "host unreachable" from "port closed" from "connection accepted but no banner."
- `ssh -vvv` — where does it stop? DNS resolution, TCP connect, key exchange, or authentication? Each points somewhere different.
- **Connect but auth fails** → wrong key, wrong user, `authorized_keys` permissions (must be 600, home dir not group-writable — the classic), `AllowUsers`/`DenyGroups`, account locked/expired, or SELinux context on `.ssh`.
- **Connect but hangs after banner** → often DNS reverse-lookup timeouts (`UseDNS no`) or a full disk / PAM issue preventing session setup.
- **Port closed** → sshd crashed, or listening on another port/interface.
- **Host unreachable** → security group/NSG/firewall change, route table, instance stopped, or the network stack is down.
- **Sudden loss after a change** → what changed? A firewall rule, a `sshd_config` edit without `-t` validation, or a full `/` preventing login.
- **Escape hatches:** cloud serial console, SSM Session Manager / Azure Run Command / Bastion (which don't need port 22 — mentioning this shows real operational experience), or attaching the volume to another instance.

### L5. High memory usage — is it a leak?

- `free -m` first, and explain the numbers: Linux uses free RAM as page cache, so "low free" is normal and healthy. The number that matters is **`available`**, not `free`.
- `ps aux --sort=-%rss | head` for the top consumers; `smem` or `/proc/<pid>/smaps_rollup` for PSS if shared memory confuses the picture.
- **Leak vs high-water-mark:** a leak shows monotonic RSS growth over hours/days with no plateau. Graph it — a single snapshot can't distinguish them. Check `/proc/<pid>/status` `VmRSS` over time.
- Check `dmesg -T | grep -i oom` for prior kills and which process the kernel chose (and `oom_score_adj`).
- Runtime-specific: JVM heap vs off-heap/metaspace, Go with `GOMEMLIMIT` unset, Node old-space, glibc malloc arenas inflating RSS in multi-threaded processes (`MALLOC_ARENA_MAX`).
- Also: `slabtop` for kernel memory (dentry/inode cache growth from a process opening millions of files), and tmpfs/shm usage counting as memory.
- **Mitigate then fix:** restart on a schedule as a stopgap while you get a heap profile; the real fix is the profile, not the restart.

### L6. A process is using 100% CPU. Find out what it's doing.

- `top -H -p <pid>` for the hot thread, then map the thread ID to application-level detail.
- `strace -c -p <pid>` for syscall counts (a syscall storm — e.g. tight `epoll` or `stat` loop — looks like CPU burn), `ltrace` for library calls.
- `perf top -p <pid>` / `perf record` + flamegraph for on-CPU profiling — the real answer for "what code is hot."
- Language-native: `jstack`/async-profiler for JVM, `py-spy` for Python, `pprof` for Go. Naming a *language-appropriate* profiler is a strong signal.
- Check `/proc/<pid>/wchan` and thread states, and whether it's a spin-lock or a busy-wait retry loop caused by a *failing dependency* (a very common real cause — the CPU burn is a symptom, not the disease).

### L7. Network troubleshooting toolkit

```bash
ip a; ip r                 # addresses and routes (not ifconfig/route)
ss -tulpn                  # who's listening
ss -ti                     # per-socket TCP info: retransmits, RTT, cwnd
nc -vz host port           # is the port reachable
dig +short host; dig @8.8.8.8 host   # DNS, and DNS via a different resolver
mtr host                   # per-hop packet loss (better than traceroute)
tcpdump -ni any port 443 -c 100      # prove what's actually on the wire
curl -w '@curl-format.txt' -o /dev/null -s https://host   # DNS/connect/TLS/TTFB breakdown
ethtool -S eth0 | grep -i drop        # NIC-level drops
netstat -s | grep -i retrans          # TCP retransmits
conntrack -C                          # conntrack table usage
```
Key interpretations: **connection refused** = nothing listening (fast failure); **timeout** = firewall dropping silently; **DNS resolves but connect fails** = network path/security group; **works by IP but not name** = DNS; **intermittent with retransmits** = packet loss or conntrack exhaustion; **TLS handshake failure** = cert/SNI/protocol mismatch. The `curl` timing breakdown is the single most efficient tool for "which phase is slow."

### L8. Rapid-fire Linux theory they ask alongside

- **Process vs thread** — separate address space vs shared address space, different memory/context-switch cost.
- **Zombie vs orphan** — exited but unreaped (parent didn't `wait()`) vs parent died and it was reparented to init. Zombies consume a PID slot, not memory; the bug is in the parent.
- **`kill` vs `kill -9`** — SIGTERM lets the process clean up; SIGKILL can't be caught, so no cleanup, possible corruption. Always try SIGTERM first.
- **Signals to know** — SIGTERM (15), SIGKILL (9), SIGHUP (1, conventionally reload config), SIGINT (2), SIGSTOP/SIGCONT.
- **Load average** — runnable + uninterruptible-sleep processes over 1/5/15 min; compare to core count, and remember I/O wait inflates it on Linux.
- **`vm.swappiness`** — how aggressively the kernel swaps; low (1–10) for latency-sensitive services, and swap is usually disabled entirely for Kubernetes nodes.
- **Hard vs soft link** — separate directory entry to the same inode (same filesystem, survives rename of the original) vs a path pointer that breaks if the target moves.
- **File permissions** — `chmod` numeric, plus **setuid/setgid/sticky bit** (`/tmp` uses sticky so users can't delete each other's files); `umask` for defaults.
- **`/proc` and `/sys`** — kernel-exposed process and device state; where every monitoring tool actually gets its numbers.
- **cgroups vs namespaces** — resource limits vs isolation; together they make containers.
- **`nice`/`renice`, `ionice`** — CPU and I/O scheduling priority.
- **`ulimit`** — per-process limits; `nofile` (open files) is the one that breaks production services at scale.
- **systemd targets vs runlevels**, `systemctl daemon-reload` after editing a unit.
- **Log locations** — `journalctl` for systemd, `/var/log/messages`|`syslog`, `/var/log/audit/audit.log`, and per-app logs.
- **`crontab` vs systemd timers** — timers give you logging, dependencies, and randomized delays; `flock` to prevent overlapping runs.
- **How to find what's using a port** — `ss -tulpn` or `fuser -n tcp 8080`.
- **How to safely fill/test a disk** — `fallocate`; and how to find large files — `find / -size +1G -type f`.

---

## C. Theory questions asked in nearly every DevOps loop

Have crisp 30-second answers ready. Details are in the earlier parts of this pack.

1. What is DevOps, in your own words? (Culture + practices reducing lead time and improving reliability through shared ownership — *not* "a set of tools" and *not* "a job title.")
2. What problem does CI actually solve? (Late, painful integration and long feedback loops.)
3. What is Infrastructure as Code and what does it give you? (Reproducibility, review, audit, disaster recovery.)
4. Declarative vs imperative — and why declarative wins for infrastructure.
5. Immutable infrastructure — replace, don't patch; no snowflakes; the artifact is the unit of change.
6. Push vs pull deployment models, and why GitOps chose pull.
7. Mutable vs immutable image tags — and why `latest` is a production incident waiting to happen.
8. Blue/green vs canary vs rolling — and when each is correct.
9. Horizontal vs vertical scaling — and why horizontal needs statelessness.
10. Stateless vs stateful, and why state is the hard part of every migration.
11. CAP theorem in practical terms — partition tolerance isn't optional, so you're choosing between consistency and availability during a partition.
12. Idempotency — why every automation and every message consumer needs it.
13. Backward compatibility — why it's the precondition for zero-downtime deploys.
14. Monolith vs microservices — and the operational tax microservices charge (network, observability, data consistency, deployment coordination).
15. What is a reverse proxy / load balancer, and what does L4 vs L7 change?
16. TLS handshake basics, certificate chains, and what expires when.
17. DNS records and TTL — and why TTL matters for failover.
18. TCP vs UDP, and the three-way handshake.
19. What happens when you type a URL and press enter? (Full path: DNS → TCP → TLS → HTTP → LB → app → DB → back. Interviewers use this to test breadth.)
20. Git branching strategies, and rebase vs merge (and why you don't rebase a shared branch).
21. What is a postmortem, and what makes it blameless — and useful?
22. MTTR vs MTBF, and why reducing MTTR usually beats chasing MTBF.
23. What is toil, and what makes it worth automating?
24. SLI/SLO/SLA and error budgets.
25. Shift-left security — what moves, and what stays at the gate.

---

## Sources

Consulted for the "most-asked" analysis in this file. All content above was paraphrased and summarized; no substantial verbatim text was reproduced.

- [Verve Copilot — Kubernetes Troubleshooting Interview Questions (2026)](https://www.vervecopilot.com/blog/kubernetes-troubleshooting-interview-questions)
- [DataCamp — Kubernetes Interview Questions and Answers 2026](https://www.datacamp.com/blog/kubernetes-interview-questions)
- [DataCamp — Terraform Interview Questions (2026)](https://www.datacamp.com/blog/terraform-interview-questions)
- [Dataquest — Kubernetes Interview Questions Interviewers Actually Ask](https://www.dataquest.io/blog/kubernetes-interview-questions/)
- [roadmap.sh — Top 50 Kubernetes Interview Questions](https://roadmap.sh/questions/kubernetes)
- [GoLinuxCloud — Kubernetes Interview Questions 2026](https://www.golinuxcloud.com/kubernetes-interview-questions/)
- [GoLinuxCloud — Prometheus Interview Questions 2026](https://www.golinuxcloud.com/prometheus-interview-questions/)
- [GoLinuxCloud — DevSecOps Interview Questions 2026](https://www.golinuxcloud.com/devsecops-interview-questions/)
- [GoLinuxCloud — Linux Interview Questions for Experienced Admins 2026](https://www.golinuxcloud.com/linux-interview-questions-for-experienced-users/)
- [Reponotes — Terraform Interview Questions for DevOps Engineers (2026)](https://reponotes.com/blog/terraform-interview-questions-devops-engineers/)
- [KodeKloud — DevOps Interview Questions 2026](https://kodekloud.com/blog/devops-interview-questions/)
- [KodeKloud — Linux Interview Questions 2026](https://kodekloud.com/blog/linux-interview-questions/)
- [PracHub — Linux Troubleshooting Interview Questions for SRE](https://prachub.com/resources/linux-troubleshooting-interview-questions-for-sre-cpu-memory-disk-and-network-scenarios)
- [Hireflow — Senior DevOps Engineer Interview Questions (2026)](https://hireflow.net/interview-questions/devops-engineer/senior)
- [TechPrep — GitOps Interview Questions and Answers (2026)](https://www.techprep.app/blog/gitops-interview-questions)
- [GoodSpace — ArgoCD Interview Questions (2026)](https://goodspace.ai/interview-questions/argocd)
- [Intellipaat — Top Kubernetes Interview Questions 2026](https://intellipaat.com/blog/interview-question/kubernetes-interview-questions-answers/)
- [Techoral — DevOps Interview Questions 2026](https://techoral.com/automation/devops-interview-questions.html)
- [Hero Vired — Top 100 DevOps Interview Questions & Answers (2026)](https://herovired.com/learning-hub/blogs/devops-interview-questions)
- [devops-interviews/devops-interview-questions (GitHub)](https://github.com/devops-interviews/devops-interview-questions)
