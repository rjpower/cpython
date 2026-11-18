# Project status

## Live state

**Active: [handlers75](issues/register-semantics59/handlers75.md), 2026-11-18.**
[Execution74](issues/register-semantics59/execution74-diagnosis.md) traced
golden73's logging regression to op-cache placement and measured golden73 at
1.1355 of upstream. Handlers75 makes handlers smaller and fewer. Round 1
(`91c9baa9600`, branch `pyjit/handlers75`) has four parts:

- Slow paths are marked cold.
- A displaced leaf object is freed at once instead of deferred.
- Handlers are compiled without frame pointers.
- `PyList_GET_VALUE` is inline.

Hot handler code shrinks 5.7% and instructions 0.5–1.3%. The all-29 geomean is
0.9973 against golden73 and 1.1236 against upstream. Its regressions
(coroutines 1.070, deepcopy_memo 1.052) coincide with collapses in op-cache
delivery. Round 2 (`f78c3902c1b`, `pyjit/handlers75-r2`, magic 64428) moves per-site
facts into word flags: a 9-bit handler id, slot-class flags that the executing
copy derives, and call-role flags. It deletes the 35 NO_SELF handlers, leaving
hot handler code at 264 KB. Its all-29 geomean is 1.0088 against golden73 and
1.1383 against upstream, and it retires 0.4–2.4% more instructions than round
1. Placement still swings single workloads. The 90% hot footprint is about
1.6 times upstream's, and 60% of it is runtime helpers rather than handlers:
call transactions, physical transfer, value-form operations that are larger
than upstream's, and frame clearing.
Both rounds are measured checkpoints; golden73 remains the selected baseline.

In progress: a cost ledger of the call/return path, ours against upstream,
with IBS precise sampling (`E59/execution76-call-ledger`). It is the causal
case the next call change requires. In parallel, `pyjit/values76` applies three
rules to every value-form operation, not only the hot ones. First, callees
borrow their arguments, as the value-slot contract states: interpreter temporaries own their values, and
`f_locals` writes keep overwritten values alive. Second, internal entry points skip public checks.
Third, inline receivers move to cold paths. Its heuristic scope
(`E59/values76-scope.txt`): about 60 functions over upstream's count that
retain their own arguments, most in `typeobject.c` and `dictobject.c`, and 43
functions with repeated tag tests. A single value-slot table, with every
slot filled at type readiness so that dispatch is one call, follows as its
own change (247 slot initializers, 224 thunk uses).

Execution-boundary73 and its bounded RETURN isolation correction are implemented,
protocol-qualified and measured. On 2026-11-16 the user selected `44f066f5fd5`
as golden73, explicitly accepting the logging regression. Sol 6.1 workers
implemented and cross-reviewed; root
integrated, generated, qualified and measured both candidates.

The complete generator/coroutine path handles yield, resume, return, real HOME
results, direct consumers, observers and callback retirement. A guarded shared
close avoids effect packaging for callback-free completed receivers. The final
correction moves generator routing out of shared RETURN bodies into one private
tail entry. Public IR and frame layout stay unchanged. The native RETURN/EMPTY
bodies shrink from boundary10's 914/1114 bytes to 509/683, below selected57's
596/780; the private route and its cost are counted separately.

Both final builds pass 117 modules/79 shared extensions. The 24-module protocol
passes 803 release and 792 debug methods, eight skips each; debug excludes 11
host parser checks. Six additional retirement methods, eight owner controls,
injected rooted recursion failure and eight native cost controls pass. Arithmetic
and exact-call controls match selected57's counts; unique generator completion
is 722 versus 769, coroutine 700 versus 721, and two-await HOME 2055 versus 2099.
Retained completion costs 11 more instructions and yield ten more. These are
reached-path costs, not throughput predictions or full-system qualification.

