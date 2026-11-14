# Interpreter parity workstream

Status: architecture and coordinator peer review complete on 2026-11-14;
implementation not started. Selected57 `0dcba75bb3df40c32137db453b001f7e8f101de0`
remains the runtime baseline. No new build, test or performance result is claimed.

Read the [stage plan](../interpreter-performance56/plan.md),
[design](../../design/proposals/interpreter-parity58.md),
[research ledger](research.md) and [peer review](review.md). The
[selected report](../comprehensions57/report.md) owns current measured results.
The as-built [IR](../../design/register-ir.md),
[interpreter](../../design/interpreter.md) and
[activation contracts](../../design/activation-transfers.md) remain authoritative
until an increment is integrated.

The design addresses repeated physical ownership/home work and loss of checked
facts between operations. It combines demanded materialization, typed operation
intervals, bounded cross-word execution and ordinary call/resume completion.
It preserves existing exact-call overlap, initialized frontier, inline return,
canonical values and useful specializations. It is not another PyValue rollout.

## Implementation order and ownership

The design's ranked packets and minimum tests are the detailed contract. This
table assigns work without creating duplicate plans or claiming workers started.

| Owner | First admitted packet | Dependency and integration gate |
| --- | --- | --- |
| Coordinator | Bounded path/recipient/materialization census on comprehensions, DeltaBlue, coroutines and generators | Use selected source/native identities. Count residual opportunities after existing passes; assign a non-overlapping removable-work budget. |
| Sol generator worker and independent peer | Typed single-word list interval, preserving SHAPE_LOCAL_TEMP as a negative control | Extend existing Storage/helper facts; demonstrate native check/spill/publication removal and focused protocol/effect correctness. |
| Sol compiler worker and independent peer | Demanded aliases only for a positive surviving multi-use/CFG population | The previous zero-candidate COPY census stays closed. Count inserted materializations and net dynamic savings before implementation. |
| Sol activation worker and independent peer | Carry validated recipient forms through complete call/return and resume/yield pairs | Freeze the common continuation contract first. Preserve redirect, cleanup, exception/running state and existing overlap; audit actual release writes. |
| Coordinator with Sol implementation peer | One high-frequency short physical region and shared checked-provider/effect exits | Require explicit materialization, interior invalidation and observation admission. Low eligible coverage rejects the catalog as a parity strategy. |
| Terra/Luna documentation worker | Update affected as-built contracts after integration | No speculative architecture edits presented as selected behavior. |
| Coordinator | Composition, serialized native tests, quiet timing and selection | Focused tests per increment; full release/debug diagnostics at coherent ownership/region/activation checkpoints; direct selected57/upstream all29 comparisons. |

Workers follow propose → peer review → implement → focused test → actual-diff
review → integration. Root owns generated products and native jobs. Astra's
completed broad review supplies the architecture; it is not in the patch loop.

## Decision gates

Comprehensions execute 1.436416 times upstream instructions and 1.333179 times
cycles. Equal counts require removing about 84.6k instructions/tray, or 30.38%
of selected57 instructions; equal cycles require 24.99% of selected57 cycles.
These are separate objectives, not a prediction or an additive profile budget.

After the first composition, account for removed native work and remaining
coverage. If short regions cannot cover much of the residual gap, choose the
design's rival matching the evidence: a resident value cache, separately admitted
borrowed-argument activation, or larger-block/alternate dispatch lowering.
None is silently part of the initial owner-only protocol. If the evidence
cannot support parity, state that rather than adding isolated opcode tweaks.

Two bounded revisions without native work removal close a mechanism. Repeated
hot-set or sentinel regressions require rejection or an explicit coordinator
tradeoff. Keep logging, raytrace, spectral_norm and nqueens alongside the hot
set. Retain losing results and immutable controls; no layout sweep, second
compiler or benchmark-specific specialization is planned.
