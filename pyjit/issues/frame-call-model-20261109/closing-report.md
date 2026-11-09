# Frame/call round: closing report

This report is being finalized against frozen runtime
`4d666e1b482cb4983c7683f91c5922af14d0ab40`. The runtime implementation is
`24f4d270ced`; the final commit adds reviewed debugger tests. Native and
benchmark inputs are frozen. The complete final benchmark matrix is pending;
no prior campaign is presented as its result.

Read [status](../../STATUS.md) for the current outcome,
[the plan](plan.md) for closure gates, and [the next five priorities](next-steps.md)
for proposed follow-up. Canonical designs:
[IR](../../design/register-ir.md), [interpreter](../../design/interpreter.md),
[frames/calls](../../design/call-and-frame-lifetimes.md), and
[PyValue](../../design/pyvalue.md).

## What changed

The round keeps one canonical eight-byte value and owning-frame model. It makes
source/event boundaries metadata, reuses initialized frame storage, transfers
arguments through overlapping windows, binds common Python calls directly,
materializes eligible empty kwargs only when needed, and names return recipients
and real continuations explicitly. Iteration/delegation completion and
commit-before-close owner transitions share their contract across providers.
Canonical stores use bounded producer proofs instead of repeating conversions.
Observer and frame-jump analysis stays cold and separate from hot execution.

These are interpreter changes, with the JIT disabled for measurements. The
native method/tracing tiers remain unfinished. Extensions are rebuilt against
the canonical-value runtime; owning bridges remain legitimate incremental ports.
Python semantics, ownership and safe reentry govern behavior. CPython's prompt
finalizer timing and internal presentation are not requirements.

## Qualification

All release, debug and stats builds check 117 modules. Stats smoke passes 150
tests. Final release full diagnostic completes **54,645 tests**, with two failed
modules: GDB pretty-printing (16 failures) and frame jumps (nine errors).
Pdb passes. Final debug completes the same **54,645 tests**, with three failed
modules: GDB (21 failures), frame jumps (nine errors), and one low-level trace
assertion expecting a removed cleanup instruction. The full suite is therefore
not green, and this report does not claim otherwise.

GDB was previously absent, so its optional tests had been skipped. Installing
it exposed a tooling port: the reader registers the old value type and the
harness expects the old `id` C arguments. Exact rendering is not an interpreter
or performance gate. The remaining frame-jump cases are reported capability
limits, not reclassified successes. Four unchanged synchronous jumps pass;
eleven deliberately unsupported protocol crossings now have explicit refusal,
unchanged-state and full normal-cleanup controls. Both Pdb backends have a real
immediate-entry/continuation test.

The historical GC negative-control failure did not recur in the final release
run. A later pass does not establish its original cause. Failed attempts remain
retained. Current value-API qualification passes **306 translation units**, with zero
failing findings and parse errors. The existing compile database remains valid
for these unchanged translation-unit/build configurations. Notes (91 home-copy,
151 slot-read and one unchecked) are retained diagnostics, not failing findings.

## Measurement coverage and controls

Final measurements cover all 29 application kernels, all 65 registered micro
instruction workloads and six timed allocation/collector workloads. The two
RSS-only startup/import rows are excluded under the user's direction. Kernels
use balanced paired elapsed timing against golden and upstream; logging has two
independent six-round confirmation campaigns. Micro instructions use the
unchanged four differenced pairs and skeleton subtraction. Memory uses the
unchanged five timed calls after warm-up; its serial three-build sweep is a
coverage diagnostic, not a balanced elapsed-time claim.

All native runs serialize. GCC15.2, CPU15, hash seed0, JIT off, checked module
rosters and clean source/native identities are fixed. Retained raw samples,
perf journals and before/after hashes bind the results to the exact ELFs,
benchmark/stdlib inputs and extensions. Geometric means weight each kernel
once. Instruction reductions and sample shares are not causal speedup budgets.

Immutable golden: `0765e9be47f55d97ba5d31d2ae1c02a55ea2e5ae`.
Matched upstream: `3a440b65695615e9fd9912342b79cdd54fced0d2`.
Final per-kernel, micro and memory tables will be attached after collection.

## Independent architectural review

Two separate V8-style engine reviews inspect the frozen implementation and
contracts. They agree that owning homes, explicit continuations, overlapping
arguments and transactional retirement are sound foundations. The main shared
criticism is duplicated ownership-effect knowledge across lowering, emission,
validation and future recovery. Neither reviewer claims V8 affiliation.
Reviewers participated in earlier workstreams; these are independent closing
assessments, not blind reviews by external engineers. The session's agent-thread
limit prevented adding another fresh reviewer.

The coordinator's [ranked five-item plan](next-steps.md) combines their findings:
structured emitted effects, semantic retirement, direct owning heap creation,
proved recipient classes, and measured compiled-path/feedback layout. Each has
a bounded first experiment and a falsifiable gate. These proposals do not
extend the closing round or promise unmeasured gains.

Detailed reviews: [architecture](closing38-cold-architecture-review.md),
[implementation](final-engine-architecture-review38.md).

## Excluded drafts and cleanup

The coroutine ingress packet, owner-bundle draft and partial GDB port are not
part of this image. The coroutine packet passed release but debug asserted on
a synthetic handler-to-RESUME negative; its exact patch was reversed. Complete
source patches and identities are preserved under
`E11/deferred-after39-source/`, with the authoritative coroutine packet separate.
The task-owned isolated worktree and branch were removed after hash verification.
Unrelated historical worktrees, user changes and sealed evidence are preserved.

Canonical documentation owns current state. Loom mirrors one goal, plan, status,
report and current designs/review. Six historical active artifacts are preserved
with their complete one-revision contents in
[the archive](../../archive/20261109/loom-artifacts.json) before active cleanup.
The full old PyValue page is preserved while the current reference describes
actual encoding and typed ownership without rollout chronology.

Raw evidence root E11:
`build/pyjit-link-wave1/frame-call-model-20261109/` under python-boil-the-ocean.
The final report will name each completed campaign and retain its limits.
