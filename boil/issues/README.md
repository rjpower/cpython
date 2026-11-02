# Work board

Each issue's `status.md` owns its state. Experiments have a hypothesis, concrete
budget and integrate/reject decision; see the [template](_template/status.md).

**GC work closed early at 07:09 UTC on 2026-11-01.** All measured candidates
were rejected. The reference-counting successor is qualified and merged locally;
fresh main builds and integration checks are accepted. No full29 GC or RC parity claim. See the
[decision](gc-parity-deadline/decision-20261101.md) and
[active RC integration issue](rc-integration/status.md).

## Workstreams and decisions

**Current priority:** restore the universal PyValue representation that was
removed with the RC restoration. The user reaffirmed that collector fallback
must retain value-based interpreter operations and heap containers. RC remains
selected; see [PyValue restoration](pyvalue-rc-restoration/status.md). New JIT
native validation is held while its value contract is reconciled.

| Track | Active experiment | Owner | Next decision |
| --- | --- | --- | --- |
| Shared attribution baseline, complete | [Accepted outcome](fundamental-runtime-design/shared-baseline-outcome-review-20261101-v1.md) | coordinator | Use retained current handler evidence and explicit counter limits; no speed claim |
| Compiler / IR | [Rejected region result](fundamental-runtime-design/region-elapsed-outcome-review-20261101-v1.md) | coordinator | Closed on measured loss; no new local IR source attempt |
| RC integration | [Accepted runtime](rc-integration/status.md) | coordinator | Closed: fresh release/debug builds and 42 methods each accepted, zero skips; historical limits retained |
| First native JIT increment | [Rejected fixed result](jit-first-native-tier/status.md) | coordinator; restart_audit; coroutine_design_review; qualification_review_resumed | Native correctness/emission passed; aggregate elapsed +2.01%/+5.31% rejects67f5d22. No resampling or late qualification. Separate entry/compiler-tax diagnosis only; RC stays active |
| JIT entry PC ABI | [Separate hypothesis](jit-entry-pc-abi/status.md) | coroutine_design_review; restart_audit; qualification_review_resumed; coordinator | Focused behavior and pilot passed; full29 GM +2.26%/+1.94% rejects599897. No publication, late qualification or resampling. Separate admission hypothesis below; RC stays active |
| JIT entry admission | [Bounded source attempt](jit-entry-admission/status.md) | restart_audit; coroutine_design_review; qualification_review_resumed; coordinator | Closed: focused mechanism passed, full29 +0.52%/+0.45% rejects11aefbdd. No publication, late qualification or resampling; frame filters closed |
| Selected IR block entry | [Component evidence](jit-quickened-block/status.md) | coordinator | Source accepted, unexecuted; superseded as a tier by user feedback36. Interrupted activation did not take effect |
| Universal values | [Restore PyValue under RC](pyvalue-rc-restoration/status.md) | coordinator; restart_audit; coroutine_design_review; value_api_consumer_map | Storage, VM readers/stores, compiler cache/keys and repr source-accepted; full compile.c syntax passes. Attribute/native-scope cuts continue; code-object and native/Clinic closure precedes coherent qualification and JIT-off measurement |
| Method and tracing JIT | [M3 partial native evidence](jit-e2e-system/status.md) | coordinator | Fresh build passes;35/36methods pass including8trace tests. Zero-loop correction95be1d28 is frozen/source-accepted; next native batch held for universal-value restoration. No speed claim or primary integration |
| GC / inventory | [Rejected fixed pilot](gc-nursery-old-work/status.md) | coordinator | Closed; no further tracing-GC grants |
| Early elapsed / RSS | [Rejected storage result](fundamental-runtime-design/span-elapsed-outcome-review-20261101-v6.md) | coordinator | Closed on 51% coroutine loss; no additional samples or expensive qualification |
| Interpreter / backend | [Measured decision](h1-tail-dispatch/reviews/elapsed-outcome-20261101-v1-root.md) | coordinator | Closed: reject Clang computed/tail; GCC computed fixed for GC timebox |

The [peer-reviewed work breakdown](../design/runtime-work-breakdown.md) replaces
the GC-heavy ranking. Counter-retention tooling is integrated; the baseline
completed and passed independent outcome review. Old cadence, VM0 and
structural-GC helper drafts are preserved and unactivated. Their expired grants
are not resumed by the new assignments.
The maintained-baseline page-map port was separately tested and rejected.

