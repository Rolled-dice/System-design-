# Part 7 — Python & Bash Automation

Type these out. Reading them is not preparation.

---

## A. Bash Fundamentals They Actually Ask

**1. What does `set -euo pipefail` do and why is it the first line of every script?**
- `-e` exit on any non-zero command
- `-u` error on undefined variables (catches typos before they `rm -rf /$UNSET/`)
- `-o pipefail` a pipeline fails if **any** stage fails, not just the last one
Without these, a script silently continues past failures — which in a deploy script means half-applied changes. Add `IFS=$'\n\t'` to avoid word-splitting surprises. Caveat worth mentioning: `-e` doesn't fire inside `if` conditions, `&&` chains, or for functions in command substitution, so it's a safety net, not a guarantee.

**2. `$@` vs `$*`, and why quoting matters.** `"$@"` expands to separate quoted words (correct for passing args through); `"$*"` joins into one string. Unquoted `$@` word-splits on spaces and breaks on filenames with spaces. **Always quote variable expansions** unless you specifically want splitting — this is the single most common Bash bug.

**3. Explain `trap`.** Run a handler on signal or exit: `trap cleanup EXIT INT TERM`. Essential for removing temp files, releasing locks, and restoring state when a CI job is cancelled — directly relevant to the stale-Terraform-lock scenario.

**4. `[[ ]]` vs `[ ]`.** `[[ ]]` is a Bash builtin: no word-splitting, supports `=~` regex, `&&`/`||`, and pattern matching. `[ ]` is POSIX `test`. Use `[[ ]]` in Bash scripts.

**5. How do you make a script idempotent?** Check before acting (`if ! grep -q ... file; then`), use declarative-safe commands (`mkdir -p`, `ln -sf`, `useradd || true` guarded properly), write to a temp file then atomically `mv`, and make the desired end state the target rather than the action. Idempotency is what makes a script safe to re-run after partial failure — say that explicitly.

**6. Useful one-liners to have ready.**
```bash
# Top 10 largest directories
du -xh --max-depth=1 / 2>/dev/null | sort -rh | head -10

# Top 10 memory consumers
ps aux --sort=-%mem | head -11

# Count HTTP status codes in an access log
awk '{print $9}' access.log | sort | uniq -c | sort -rn

# Top 10 IPs hitting the server
awk '{print $1}' access.log | sort | uniq -c | sort -rn | head

# Find files >100MB modified in last 7 days
find /var -type f -size +100M -mtime -7 -printf '%s\t%p\n' | sort -rn

# Delete files older than 30 days, safely
find /backups -type f -mtime +30 -print0 | xargs -0 -r rm -v

# Which process holds a deleted file (disk not freed)
lsof +L1

# Listening ports with process
ss -tulpn

# Watch a rollout
kubectl get pods -w -l app=payment-api

# All non-running pods across the cluster
kubectl get pods -A --field-selector=status.phase!=Running

# Sum of CPU requests per namespace
kubectl get pods -A -o json | jq -r '.items[] | "\(.metadata.namespace) \(.spec.containers[].resources.requests.cpu // "0")"'

# Retry with backoff
for i in {1..5}; do curl -fsS "$URL" && break || sleep $((2**i)); done
```

**7. How do you prevent two copies of a cron script running at once?** `flock`:
```bash
exec 9>/var/lock/myjob.lock
flock -n 9 || { echo "already running"; exit 0; }
```
This is the local equivalent of a distributed lock — and a good segue into how Terraform/etcd do it properly.

---

## B. Bash Coding Tasks

**Task 1 — Health-check script with retries and alerting.**
```bash
#!/usr/bin/env bash
set -euo pipefail

URL="${1:?usage: healthcheck.sh <url> [retries]}"
RETRIES="${2:-3}"
TIMEOUT=5
SLACK_WEBHOOK="${SLACK_WEBHOOK:-}"

log() { printf '%s [%s] %s\n' "$(date -Iseconds)" "$1" "$2" >&2; }

notify() {
  [[ -z "$SLACK_WEBHOOK" ]] && return 0
  curl -fsS -X POST -H 'Content-type: application/json' \
    --data "$(jq -n --arg t "$1" '{text:$t}')" "$SLACK_WEBHOOK" >/dev/null || \
    log WARN "failed to send notification"
}

for ((attempt = 1; attempt <= RETRIES; attempt++)); do
  if code=$(curl -fsS -o /dev/null -w '%{http_code}' --max-time "$TIMEOUT" "$URL"); then
    log INFO "healthy ($code) after $attempt attempt(s)"
    exit 0
  fi
  log WARN "attempt $attempt/$RETRIES failed"
  sleep $((2 ** attempt))          # exponential backoff
done

log ERROR "unhealthy after $RETRIES attempts: $URL"
notify ":rotating_light: Health check FAILED for $URL"
exit 1
```
Talking points: exponential backoff, timeout so it can't hang forever, structured logging to stderr, notification failure doesn't mask the real failure, non-zero exit so the caller (cron/K8s/monitoring) sees it.

