# Development process

Work in bounded issues with one owner and a concrete result. The
[issue board](issues/README.md) identifies current work; an issue's status is
the authority for its scope, dependencies and next action.

## Work

- Start from the [status template](issues/_template/status.md). Write only the
  design, plan or result documents the task needs; follow the
  [documentation rules](DOCUMENTATION.md).
- State the hypothesis, time budget, smallest useful change, observable checks
  and stopping rule. At the boundary, integrate or reject the attempt and record the result;
  optional follow-ups do not silently expand the task.
- Keep changes focused and preserve unrelated working files. Owners use
  separate managed worktrees and do not edit another active owner's files.
- Review significant designs and implementations independently. Resolve concrete
  findings; record decisions in the issue rather than adding approval stages.
- Prefer behavioral checks of data, liveness, resources and bounded memory.
  Use focused internal controls for critical ownership and rollback invariants.
- Freeze source and products while they are in use. Preserve original failures
  and actual source/build/mode identities; a component pass is not a union pass.

## Validate and integrate

- Use the [validation contract](design/validation.md) and existing
  [gate and runner tools](tools/README.md). Select finite checks for the change;
  qualify a coherent landing instead of repeating whole ladders for unchanged code.
- Coordinate the shared machine slots and build locks. Serialize full suites
  and perf-map work; reserve quiet measurement windows separately.
- Review the complete diff and applicable evidence before integration. The
  coordinator owns the integration branch and postmerge checks.
- Use new commits for corrections. Do not amend, rebase, reset, force-push or
  use the shared stash. Keep commit messages focused and omit agent credits.
- Retire worktrees only after preserving unique commits, user changes and live
  evidence dependencies. Use ordinary removal, never forced cleanup.

## Report

- Update status at start, handoff, failure, review readiness and landing.
  Report material outcomes promptly and active progress at agreed checkpoints.
- Link detailed results and follow-ups. Append only concise, high-level
  outcomes to [WORKLOG](WORKLOG.md); do not duplicate gate logs or task queues.
- A status transition does not waive existing validation or add an approval step.
