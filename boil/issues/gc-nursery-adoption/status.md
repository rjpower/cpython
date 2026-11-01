# Adopt the nursery as the runtime architecture

- State: active
- Owner: restart_audit implements; independent reviewers check source/helpers; coordinator integrates
- Updated: 2026-11-01
- Base: integrated `d98291018c9665ae3db6952da0b0a40a4e04992b`
- Adopted automatic-major baseline: `ef0af95904b87d888eee4a98b6a745835ff95085`
- Current cadence candidate: `47ccf35e21a94fde9351d1c2e4571bf207257a4c`
- Branch/worktree: `boil/gc-nursery-adoption`; `build/worktrees/nursery-adoption`
- Dispatch started: 2026-10-31 20:18:11 UTC; overall deadline 2026-11-01 04:18:11 UTC
- First-feedback deadline (closed): 2026-11-01 00:14:59.201344 UTC, unchanged through corrections
- Release-readiness deadline (closed): 2026-11-01 01:05:46.376648 UTC
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
remaining rows at 23:55:53.807706 UTC. All seven rows completed at
00:03:02.533149 UTC; root confirmed session 45104 terminal exit 0 at 00:03:17.
All row cleanup records found no live targets before the original deadline.
Independent [outcome review](reviews/cadence-trigger-outcome-20261101-v1.md)
accepted thirteen methods/twenty-two normal children and three exact causal
diagnostics. Empty due-major callback lists earn no callback-generation credit;
strict collection counts and liveness assertions passed. The original feedback
deadline remains fixed. The
[cadence execution record](cadence-feedback-results.md) owns actual
rows, identities, failures and terminal observations.

The [cost-helper review](reviews/cadence-cost-helper-preparation-20261031-v1.md)
accepted host preparation of the three-arm, 120-fresh-sample/12-origin campaign.
It remains unready: actual release bindings and quiet activation are separate.
Historical e9 source/image/configuration/product reuse passed its host bridge;
six-row release preparation froze early under isolated
`build/nursery-cadence-release-readiness-v1`: two fresh matched O3 builds, two
source-defined focus rows and two registered stale-root rows. Its release
roster explicitly allows two existing debug-only skips and expects fifty-two
normal children; debug child counts are not reused. Independent
[release-helper review](reviews/cadence-release-helper-preparation-20261101-v1.md)
accepted the exact preparation. After terminal IR cleanup, root activated it
at 00:20:46.376648 UTC, deadline 01:05:46.376648. All six rows completed
at 00:26:15.145783 UTC; root confirmed session 61492 terminal exit 0 and all
six idle cleanup records. Independent release outcome review is pending.
The [release execution record](release-feedback-results.md) owns actual rows.
No performance workload or counter probe has run here.

New benchmark activation is held during the user's holistic design review.
The all-old scan and individual death processing remain structural costs;
schedule correctness alone establishes no useful generational performance.

## Next

Establish the two new release arms' actual build/correctness identities under
separately reviewed, bounded execution.
Compare the three frozen source arms under matched release policy and qualify
the one selected coherent candidate.
No cadence performance, broad qualification or landing is accepted yet.

Rebase [broader allocation coverage](../gc-nursery-coverage/status.md) afterward;
its historical global-knob proposal grants no new switch or implementation.