**Task 2 — Log rotation / cleanup with a dry-run flag.**
```bash
#!/usr/bin/env bash
set -euo pipefail

DIR="${1:?dir required}"; DAYS="${2:-30}"; DRY_RUN="${DRY_RUN:-true}"

[[ -d "$DIR" ]] || { echo "not a directory: $DIR" >&2; exit 2; }

mapfile -d '' -t files < <(find "$DIR" -type f -name '*.log' -mtime +"$DAYS" -print0)
echo "found ${#files[@]} file(s) older than ${DAYS}d in $DIR"

for f in "${files[@]}"; do
  if [[ "$DRY_RUN" == "true" ]]; then
    echo "[dry-run] would delete: $f ($(du -h "$f" | cut -f1))"
  else
    gzip -c "$f" > "${f}.gz" && rm -f "$f" && echo "archived: $f"
  fi
done
```
Talking point: **dry-run default true**. Any destructive automation should default to safe. Interviewers notice this.

---

## C. Python for DevOps

**8. Why Python over Bash?** Bash for gluing CLI tools in <50 lines. Python when you need: API calls with retries and pagination, JSON/YAML manipulation, error handling and typing, testability, cloud SDKs (boto3/azure-sdk), or anything another engineer will maintain. Rule of thumb: if it has more than a couple of conditionals or needs data structures, use Python.

**9. What Python libraries do you use?** `boto3` / `azure-identity` + `azure-mgmt-*`, `kubernetes` client, `requests` (with `urllib3.Retry`), `pyyaml`, `jinja2` for templating, `click`/`argparse` for CLIs, `tenacity` for retry/backoff, `pytest` + `moto` for mocking AWS, `rich` for output, `pydantic` for config validation.

**10. Explain retries, backoff, jitter, and idempotency for cloud APIs.** Cloud APIs throttle and fail transiently, so retry — but with exponential backoff **and jitter**, because synchronized retries from many clients create a thundering herd that keeps the dependency down. Only retry idempotent operations, or use idempotency tokens. Cap total attempts and respect `Retry-After`. This question is really about whether you'll write automation that amplifies an outage.

---

## D. Python Coding Tasks

**Task 3 — Find and optionally clean up untagged / orphaned cloud resources.**
```python
#!/usr/bin/env python3
"""Report EBS volumes that are unattached or missing required tags."""
import argparse, logging, sys
import boto3
from botocore.config import Config
from botocore.exceptions import ClientError

REQUIRED_TAGS = {"Owner", "Environment", "CostCenter"}
log = logging.getLogger("audit")


def find_waste(region: str):
    ec2 = boto3.client("ec2", region_name=region,
                       config=Config(retries={"max_attempts": 5, "mode": "adaptive"}))
    findings = []
    paginator = ec2.get_paginator("describe_volumes")
    for page in paginator.paginate():                 # always paginate
        for vol in page["Volumes"]:
            tags = {t["Key"] for t in vol.get("Tags", [])}
            missing = REQUIRED_TAGS - tags
            issues = []
            if vol["State"] == "available":
                issues.append("unattached")
            if missing:
                issues.append(f"missing tags: {sorted(missing)}")
            if issues:
                findings.append({
                    "id": vol["VolumeId"],
                    "size_gb": vol["Size"],
                    "type": vol["VolumeType"],
                    "monthly_usd": round(vol["Size"] * 0.08, 2),  # gp3 approx
                    "issues": issues,
                })
    return findings


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--region", default="eu-west-1")
    p.add_argument("--delete", action="store_true", help="actually delete unattached volumes")
    args = p.parse_args()
    logging.basicConfig(level=logging.INFO, format="%(levelname)s %(message)s")

    try:
        findings = find_waste(args.region)
    except ClientError as e:
        log.error("AWS API error: %s", e)
        return 1

    total = sum(f["monthly_usd"] for f in findings)
    for f in findings:
        log.info("%s (%sGB %s) ~$%.2f/mo - %s",
                 f["id"], f["size_gb"], f["type"], f["monthly_usd"], "; ".join(f["issues"]))
    log.info("TOTAL: %d finding(s), ~$%.2f/month potential saving", len(findings), total)

    if args.delete:
        log.warning("delete mode enabled")   # destructive path is opt-in
        # deliberately left as an exercise: snapshot first, then delete
    return 0


if __name__ == "__main__":
    sys.exit(main())
```
Talking points: pagination (a script that misses page 2 gives you false confidence), adaptive retries for throttling, **dry-run by default with `--delete` opt-in**, cost quantified so the output is a FinOps artifact not just a list, structured exit codes for CI.

