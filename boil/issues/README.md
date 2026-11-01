# Work board

Each issue's `status.md` owns its state. Experiments have a hypothesis, concrete
budget and integrate/reject decision; see the [template](_template/status.md).

**Hard GC cutoff: 2026-11-01 15:14:44 UTC.** Reach geometric-mean elapsed parity
across all 29 registered workloads against matched upstream, or stop tracing GC
and switch to the prepared reference-counting successor. See the
[twelve-hour decision and gate ordering](gc-parity-deadline/status.md).

## Running in parallel

| Track | Active experiment | Owner | Next decision |
| --- | --- | --- | --- |
| Shared attribution baseline, complete | [Accepted outcome](fundamental-runtime-design/shared-baseline-outcome-review-20261101-v1.md) | coordinator | Use retained current handler evidence and explicit counter limits; no speed claim |
| Compiler / IR | [Accepted region source](fundamental-runtime-design/region-source-review-20261101-v1.md) | restart_audit | Minimal v2 host helper rebinding by 04:25; no target ran in failed preactivation |
| RC fallback preparation | [Accepted RC-only source](fundamental-runtime-design/rc-source-review-20261101-v1.md) | coroutine_design_review | Host-only native helper preparation by 04:40; no promotion |
| GC / allocator | [V3 native outcome](fundamental-runtime-design/span-native-outcome-20261101-v3.md) | coordinator; restart_audit | V3 stopped before cases on test-extension link; narrow core-hook correction awaits review |
| Early elapsed / RSS | [Fixed protocol](gc-parity-deadline/measurement-protocol.md) | qualification_review_resumed | Host-only pilot preparation by 04:40; candidate products and outcome pending |
| Interpreter / backend, queued | [Existing backend controls](h1-tail-dispatch/status.md) | coordinator | Reuse matched compiler/backend plan; no presumed tail adoption |

The [peer-reviewed work breakdown](../design/runtime-work-breakdown.md) replaces
the GC-heavy ranking. Counter-retention tooling is integrated; the baseline
completed and passed independent outcome review. Old cadence, VM0 and
structural-GC helper drafts are preserved and unactivated. Their expired grants
are not resumed by the new assignments.

The user clarified that holistic interpreter and GC improvements take priority.
Regex extraction is a frozen, paused native-interface proposal, not the next
implementation. The six baseline consumers remain a small shared observation
set; their inclusion does not select six separate optimization projects.

Source work, light analysis and reviews can overlap with separate ownership;
quiet measurements, shared full-suite/perf-map windows and integration serialize.
Qualify each coherent increment once. Temporary source comparisons remain
experiments, not permanent runtime choices. The earlier recommendation to retain
the particular nursery design is under review; keeping or rejecting it must
still produce one maintained policy.

## Prior GC sequence, held for design review

The [integrated candidate](../design/integrated-runtime-candidate.md) reopens
this ordering and the particular nursery/storage commitment. The sequence below
records the prior plan; it does not authorize work while the review is active.

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
