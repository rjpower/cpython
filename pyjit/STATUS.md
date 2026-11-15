# Project status

Current stage is [interpreter performance](issues/interpreter-performance56/plan.md).
The [comprehension workstream](issues/comprehensions57/plan.md) is complete:
select the dictionary-only cleanup and lifetime repair; reject the larger
method/list composition and its losing revisions. The [goal](GOAL.md),
[architecture](design/runtime-architecture.md) and [handoff](HANDOFF.md) own
direction, selected contracts and exact execution paths.

The active [IR-semantics workstream](issues/register-semantics59/README.md)
has closed all five round59 executable screens without a selected winner.
Final loop binding, independent builders, builder+Invoke, continuation action
encoding and isolated Invoke all lose important workloads despite focused
qualification. Isolated Invoke passes both builds and 66 methods per role,
but loses coroutines 8.1%, logging 12.6% and spectral_norm 14.2% while improving
comprehensions 3.1%. Continuation encoding passes 112 methods per role but
loses DeltaBlue 15.5%, generators 5.5% and logging 11.5%. The
[trial report](issues/register-semantics59/report.md) retains every ratio,
qualification limit, rejected branch and native mechanism. Golden is unchanged.

The preceding [round60 design](design/proposals/interpreter-parity60.md) and
[review ledger](issues/register-semantics59/review-next.md) prioritize a same-GCC
computed-goto engine, followed by actual cross-word inline-scalar lowering.
Their current dispositions follow; round61 below drives ongoing work.
Upstream's measured build already uses computed goto; selected57 uses separate
tail-called handlers with path-specific state repacking. The independent
[backend peer](issues/register-semantics59/backend-peer.md) admits that experiment
with explicit lexical-owner, error, redispatch and identity contracts. Engine
source and identity/config/preflight packets are integrated in G at ec230e55641.
Generation passes with unchanged semantic metadata. Both fresh configurations
match selected57's core/module flags and byte-identical pyconfig.h. Both builds
complete 117 modules/79 shared extensions. Release and debug each pass 465
methods across 17 ownership, call, exception, observer and suspension modules.
The balanced screen rejects the engine in all eight cases: comprehensions
+21.9%, DeltaBlue +33.9%, coroutines +18.6%, generators +43.8%, logging +60.0%,
raytrace +30.0%, spectral_norm +29.2% and nqueens +30.3%. Both pairs lose in
every case; identities remain stable. The [native post-review](issues/register-semantics59/engine-post-review.md)
resolves all 367 rows and finds a 4,472-byte stack reservation, stack-backed
engine state and one shared indirect dispatch after GCC tail merging. No
full-system or upstream rerun is claimed for this loser. Golden stays fixed;
the [scalar proposal](issues/register-semantics59/scalar-proposal.md) and its
[peer](issues/register-semantics59/scalar-peer.md) reject the allocated plan
framework and defer the fixed two-word rival. The
[common-constant producer](issues/register-semantics59/common-constant-proposal.md)
is integrated at 8bb861d357e on selected57 without a new backend, frame field or
per-site descriptor. Both builds complete 117/79 and 43 focused runtime methods
pass per role. Full generation changes only its handler and resolved ownership
metadata; the other 330 populated format rows retain their contracts. Native
handler size falls 620→356 bytes, with DUP and numeric normalization removed.
The producer-only screen is not selected: comprehensions .969376, DeltaBlue
1.036249, coroutines .979717, generators .984184, logging 1.139183, raytrace
.989650, spectral_norm 1.015950 and nqueens .995691 versus selected57.
Matched whole-process diagnostics show fewer instructions but more cycles in
DeltaBlue/logging; no exclusive cache/layout cause is established. The
[trial report](issues/register-semantics59/report.md) retains the scope and raw
evidence. TEMP source packet 8db82c67bd4 is peer-approved, generated and
integrated at fe38bad0b50. Both 117/79 builds pass. The first release focused
run fails one new scratch-owner admission test; the other five modules pass.
A constructor-only baseline proves regression: selected57 rejects the image,
candidate admits it. No malformed image was executed. The shared normal-map
consumer has a peer-approved cold diagnostic correction in implementation;
debug qualification and timing await the correction.

Astra's [round61 design](design/proposals/interpreter-parity61.md) and
[ledger](issues/register-semantics59/review61.md) are complete. After the producer,
independently admit its TEMP destination refinement, direct exact coroutine
creation in the final heap frame, then checked delegated-yield ascent. The
last two compose whole transitions; they retain real frames and owning homes
at effects. Worker source proposals and independent peers precede implementation;
root owns generation, native jobs, combined qualification and measured selection.
Ascent's actual source diff has root peer GO; creation's source implementation
is still in progress. Neither has a native performance result yet.

The [all-word census](issues/register-semantics59/resident-coverage.md) limits
the finite scalar-pair pilot, without bounding all possible regions. Its
pre-monitor label weights are not natural specialized-hit counts. The new
trials do not close a parity budget: comprehensions still needs about 30.38%
of selected57's instructions removed for equal upstream count. PyValue rollout
is complete; transport, effects, ownership and physical execution remain targets.

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