**Task 4 — Parse a log file and summarize errors (very common live task).**
```python
#!/usr/bin/env python3
"""Summarize error rate and top errors from JSON-lines logs, streaming."""
import json, sys
from collections import Counter
from datetime import datetime

def summarize(stream):
    total = 0
    errors = Counter()
    per_minute = Counter()
    for line in stream:                      # stream: never read a 10GB log into memory
        line = line.strip()
        if not line:
            continue
        try:
            rec = json.loads(line)
        except json.JSONDecodeError:
            errors["<malformed-log-line>"] += 1
            continue
        total += 1
        if rec.get("level", "").upper() in {"ERROR", "CRITICAL"}:
            errors[rec.get("message", "<no message>")[:80]] += 1
            ts = rec.get("timestamp")
            if ts:
                try:
                    per_minute[datetime.fromisoformat(ts).strftime("%Y-%m-%d %H:%M")] += 1
                except ValueError:
                    pass
    return total, errors, per_minute

if __name__ == "__main__":
    total, errors, per_minute = summarize(sys.stdin)
    err_total = sum(errors.values())
    rate = (err_total / total * 100) if total else 0
    print(f"lines={total} errors={err_total} error_rate={rate:.2f}%\n")
    print("Top errors:")
    for msg, n in errors.most_common(10):
        print(f"  {n:6d}  {msg}")
    print("\nPeak error minutes:")
    for minute, n in per_minute.most_common(5):
        print(f"  {minute}  {n}")
```
Talking points: streaming (memory-safe), tolerant of malformed input, error *rate* not just count, and a time histogram so you can correlate with a deploy.

**Task 5 — Toil automation design question: "automate our manual certificate renewal."**
Don't jump to code. Structure it: (1) **Quantify the toil** — how often, how long, who does it, what breaks when it's missed. (2) **Map the manual steps** and find the API for each. (3) **Design for failure** — idempotent, resumable, dry-run, alerting on failure, and a manual override. (4) **Eliminate rather than automate** — the best answer here is *cert-manager with ACME* or Key Vault managed certificates so renewal stops being a task at all. (5) **Measure** — hours saved per month, incidents avoided. (6) **Add a safety net** — an expiry alert at 21/14/7 days that fires regardless, because automation can silently fail.
That last point — *"I'd still alert on expiry even after automating it"* — is the senior answer. Automation that fails silently is worse than a manual process on a calendar.

---

## E. Questions About Your Code

**11. How do you test automation scripts?** `pytest` with `moto` for AWS mocking, `bats` for Bash, dependency injection so cloud clients are mockable, a sandbox account for integration tests, and — critically — **test the failure paths**, since that's where automation causes incidents. Add `shellcheck` and `ruff`/`mypy` in CI.

**12. How do you handle secrets in scripts?** Read from environment or a secret store at runtime; never hardcode, never log them, never pass as CLI args (visible in `ps`); mask in CI output; and prefer short-lived credentials from OIDC/managed identity so a leak has a small window.

**13. How do you decide whether to automate something?** Frequency × time × error-rate × risk versus build+maintenance cost. Automate the frequent and error-prone first. Explicitly recognize that automation is not free — it needs testing, ownership, monitoring, and documentation — and that a badly-owned script becomes tomorrow's unexplained outage. Also mention: automate the *decision-free* parts and keep a human in the loop for judgment calls in prod.
