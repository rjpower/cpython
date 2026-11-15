# Project status

Current stage is [interpreter performance](issues/interpreter-performance56/plan.md).
The [comprehension workstream](issues/comprehensions57/plan.md) is complete:
select the dictionary-only cleanup and lifetime repair; reject the larger
method/list composition and its losing revisions. The [goal](GOAL.md),
[architecture](design/runtime-architecture.md) and [handoff](HANDOFF.md) own
direction, selected contracts and exact execution paths.

The active [IR-semantics workstream](issues/register-semantics59/README.md)
executes Astra's [round62 design](design/proposals/interpreter-parity62.md) and
[review ledger](issues/register-semantics59/review62.md). Selected57 remains
unchanged. PyValue rollout is complete; transport, effects, ownership and
physical execution remain targets. Parity is not achieved.

| Packet | Current result |
| --- | --- |
| Allocation-only creation62 | Both 117/79 builds and 526 focused methods per role pass. Coroutines improves 4.4% in six-pair confirmation, but all29 is 0.49% slower with 12 wins; logging, spectral_norm and deepcopy_memo lose. Rejected as a standalone packet. |
| Prepared CALL62 | Integration a3cbad0637e completes both 117/79 builds and 227 focused methods per role, including corrected synthetic cursor retirement. Native capture is retained; actual admission/execution probes and the benchmark screen are next. |
| Method-to-exact CALL62 | Root and source peer approve the one-clear contract. Implementation passes the acquired method owner directly to the shared initializer; actual admission coverage and fallback publication remain gates. |
| Delegated resume62 | Integration 17139aa0166 completes both 117/79 builds and 397 focused methods per role. Generators improves 4.3%, but DeltaBlue loses 6.5%, logging 20.3% and spectral_norm 9.3% in both screen pairs. Rejected as a standalone packet. |

Every round59–61 runtime packet remains unselected. The
[trial report](issues/register-semantics59/report.md) owns every ratio, exact
identity, failure and selection decision. The round60
[engine native review](issues/register-semantics59/engine-post-review.md)
records its stack-backed state and merged dispatch. Losing packets are not
prerequisites for independent lanes.

The corrected allocator effect model removes unjustified input staging: the
admitted allocation-only interval cannot execute Python, while real publication
and redirect cleanup still require roots. The new prepared spans publish into
proved-vacant homes without reading stale bits or requiring physical NULL.
Delegated resume preserves suspended semantic state and bypasses the backedge
under existing observation guards. No source-position equality grants a proof.

Astra reviews the architecture after coherent measured cycles. Sol workers
implement and peer-review bounded packets; root owns generation, native jobs,
merged qualification and measured selection. At a winning checkpoint compare
all29 and upstream directly and run full supported diagnostics. Repeat the
review/implementation cycle until the unchanged hot set reaches parity while
retaining broad gains. Comprehensions still needs about 30.38% of selected57's
instructions removed merely to equal upstream's measured instruction count;
small native removals do not establish that budget or a throughput result.

## Selected checkpoint and performance

Selected57: **0dcba75bb3df40c32137db453b001f7e8f101de0**, branch
`pyjit/round57-golden`, tag `pyjit-golden-20261114-dictionary-readers`.
Golden56 `883524c180e4486b47df77ee2276c9276c0ec32c` and golden54
`61015a9452d9069646fb5fede9ff963896833b89` remain immutable.

Dictionary lookup/result acquisition share one implementation below public
validation and typed `dict.get`. Arbitrary callbacks retain dictionary/query
pins; results are acquired before input cleanup. Cached exact-string shortcuts
also require a Unicode-only table under the GIL: mixed-table equality can run
Python. Both GetValue and ContainsValue use that admission rule. No method,
list, generator, frame or IR changes from the rejected composition are selected.

The direct all-29 observed means are **.995586/golden56** (16 wins) and
**1.127137/upstream** (six wins). These are two-pair aggregates, not precise or
statistically established speedups; the upstream XML iterparse pairs have
large opposing outliers. In separate six-pair confirmation, comprehensions
.997484, DeltaBlue 1.000239 and logging .992049 are near flat with mixed pairs.
Richards .991167, spectral_norm .931732 and nqueens .987729 improve in every pair.
Root explicitly accepts coroutines 1.019635 and raytrace 1.020117 losses in every
pair. Generators 1.024797 has a wide .992243–1.119177 range and remains debt too.
This is a correctness/abstraction checkpoint with a small broad benefit, not
parity. The [report](issues/comprehensions57/report.md),
[complete tables](issues/comprehensions57/benchmark-tables.md) and
[cold review](issues/comprehensions57/review.md) own evidence and selection.
Matched upstream counters still show 36.14% extra native instructions in
DeltaBlue, 43.64% in comprehensions and 30.78% in coroutines. More common-path
work remains; frame clearing alone does not explain those instruction gaps.

Both final builds complete 117 modules/79 shared extensions; all 203 methods
across three dictionary modules pass per role. The 28 focused reader methods
also pass and overlap that coverage. No generated protocol changed. Selected56
supplies the inherited coherent full-system checkpoint; no fresh full 55k run
is claimed for this two-file successor. The rejected four-lane composition ran
55,045 tests per role, retaining six baseline debugger errors and closing one
release assertion-lint finding. Only GDB PrettyPrintTests remains quarantined;
qualification is GIL-only. See the report for exact heads and limits.

The rejected composition reduced comprehension instructions 4.8% but lost
logging 13.2% and coroutines 4.6% in confirmation. Uniform handler alignment and
interpreter-local append also lost. The minimal lifetime-only repair retained
6.9% DeltaBlue and about 3% comprehension regressions in its last diagnostic despite
near-identical instruction counts. All source, native captures and results are
preserved; no further layout or padding sweep is planned.

The [selected56 report](issues/interpreter-performance56/report.md) retains
its earlier 1.53% aggregate gain over golden54 and accepted regression debt.
Bitmap, IRSC2 and full static scratch authority remain independent unfinished
gates. Native method and tracing JIT tiers remain unfinished.

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