The final two-pair hot8 screen has 32 successful rows and stable identities.
Candidate/selected57 GM is **0.985010**. Coroutines improve 8.70%, generators 6.73%,
comprehensions 3.49%, raytrace 2.61%, spectral_norm 2.18% and DeltaBlue 0.87%;
nqueens regresses 0.40%. **Logging regresses 13.73%**, failing protection in both
pairs. Independent review recorded NO-GO under the original protection rules;
the user subsequently accepted this specific tradeoff. The sealed verdict stays
unchanged. Winner confirmation, all29, full suite and a fresh upstream comparison
have not run for this checkpoint; broader qualification remains follow-up work.

The earlier boundary10 candidate `dee619e80ab` is separately preserved, including
its ELF copies: GM 0.962227, but logging +10.44% and raytrace +1.97%. Its natural
snapshots retain all 659/597 warmed logging/raytrace specializations. Its bounded
PMU diagnosis finds logging cycles +10.47% with instructions +0.15%. Those
counters belong to boundary10, not the final return-isolated executable.
Outlining RETURN did not solve logging; neither smaller handlers nor the two
builds establish a hardware cause. Do not combine their best benchmark results.

The complete fast paths remain preserved. The
[trial report](issues/register-semantics59/report.md) owns both results,
qualification and causal limits; the [plan](issues/interpreter-performance56/plan.md)
records the remaining common call/return cycle-cost problem. The older
[diagnosis](issues/register-semantics59/execution73-diagnosis.md) concerns frozen72
and does not reopen its selection decision.

The current stage is [interpreter performance](issues/interpreter-performance56/plan.md).
The active runtime is golden73 at
`44f066f5fd55947edf709bbcf9bf17c607307e4b`, branch `pyjit/round73-golden`,
tag `pyjit-golden-20261116-generator-boundaries`, pushed to origin. Selected57
`0dcba75bb3df40c32137db453b001f7e8f101de0` and its refs remain immutable controls.

Execution72 is complete and unselected. Sol 6.1 workers implemented and
cross-reviewed the compiler/validation and runtime/generator changes; root
integrated, generated, built, tested and measured them. The separate cold-layout
unit passed 1,143 related methods per role but lost its screen. It demonstrated
no frame-packing gain and was excluded from the ABI candidate.

The carried-owner ABI is preserved at `c6d54fd3b5d` on
`pyjit/execution72-abi-integrated`. Both final builds pass 117 modules/79 shared
extensions. Across the affected integrations, 768 methods in 19 related modules
pass per role (12 skips), plus eight discarded-result completion methods. Final
coroutine qualification passes all 100 methods per role. Six complete native
paths, six owner-placement probes and an injected native-entry failure pass.
Cold review finds no further structural blocker, while retaining actual branch
coverage and full-suite limits.

The unchanged two-pair hot8 screen rejects selection: **1.067106 GM** versus
selected57. Comprehensions improves 2.7% and spectral_norm 1.2%; DeltaBlue loses
6.8%, coroutines 1.8%, generators 1.6%, logging 21.1%, raytrace 29.0% and nqueens
1.4%. All 32 rows succeed with stable identities. No confirmation, all29, full
suite, fresh upstream measurement or repair follows for this loser.

The ABI retains the 112B frame header, existing reserved homes and initialization.
It adds an 8B executable-header pointer and 2B/word map. Complete native arithmetic
and exact-call paths improve slightly; generic calls and generator completion
add work. Thus carrying a value is demonstrated, but the intended common-case
simplification is not. The [trial report](issues/register-semantics59/report.md)
owns exact results, costs and follow-up constraints; the
[design](design/proposals/execution-state72.md) and
[source contract](issues/register-semantics59/execution72-abi.md) retain the
attempted architecture. Earlier rejected units remain closed. The authorized73
review must price a complete execution boundary and preserve specialization
coverage before broader implementation.

The register interpreter and canonical eight-byte PyValues are implemented.
Reference counting with cyclic collection is selected; extensions are rebuilt
and ported. Native method and tracing JIT tiers remain unfinished. The selected
runtime uses one owning frame stack, explicit recipients and continuations,
overlapping argument homes, and an initialized frontier. Source positions do
not prove ownership or initialization.

This checkout is the canonical documentation authority. Its C source is stale;
do not build it. Use [handoff](HANDOFF.md) for runtime paths and evidence, and
[process](PROCESS.md) for contribution steps.