The user clarified that holistic interpreter and GC improvements take priority.
Regex extraction is a frozen, paused native-interface proposal, not the next
implementation. The six baseline consumers remain a small shared observation
set; their inclusion does not select six separate optimization projects.

Source work, light analysis and reviews can overlap with separate ownership;
quiet measurements, shared full-suite/perf-map windows and integration serialize.
Qualify each coherent increment once. Temporary source comparisons remain
experiments, not permanent runtime choices. The collector decision is RC with
cyclic collection; qualification precedes integration, and no permanent runtime
collector option is added.

## Prior GC sequence, closed

The [integrated candidate](../design/integrated-runtime-candidate.md) and sequence
below record the former tracing-GC plan. GC work is closed; RC with cyclic
collection is selected. This sequence authorizes no further collector work.

1. [Adopt real minor/checker semantics](gc-nursery-adoption/status.md), then
   select one measured automatic cadence without a runtime switch.
2. [Broader coverage and cheaper reclamation](gc-nursery-coverage/status.md):
   rebase the exact-coroutine proposal on adoption; its opt-in grant is superseded.
3. [Reduce old-object scan work](gc-nursery-old-work/status.md).

Each follows the preceding result; the initial nursery need not solve the full
future allocator, copying, lifecycle and barrier design at once.

## Unassigned follow-ups

- [Verification orchestration](verification-orchestration/status.md).
- [Root/checker audit](root-checker-audit/status.md).

These are intake scopes. Split a bounded hypothesis before dispatching work.

## Deferred

- [Physical liveness/root maps](h2-root-maps/status.md).
- [Foreign interpreter lifecycle](gc-foreign-lifecycle/status.md).
- [Handler tooling consumers](handler-tooling-consumers/status.md).

JIT tiers and broader package ports follow interpreter work and settled native
contracts. Roughly 2× interpreter speed is guidance, not a numerical gate.

## Rejected

- [Compiler-proven compare/branch regions](fundamental-runtime-design/region-elapsed-outcome-review-20261101-v1.md): local bool-home/second-dispatch removal verified, but coroutine medians regressed 10.86% and 14.11%; four-row geometric means regressed 16.21% and 13.74%. No further samples or qualification.
- [Handler-local slot bases](interpreter-dispatch-state/status.md): coroutine medians regressed 11.98% and 11.00%; all controls also regressed. Native store removal did not earn an elapsed improvement.
- [Thirteen constant-operand handlers](interpreter-constant-operands/status.md): coroutine medians regressed 4.47% and 11.64% in the fixed elapsed comparison; controls also regressed. Code size was diagnostic, not the rejection rule.
- [Bulk frame transfer](frame-transfer-bulk/status.md): no repeatable gain in two matched process rounds; no further qualification.

## Reported

- [Documentation consolidation](documentation-migration/status.md): complete; current contracts, concise workflow and bounded experiment template adopted.

- [Residual attribution](interpreter-residual-costs/status.md): independently replayed finding selected the frame-transfer experiment; no new benchmark was needed.
- [Call setup attribution](interpreter-call-setup/status.md): existing exact calls
  already use value windows; present profiles do not support a generic interface
  rewrite. Hypothesis rejected without a runtime change or new benchmark.
- [Lowering and LuaJIT staging](interpreter-lowering-research/status.md): current
  local optimizations and interblock liveness are substantial; a general SSA
  optimizer is absent. The constant-operand experiment was rejected on elapsed
  performance; common evaluator state is the active follow-up. Literal read
  subscripts and compare/branch fusion remain unassigned design candidates.

## Landed

- [Minimal nursery and shared shape admission](nursery-shape-landing/status.md):
  exact source, combined qualification, actual merge bridge, release/debug smokes
  and independent closure accepted at d98291018c9. The temporary nursery control
  remains retirement debt while adoption is deferred; the next collector
  adoption must select one policy and remove the control. No new speedup is claimed.
- [GC cleanup and scanning](gc-cleanup-scanning/status.md): qualified, integrated
  and measured. This is the comparison baseline for the new experiments.
