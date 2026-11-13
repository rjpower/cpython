# Project status

Current work is [stage2 interpreter performance](issues/interpreter-performance56/plan.md):
one combined architecture for value transport, borrowing, calls/returns and
specialization. Astra completed its [combined design](design/proposals/interpreter-performance56.md)
and [audit](issues/interpreter-performance56/review.md); Sol source peer review
and foundation qualification are active.
The [goal](GOAL.md) and [architecture](design/runtime-architecture.md) define
the project; [HANDOFF](HANDOFF.md) owns exact execution state.

## Selection and performance

Selected54 remains golden: `61015a9452d9069646fb5fede9ff963896833b89`, branch
`pyjit/round54-golden`, tag `pyjit-golden-20261112-frame-state`. No model55
performance candidate has earned promotion.

Latest measured integration is **f6ef4a81a87**, not selected. Both builds
complete 117 modules/79 shared; nine completion/observer modules pass 84 methods
per role. Two balanced pairs against golden54 give these elapsed ratios.

| Benchmark | Candidate/golden54 | Pair range |
| --- | ---: | ---: |
| DeltaBlue |1.022672 |1.003553–1.042155 |
| Generators |0.949424 |0.948526–0.950322 |
| Coroutines |1.065409 |1.026978–1.105277 |
| Richards |0.968734 |0.967106–0.970366 |
| Comprehensions |1.027499 |1.022269–1.032757 |
| Silent logging |1.017075 |0.956376–1.081626 |

Identities are stable and all commands succeed. Generators improve in both
pairs; the overall result remains mixed and is not promoted. There is no
same-head upstream/broad/PMU result. The earlier **6a00c6a08d4** broad screen
has all 29/upstream geometric mean 1.161978, six wins. Matched PMU native
instruction ratios are 1.366815 DeltaBlue, 1.492557 generators and 1.353759
coroutines. Those results do not transfer to newer source. The
[current report](issues/interpreter-performance56/report.md) owns exact-head
tables, profiles and pair ranges. Historical evidence stays in model55's report.

Golden54's full comparison is all 29/upstream 1.156543, six wins. Its
[report](issues/frame-state54/report.md) retains benchmark tables, rejected
variants and qualification. Startup RSS and zero-box counts are not throughput
gates.

## Implemented architecture

Canonical eight-byte PyValues span frames, containers, object storage and
rebuilt bundled extensions. C-local typed borrows and owning bridges are
implemented. Native results may still use private numeric boxes until actual
home publication; improving that transport is distinct from value integration.

The register interpreter is implemented. Existing frontend borrow selection
and register copy propagation eliminate many local-read instructions. Exact
calls already overlap arguments in one frame arena and return inline. Warm
selected storage uses an initialized frontier. Source positions are separate
from semantic exception/recovery state. Native method and tracing JIT tiers
remain unfinished.

## Frozen model55 components

These remain experimental components, not selected replacements:

| Component | Actual state |
| --- | --- |
| Scalar facts | Native8232 qualifies the first GET_ANEXT/UNARY_NOT error snapshots. VALUE/error-copy plus isolated repair pass 33 host checks and full emission. The new chained runtime proof fails in both initial native roles. Second replay repair469b and generator recipient0888 are integrated at If6ef4 with regenerated products and 117/79 builds. The chained proof now passes; a synthetic debugger target must supply an integer line to reach its intended check. Completion/observer checks pass 84 methods per native role. Full static scratch authority is unfinished. |
| Bitmap execution readers | B0e918 builds 117/79 in both roles. All34 focused cases pass release. Debug aborts in a constructed HomeRead image at LOAD_COMMON_CONSTANT; its other four selected modules pass. Earlier release lifecycle diagnostics pass seven modules but fail monitoring/profiling with absent-input errors. No complete bitmap timing or qualification. |
| Explicit exceptional transfers | Compiler071d, readers4c646 and runtime/container9c297 are source-reviewed with Clinic/AST checks only. IRSC2 names retained homes and protocol destinations explicitly. Full generation and native qualification remain pending; the allocator is still canonical. |

The [model55 ledger](issues/call-model55/implementation.md),
[report](issues/call-model55/report.md) and frozen
[design](design/proposals/register-call-model55.md) retain component history.
The active plan decides which pieces are needed by each performance increment;
completing all old experiments is not an automatic gate for unrelated gains.

## Qualification limits

No green full model55 suite follows the allocation-injection repair: its
original reproducer and837 focused cases pass, but the last full foundation
run found the crash. Selected54 also records debugger-jump failures, LLTrace,
GC-verifier instability and an unexplained full-run profiling failure. These
remain explicit; they are not called fixed or silently quarantined. Only the
existing GDB pretty-print class is quarantined. Current qualification is GIL-only.

Canonical documentation is this checkout. Its C source is not the measured
runtime. Preserve immutable controls and evidence; use the handoff before
building or measuring any candidate.