## Selected57 performance and qualification

The selected change unifies dictionary lookup and result acquisition below
public validation and typed `dict.get`. Arbitrary callbacks retain dictionary
and query pins; results are acquired before input cleanup. Cached exact-string
shortcuts require a Unicode-only table under the GIL because mixed-table
equality can run Python. Both `GetValue` and `ContainsValue` use that rule.

The observed all-29 means are **0.995586/golden56** (16 wins) and
**1.127137/upstream** (six wins). These are two-pair aggregates, not statistically
established speedups; upstream XML iterparse pairs have large opposing outliers.
There is no fresh upstream performance claim for the current checkpoint.

In separate six-pair confirmation versus golden56, comprehensions **0.997484**, DeltaBlue
**1.000239** and logging **0.992049** are near flat with mixed pairs. Richards
**0.991167**, spectral_norm **0.931732** and nqueens **0.987729** improve in
every pair. Root accepts coroutines **1.019635** and raytrace **1.020117** losses
in every pair. Generators **1.024797** remains debt, with a wide
**0.992243–1.119177** range. Matched upstream counters show 36.14% extra native
instructions in DeltaBlue, 43.64% in comprehensions and 30.78% in coroutines;
these counts do not identify a cause or predict throughput.

Both final selected57 builds complete 117 modules and 79 shared extensions;
all 203 methods across three dictionary modules pass per role. The 28 focused
reader methods overlap that coverage. Selected56 supplies the inherited coherent
full-system checkpoint; no fresh full 55k run is claimed for this successor.
Qualification is GIL-only. Six debugger-jump errors remain in the exact
golden54 baseline; GDB PrettyPrintTests remain quarantined. No free-threaded or
arbitrary third-party extension qualification is claimed. The
[selected57 report](issues/comprehensions57/report.md) and
[benchmark tables](issues/comprehensions57/benchmark-tables.md) retain details.

## Round67 closure

Both independent candidates qualified their mechanisms and lost their balanced
screens. A's stable 32-row screen rejects selection: hot8 geometric mean
**1.026137293**, logging **1.210653142**, DeltaBlue **1.022084868** and
spectral_norm **1.053212755**. B's anchor4 builds pass 117/79 and eight focused
methods per role; natural and promotion receipts and two fallback controls
pass. Static native review and an independent comparison pass, but those
structural results do not establish performance cause.

B's stable-identity 32-row screen rejects selection. Its geometric mean is
**1.114220219841174**, and every design67 screen predicate is false:

| Workload | Anchor4 / selected57 |
| --- | ---: |
| comprehensions | 1.0208926125877458 |
| deltablue | 1.141349549410481 |
| coroutines | 1.0279987865402465 |
| generators | 1.0325652873804605 |
| logging_silent | 1.4303733131817118 |
| raytrace | 1.0613165681229204 |
| spectral_norm | 1.149823408700587 |
| nqueens | 1.1003578304681976 |

Root closes round67 without confirmation, all29, PMU, full suite or repair.
Independent screen-peer readback confirms rejection. A and B are rejected;
selected57 remains unchanged. The full account, including earlier failures and
native evidence, is in the [trial report](issues/register-semantics59/report.md).

## Direction and evidence

The [goal](GOAL.md) and [active plan](issues/interpreter-performance56/plan.md)
define the objective and current increment. The [runtime architecture](design/runtime-architecture.md),
[register IR](design/register-ir.md), [interpreter](design/interpreter.md),
[calls and frames](design/call-and-frame-lifetimes.md), and
[activation transfers](design/activation-transfers.md) describe selected
contracts. The [roadmap](ROADMAP.md) records stage outcomes.

The [selected56 report](issues/interpreter-performance56/report.md),
[model55 ledger](issues/call-model55/implementation.md), and linked trial
reports retain historical experiments and unfinished independent gates. They
are evidence, not an automatic dependency list for round68.
Bitmap execution readers, IRSC2 exceptional transfers and full static scratch
authority (M4) remain unfinished independent gates, outside selected57.
