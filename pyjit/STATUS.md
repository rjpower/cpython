# Project status

Current work is [stage2 interpreter performance](issues/interpreter-performance56/plan.md):
one combined architecture for value transport, borrowing, calls/returns and
specialization. Astra is reviewing it at high effort before implementation.
The [goal](GOAL.md) and [architecture](design/runtime-architecture.md) define
the project; [HANDOFF](HANDOFF.md) owns exact execution state.

## Selection and performance

Selected54 remains golden: `61015a9452d9069646fb5fede9ff963896833b89`, branch
`pyjit/round54-golden`, tag `pyjit-golden-20261112-frame-state`. No model55
performance candidate has earned promotion.

Latest measured static source/native is 8232dd8adde. Both builds complete117
modules,79 shared, and pass 23 focused plus 233 IR/code cases per role and the
5+27 bootstrap/compile matrix. Two balanced pairs give the following elapsed
ratios; lower is better. The two control campaigns are independent.

| Benchmark | Candidate/golden54 | Candidate/upstream |
| --- | ---: | ---: |
| DeltaBlue |1.017884 |1.177991 |
| Generators |0.997913 |1.430017 |
| Coroutines |1.022717 |1.292308 |
| Richards |0.978000 |1.242869 |
| Comprehensions |0.981699 |1.332579 |
| Silent logging |0.981101 |0.756278 |

Both campaigns retain stable identities and complete without failures. This
is a mixed screen, not a general speedup. Pair ranges and exact evidence are
in the [model55 report](issues/call-model55/report.md). There is no same-head
all 29 or PMU result. Latest broad model55 evidence remains ed08ab64c7e:
all 29/upstream geometric mean 1.165467, six wins; matched DeltaBlue instruction
ratio 1.368099. Do not extrapolate that measurement to newer source.

Golden54's full comparison is all 29/upstream1.156543, six wins. Its
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

These are inputs to the combined review, not selected replacements:

| Component | Actual state |
| --- | --- |
| Scalar facts | Native8232 qualifies the first GET_ANEXT/UNARY_NOT error snapshots. New VALUE-result source in Iccb96 passes 32 narrow checks and full cases emission; metadata generation awaits the reviewed empty-family fix3cfc808. Full static scratch authority is unfinished. |
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
