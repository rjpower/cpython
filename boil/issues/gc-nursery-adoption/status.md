# Adopt the nursery as the runtime architecture

- State: active
- Owner: restart_audit implements; independent reviewers check source/helpers; coordinator integrates
- Updated: 2026-10-31
- Base: integrated `d98291018c9665ae3db6952da0b0a40a4e04992b`
- Adopted automatic-major baseline: `ef0af95904b87d888eee4a98b6a745835ff95085`
- Current cadence candidate: `47ccf35e21a94fde9351d1c2e4571bf207257a4c`
- Branch/worktree: `boil/gc-nursery-adoption`; `build/worktrees/nursery-adoption`
- Dispatch started: 2026-10-31 20:18:11 UTC; overall deadline 2026-11-01 04:18:11 UTC
- Current feedback deadline: 2026-11-01 00:14:59.201344 UTC, unchanged after the first build failure
- Decision: keep mandatory nursery admission and real minors; automatic cadence remains an experiment

## Hypothesis

Mandatory admission for the exact cell/nonempty-base-tuple cohort and real
checked minors replace the global nursery switch. One automatic minor halfway
through the existing major byte budget may reclaim useful young work sooner;
its all-old scan must earn its cost through actual elapsed performance.

## Experiment and decision rule

The [adoption design](design.md) defines admission, age-aware verification,
transactional diagnostic records, retirement and independent major debt. The
[concrete cadence design](cadence-design.md) adds only major/opportunity
deadlines and no runtime policy choice. Failed attempts cannot consume a minor
opportunity or reset major debt; explicit full/emergency entries remain majors.

Keep the nursery architecture. Integrate a correct, useful scheduling increment
or reject that increment and retain automatic majors/manual minors as the one
settled policy. Never restore a nursery disable option. Performance uses the
elapsed-first [cost plan](cadence-cost-plan.md), with resource correctness and
explicit memory tradeoffs; text/instruction counts are diagnostics.

## Current evidence

The mandatory-admission baseline passed its [first native feedback](feedback-results.md)
and [independent outcome review](reviews/retirement-feedback-outcome-20261031-v1.md).
Its separate [causal controls](causal-plan.md) detected the nominated manual-kind,
debt-reset and finite-major starvation faults; [outcome review](reviews/causal-controls-outcome-20261031-v1.md)
accepted those exact scopes. That evidence does not qualify the later cadence.

The cadence source froze at `c8cc7718ef3` within its 45-minute source scope.
[Exact-source review](reviews/cadence-source-20261031-v1.md) accepted the seven-file
change and host preparation of thirteen methods/twenty-two children. Native
feedback stopped at its first linked-frame audit before any focused case or
negative ran. Root confirmed session 71061 terminal exit 1 and idle cleanup.
All failed products and the original failure remain preserved.

Successor `5cc1e5310e2` adds precise-frame annotations to the two actually
outlined scalar helpers, changing no scheduling logic, tests or audit rules.
Its [independent correction review](reviews/cadence-precise-correction-source-20261031-v1.md)
accepts fresh reviewed feedback. Remaining-only helpers use fresh builds and
source-negative arms under the original batch deadline. Their
[helper review](reviews/cadence-precise-helper-preparation-20261031-v1.md) accepted
the exact successor. Root activated it at 23:42:14.488441 UTC. Its fresh
build, linked audit and verification passed; focused execution stopped with one
failure among thirteen methods, in the due-major stress=1 child. Root confirmed
session 73424 terminal exit 1 at 23:45:07 UTC and idle cleanup. Independent
[trigger review](reviews/cadence-trigger-correction-design-20261031-v1.md)
proved that the original CellType trigger allocates an argument tuple and a
cell, permitting a major followed by a valid minor. A test-only empty-list
trigger correction is authorized; strict count and liveness assertions remain.
Frozen successor `47ccf35e21a9` changes only that test trigger/comment/result
assertion. Independent source and helper reviews accepted reuse of the 5cc
compiled image and the untouched three negative arms. Root activated seven
remaining rows at 23:55:53.807706 UTC; session 45104 is live. The original
feedback deadline remains fixed. The
[cadence execution record](cadence-feedback-results.md) owns actual
rows, identities, failures and terminal observations.

## Next

Complete reviewed [focused cadence feedback](cadence-feedback-plan.md), including
three exact isolated fault children. Then compare the three frozen source arms
under matched release policy and qualify the one selected coherent candidate.
No cadence performance, broad qualification or landing is accepted yet.

Rebase [broader allocation coverage](../gc-nursery-coverage/status.md) afterward;
its historical global-knob proposal grants no new switch or implementation.
