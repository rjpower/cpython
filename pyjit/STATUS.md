# Project status

Current work is [stage2 interpreter performance](issues/interpreter-performance56/plan.md):
one combined architecture for value transport, borrowing, calls/returns and
specialization. Astra completed its [combined design](design/proposals/interpreter-performance56.md)
and [audit](issues/interpreter-performance56/review.md); implementation and peer review are complete for the current composition;
root qualification, measurement and incremental selection are complete.
The [goal](GOAL.md) and [architecture](design/runtime-architecture.md) define
the project; [HANDOFF](HANDOFF.md) owns exact execution state.

## Selection and performance

Selected56: **883524c180e4486b47df77ee2276c9276c0ec32c**, branch
`pyjit/round56-golden`, tag `pyjit-golden-20261113-interpreter-performance`.
Golden54 remains immutable at `61015a9452d9069646fb5fede9ff963896833b89`.

Direct all-29 comparisons give **.984655/golden54** (17 wins) and
**1.141077/upstream** (seven wins). In six-pair confirmation, all six focus
workloads improve in every pair: Richards .712389, silent logging .904529,
generators .951182, DeltaBlue .981856, comprehensions .983860, coroutines .990377.
Raytrace regresses 4.50% in every pair. Broad spectral_norm/nqueens regress
about 3.5%, XML iterparse 2.7% and chaos 2.5%. Root accepts these unresolved
losses for an incremental landing, explicitly overriding the audit's suggested
>2% promotion tripwire, with independent selection review. This is not parity.

Matched upstream native instruction ratios are 1.361430 for DeltaBlue,
1.411083 for generators and 1.307831 for coroutines. More common-path work
remains; historical generator-copy zeroing is identified but cannot explain
the entire gap. [Report](issues/interpreter-performance56/report.md) and
[complete tables](issues/interpreter-performance56/benchmark-tables.md) own
ratios, ranges, diagnostic profiles and selection rationale.

Coherent full release/debug at cb3 each execute 55,023 tests with no crash.
Release has only six baseline debugger-jump errors; debug additionally found
one stale LLTrace assertion. Final883 changes only that fixture, with identical
runtime sources: all three debug trace methods pass; release skips them. Final
verbose sys_settrace checks reproduce the same six baseline errors per role.
Both builds complete 117 modules/79 shared; regeneration, five bootstrap images
and 27 compile probes pass. No green full-suite claim is made. GIL qualification
only; existing GDB PrettyPrintTests remains the sole quarantine.

The inline-list experiment is rejected/reverted after a 10.7% coroutine
regression despite removing its native call/spills. The initial COPY-web census
found zero eligible surviving candidates; no speculative pass was added.

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

## Model55 component dispositions

The scalar subset is selected; bitmap and exceptional-transfer branches remain
experimental components:

| Component | Actual state |
| --- | --- |
| Scalar facts | Actual scalar normal/error graphs and current publication receipts pass the core gate within selected56; logical-vacancy consumers are repaired and full diagnostics complete. Missing proof retains initialized-NULL control. Full static scratch authority remains unfinished. |
| Bitmap execution readers | B0e918 builds 117/79 in both roles. All34 focused cases pass release. Debug aborts in a constructed HomeRead image at LOAD_COMMON_CONSTANT; its other four selected modules pass. Earlier release lifecycle diagnostics pass seven modules but fail monitoring/profiling with absent-input errors. No complete bitmap timing or qualification. |
| Explicit exceptional transfers | Compiler071d, readers4c646 and runtime/container9c297 are source-reviewed with Clinic/AST checks only. IRSC2 names retained homes and protocol destinations explicitly. Full generation and native qualification remain pending; the allocator is still canonical. |

The [model55 ledger](issues/call-model55/implementation.md),
[report](issues/call-model55/report.md) and frozen
[design](design/proposals/register-call-model55.md) retain component history.
The active plan decides which pieces are needed by each performance increment;
completing all old experiments is not an automatic gate for unrelated gains.

## Qualification limits

The selected56 coherent full runs close the earlier admission, retirement,
generated-source and fixture failures. Six debugger-jump errors remain per role,
all present in golden54's exact baseline. The final LLTrace-only repair has
focused verification over the byte-identical runtime; the full suite was not
repeated for that fixture. Only existing GDB PrettyPrintTests is quarantined.
No free-threaded qualification or arbitrary third-party extension conformance
is claimed. Bitmap/IRSC2/M4 remain independent unfinished gates.

Canonical documentation is this checkout. Its C source is stale; do not build
it. Preserve immutable controls and evidence; use the handoff before building
or measuring a runtime.
