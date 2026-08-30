# Part 3 — Terraform & Infrastructure as Code

---

## A. State — the #1 interview topic

**1. What is Terraform state and why does it exist?** A JSON mapping between config resource addresses and real-world resource IDs, plus cached attributes and dependency metadata. Without it Terraform can't tell "create new" from "update existing," can't compute a diff, and can't order destroys. It is the source of truth about *what Terraform believes it owns*.

**2. How do you manage state for a team?** Remote backend with locking and versioning: S3 + DynamoDB (or S3 native locking with `use_lockfile` in newer versions), or `azurerm` blob storage with lease-based locking. Requirements: encryption at rest, versioning enabled (your undo button), least-privilege access, and **state segmentation by blast radius**.

**3. How do you structure state files?** Split by lifecycle and blast radius — not by "one giant prod." Typical: `network/`, `data/`, `platform/` (cluster, addons), `apps/<service>/`, per environment. Benefits: smaller plans (seconds not 20 minutes), reduced lock contention, limited damage from a bad apply, and independent team ownership. Consume outputs across boundaries via `terraform_remote_state` data source or, better, a proper contract (SSM/Key Vault parameters) so you don't couple to another team's state internals.

**4. Workspaces vs directories for environments.** Workspaces share one config and backend with separate state — cheap, but easy to apply to prod thinking you're in dev, and they can't express *structural* differences between environments. Directory-per-environment (or Terragrunt) with explicit backend config and separate credentials is the safer, more common production pattern. Say the trade-off, don't just pick one.

**5. Never commit state — why?** It contains secrets in plaintext (DB passwords, keys), it's not mergeable, and concurrent edits corrupt it.

**6. `terraform import` vs `moved` blocks vs `state mv`.** `import` (or `import {}` blocks in ≥1.5, which are plan-reviewable) brings existing infra under management. `moved` blocks / `state mv` change a resource's *address* without touching infrastructure — essential when refactoring into modules, otherwise Terraform plans a destroy+create.

**7. `terraform state` subcommands you should know.** `list`, `show`, `rm` (forget without destroying), `mv`, `pull`, `push`, `replace-provider`. `taint` is deprecated — use `terraform apply -replace=<addr>`.

**8. How do you recover corrupted or lost state?** S3/blob version rollback; `.tfstate.backup`; worst case rebuild by `import`ing every resource (painful — which is why versioning is non-negotiable). Always `terraform state pull > backup.json` before surgery.

---

## B. Core Mechanics

**9. `plan` vs `apply` vs `refresh`.** Plan = refresh state against reality + diff against config → proposed actions. Apply = execute. In CI always `plan -out=tfplan` then `apply tfplan` so what you reviewed is exactly what runs (otherwise the world can change between review and apply). `-refresh-only` shows drift without proposing config changes.

**10. How does Terraform build its dependency graph?** Implicitly from interpolation references (`aws_subnet.x.id` inside an instance creates an edge), explicitly via `depends_on`. It's a DAG, walked with parallelism (default 10). Only use `depends_on` for hidden dependencies (e.g. IAM policy must exist before a resource can use it, but isn't referenced).

**11. `count` vs `for_each` — which and why?** `for_each` almost always. `count` indexes by position, so removing the middle element re-creates everything after it. `for_each` keys by a stable map key, so additions/removals are surgical. This is a very common real-world outage cause.

**12. Explain `lifecycle` blocks.** `create_before_destroy` (zero-downtime replacement — needs unique naming, e.g. `name_prefix`), `prevent_destroy` (guardrail on stateful resources), `ignore_changes` (for fields mutated out-of-band, e.g. autoscaling desired count or a tag written by another system), `replace_triggered_by`.

**13. Data sources vs resources.** Data sources read existing infra (not managed by this config). Beware: they resolve at plan time, so referencing something created in the same apply can fail — use direct references instead.

**14. What are provisioners and why avoid them?** `local-exec`/`remote-exec` run scripts. They break the declarative model, aren't tracked in state, and fail non-idempotently (a failed provisioner marks the resource tainted). Use `cloud-init`/`user_data`, a config-management tool, or a purpose-built image (Packer) instead. Mention this — knowing what *not* to use is a senior signal.

**15. Explain provider version pinning and `.terraform.lock.hcl`.** Pin `required_version` and provider constraints (`~> 5.0`), commit the lock file for reproducible provider versions + checksums. Unpinned providers = a Tuesday morning where a new major version wants to replace your database.

**16. `terraform fmt`, `validate`, `tflint`, `checkov` — where do they fit?** fmt/validate = syntax and internal consistency. tflint = provider-aware lint (invalid instance types, deprecated args). Checkov/tfsec/Trivy = security misconfiguration (public S3, unencrypted volumes, open security groups). OPA/Conftest/Sentinel = *organizational policy* on the plan JSON (`terraform show -json tfplan`), e.g. "no resource destruction in prod," "must have `owner` tag," "no instance larger than X." That last layer is what separates senior IaC answers.

---

## C. Modules & Code Quality

