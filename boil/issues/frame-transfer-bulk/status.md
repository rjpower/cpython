# Bulk frame payload transfer

- State: active
- Owner: stats_design_review; coordinator reviews/integrates
- Updated: 2026-10-31
- Budget: 90 minutes from assignment; checkpoint at 30 minutes
- Depends on: [residual attribution](../interpreter-residual-costs/status.md)
- Branch: boil/frame-transfer-bulk, candidate 5b4d7d9126c
- Worktree: ../boil-frame-transfer-bulk; separate release build running

## Hypothesis

Expressing the existing full-range frame payload move as nonoverlapping copy
followed by source clearing produces simpler/faster native code than the scalar
copy-and-clear loop, while keeping one shared frame lifetime contract.

## Experiment

Audit all three callers, preserve every slot/header/root/publication rule, and
change only the shared payload implementation. Verify nonoverlapping storage and
all-zero NULL representation. Use relevant generator/coroutine/escaped-frame
behavior, inspect emitted O3 code, then run a small matched comparison if viable.
No prefix elision, new frame ABI, dispatch change or workload-specific handler.

## Decision rule

Integrate when behavior is preserved and native code/cost justify the simpler
implementation. Reject if tiny-frame call overhead or regressions outweigh the
benefit, or the caller alias proof fails. Stop at the budget with the evidence
and decision; further ideas become separate experiments. A sampled source share
is a motivation, not a promised saving.

## Now / next

Attribution found a scalar loop in the qualified coroutine image; its sampled
loop interval is 2.476% of that capture's total CPU. The larger whole-helper
share includes other stores and is not the expected gain. The coordinator reviewed the
minimal shared-helper patch; the separate release feedback build is running.
No new timing result or runtime landing is claimed.
