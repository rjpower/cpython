# Project status

Current work is [stage2 interpreter performance](issues/interpreter-performance56/plan.md):
one combined architecture for value transport, borrowing, calls/returns and
specialization. Astra completed its [combined design](design/proposals/interpreter-performance56.md)
and [audit](issues/interpreter-performance56/review.md); implementation and peer review are complete for the current composition;
root full qualification and selection are active.
The [goal](GOAL.md) and [architecture](design/runtime-architecture.md) define
the project; [HANDOFF](HANDOFF.md) owns exact execution state.

## Selection and performance

Selected54 remains golden: `61015a9452d9069646fb5fede9ff963896833b89`, branch
`pyjit/round54-golden`, tag `pyjit-golden-20261112-frame-state`. No model55
performance candidate has earned promotion.

Current integration is **ac4c7019f9b**, with logical-vacancy and qualification
fixture fixes. Both builds pass 117 modules/79 shared, and the three unchanged
admission/retirement regression methods pass per role. Broader focused verification is active. Full release at
7a21 completed55,023 tests with116 failures across13 modules; debug was
interrupted after5,310 tests to fix a confirmed consumed-slot admission and
retirement-proof bug. The shared predicate repair grants no readability or
physical NULL fact. Generated-case closure remains active; no new golden is
selected. The [handoff](HANDOFF.md) owns exact paths and executed counts.

The inline-list experiment passed focused semantics but regressed coroutines
10.7% in six pairs and was reverted. Its native call/spill reduction did not
pass the performance gate.

At measured **82e857384f8**, formats and generated products pass review, both
builds complete 117/79, all 154 focused methods pass per role, and five bootstrap
plus 27 compile probes pass. Independent cold source and conditional design
reviews found no blocker.

Six balanced golden54 pairs confirm these elapsed ratios; lower is better.

| Benchmark | /golden54 | Pair range |
| --- | ---: | ---: |
| DeltaBlue |.983547 |.966438–.995870 |
| Generators |.942484 |.925687–.961010 |
| Coroutines |.993560 |.981109–1.014690 |
| Richards |.718632 |.716043–.722528 |
| Comprehensions |.959073 |.868742–.981809 |
| Silent logging |.898868 |.860832–.936960 |
| Raytrace |1.035032 |.951701–1.059284 |

Separate direct all-29 campaigns give .989410/golden (12 wins) and
1.151069/upstream (seven wins). Raytrace and several smaller regressions remain
visible. Matched native instruction ratios against upstream are 1.361387 for
DeltaBlue, 1.411737 for generators and 1.307831 for coroutines. Extra executed
work remains the main performance gap; frame clearing is not established as
its cause. All campaigns have stable identities and no command errors.

Earlier f291's logging regression is not reproduced by 82e857 confirmation.
Actual PIC counts establish use in DeltaBlue/Richards and no selector attempts
in logging; they do not explain that earlier regression. The
[current report](issues/interpreter-performance56/report.md) owns exact-head
tables, native costs, profiles and rejected experiments. Do not multiply
separate campaign ratios or transfer results without an identity check.

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
| Scalar facts | Actual scalar normal/error graphs and current publication receipts pass the core gate within 82e857's 154 focused methods per role. Missing proof retains initialized-NULL control. Full static scratch authority remains unfinished. |
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
