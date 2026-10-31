# Development process

Use the [issue board](issues/README.md) to find current work. Each issue owns
its hypothesis, budget, dependencies, decision and next action.

- **Scope:** start with the [template](issues/_template/status.md); choose the
  smallest useful experiment and an integrate/reject deadline.
- **Work in parallel:** give owners separate files/worktrees. The
  [GC](issues/gc-nursery-minimum/status.md) and
  [interpreter](issues/shape-admission-reuse/status.md) tracks can overlap;
  shared full-suite, perf-map and quiet measurement windows serialize.
- **Review and qualify:** review significant changes independently, then apply
  the [validation contract](design/validation.md) to one coherent candidate.
  Prefer observed behavior; retain actual failures and source/build identities.
- **Decide:** integrate the useful increment or reject it. Follow-ups receive
  their own scope and budget; they do not extend an experiment silently.
- **Report:** update issue status at meaningful transitions and agreed
  checkpoints. Keep [WORKLOG](WORKLOG.md) to brief outcomes with links;
  follow the [documentation rules](DOCUMENTATION.md).
- **Integrate and retire:** the coordinator merges and checks the result.
  Preserve unrelated work and live evidence; use new commits and ordinary
  worktree removal, without rewriting shared history or forced cleanup.
