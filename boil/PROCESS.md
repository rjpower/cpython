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
- **Gate for throughput:** review critical ownership/root/effect boundaries,
  implement and run focused tests, then review actual outcomes and screen on
  elapsed/RSS. Reserve expensive full stress/ASan/stdlib/extension qualification
  for promising candidates; it remains mandatory before integration. Do not
  time a partial ownership implementation or use broad tests to rescue a loss.
  Create required output/cache and lock paths before sealing product inventories;
  activation must not add a file after its inventory check.
- **Separate architecture from mechanism hypotheses:** method and tracing JIT
  development is the selected direction. A component experiment answers its
  declared coverage, correctness, overhead or optimization question; it does
  not repeatedly decide whether a JIT should exist. Exercise useful loops and
  calls early. Reserve full-suite performance and broad qualification for
  coherent adoption candidates, while reviewing recovery and ownership before
  mechanisms that depend on them. Performance regressions remain evidence to
  diagnose and fix; no unfinished component is represented as adopted.
- **Decide:** integrate the useful increment or reject it. Follow-ups receive
  their own scope and budget; they do not extend an experiment silently.
  Check the latest machine-readable decision as well as the original budget.
  The GC budget is closed early; unused time does not authorize another attempt.
- **Validate launch composition:** before any lock is opened, check the actual
  shell/gate/lock/interpreter/command roles in the complete argument vector.
  Replay a host fixture through the composed wrapper. Recheck before launch;
  valid individual helpers do not prove their composition.
- **Match gates to the runtime:** carry exact unchanged evidence, and state
  which checks apply. RC qualification cannot require removed tracing-root
  getters or claim their absent counters are zero. Refleak observation must
  leave the measured callback unchanged and retain no per-iteration records.
- **Report:** update issue status at meaningful transitions and agreed
  checkpoints. Keep [WORKLOG](WORKLOG.md) to brief outcomes with links;
  follow the [documentation rules](DOCUMENTATION.md).
- **Integrate and retire:** the coordinator merges and checks the result.
  Preserve unrelated work and live evidence; use new commits and ordinary
  worktree removal, without rewriting shared history or forced cleanup.