**17. What makes a good module?** A single logical purpose; inputs validated (`variable` with `validation` blocks and types); outputs that expose *contracts*, not internals; no hardcoded provider or backend config (callers own that); versioned via git tag or registry; documented with `terraform-docs`; examples + tests. Bad modules are thin wrappers that add indirection without abstraction, or god-modules with 80 variables.

**18. How do you version and consume modules?** Private registry or git source with `?ref=v1.4.2`. Never `ref=main` for prod — you lose reproducibility. Consumers upgrade deliberately; use Renovate/Dependabot to raise PRs.

**19. How do you test Terraform?** Layers: `validate` + lint (fast), `terraform test` (native, ≥1.6) or Terratest for real apply-assert-destroy in a sandbox account, policy tests with Conftest against plan JSON, and drift detection on a schedule. Ephemeral test environments in a throwaway account are the practical answer.

**20. What is drift and how do you handle it?** Reality diverging from state/config (manual console change, another tool, cloud-side default change). Detect with a scheduled `plan -detailed-exitcode` (exit 2 = drift) and alert on it. Handle by: fixing config to match intent and re-applying, `import`ing legitimate out-of-band resources, or `ignore_changes` where another system legitimately owns the field. Culturally: remove console write access in prod so drift can't happen — that's the real fix.

**21. How do you handle secrets in Terraform?** Never in `.tfvars` in git. Reference them at runtime from Secrets Manager/Key Vault/Vault data sources, inject via environment (`TF_VAR_`) from the CI secret store, use `sensitive = true` to keep them out of logs — and accept that **they still land in state**, which is exactly why state must be encrypted and access-controlled. Best answer: generate credentials outside Terraform, or use dynamic/short-lived identity so nothing static exists.

**22. How do you authenticate Terraform in CI without long-lived keys?** OIDC federation: GitHub Actions / Azure DevOps / GitLab issues a short-lived token, exchanged for an AWS IAM role (`sts:AssumeRoleWithWebIdentity`) or an Azure service principal via workload identity federation. This is the expected 2026 answer; static access keys in CI is an audit finding.

---

## D. Scenario Questions

**23. Terraform is slow — a plan takes 20 minutes. What do you do?** Diagnose: how many resources in state, how many providers, how many data sources hitting slow APIs. Fixes: split the state (biggest win), `-target` only for emergencies, raise `-parallelism` carefully, replace expensive data sources with passed-in values, cache provider plugins in CI, and remove `for_each` over huge maps. Root cause is usually a monolithic state that accreted for two years.

**24. Someone applied from their laptop and broke prod. How do you prevent it?** Remove human credentials for prod writes; only the CI identity can apply. Enforce plan-in-PR + required review + policy gate + manual approval environment. Branch protection, CODEOWNERS on infra directories, and an audit trail (CloudTrail/Activity Log showing only the pipeline role acting). Make the compliant path the *easiest* path or people route around it.

**25. How would you migrate 200 manually-created resources into Terraform?** Inventory first (AWS Config / Resource Graph / `az resource list`). Import in dependency order, in small batches, using generated config (`terraform plan -generate-config-out` with `import` blocks, or Terraformer) as a starting point. After each batch, the success criterion is **`plan` shows no changes** — that's the proof of a clean import. Wrap with `prevent_destroy` on anything stateful before you start, and do the network layer first since everything depends on it.

**26. How do you do zero-downtime infrastructure changes with Terraform?** `create_before_destroy` with `name_prefix`; blue/green at the LB/target-group level; for immutable-field changes on stateful resources, orchestrate a real migration (snapshot/replica promotion) rather than letting Terraform replace. Separate the *provisioning* change from the *traffic-shifting* change into two applies so each is independently reversible.

**27. Terraform vs Pulumi vs CloudFormation/ARM/Bicep vs CDK — when would you not choose Terraform?** Terraform: multi-cloud, huge provider ecosystem, declarative HCL, the de-facto standard (note the OpenTofu fork after the BSL license change — worth mentioning you're aware of it). Choose Bicep/ARM for Azure-only shops wanting first-party support and no state file to manage. Choose CDK/Pulumi when the team wants real programming languages, loops/abstractions, and unit testing in-language. Judgement answer: consistency across the org usually beats marginal tool superiority.

**28. What's the difference between `terraform destroy` in dev and how you'd protect prod?** Dev: nightly destroy to control cost. Prod: `prevent_destroy`, deletion protection at the cloud level, policy-as-code failing any plan with deletes, separate credentials with no delete permissions on stateful resource types, and required two-person approval.

**29. How do you manage multi-account / multi-subscription infrastructure?** Provider aliases per account with `assume_role`, an account-vending/landing-zone pattern (Control Tower / Azure Landing Zones), separate state per account, and a bootstrap module that creates the state backend and CI role in each new account. Never one state spanning many accounts.

**30. What is Terragrunt and do you need it?** A wrapper adding DRY backend/provider config, dependency orchestration between stacks, and `run-all`. Useful for many-environment/many-account repos. Modern Terraform (and stacks/CI tooling like Atlantis, Spacelift, Env0) closes much of the gap — so justify it by team need, not fashion.
