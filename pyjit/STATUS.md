# Project status

## Live state

The current stage is [interpreter performance](issues/interpreter-performance56/plan.md).
The active runtime is selected57 at
`0dcba75bb3df40c32137db453b001f7e8f101de0`, branch `pyjit/round57-golden`,
tag `pyjit-golden-20261114-dictionary-readers`. No round67 or round68 candidate replaced
it.

The grouped [round68 proposal](design/proposals/interpreter-parity68.md) is
implemented, qualified and rejected by its protected hot8 screen. Current G is
`c02cebf80c48d47a9167dd86dce9bdc0fd11a623`. Independent source, generated-product
and native/control reviews pass. Final15 release/debug builds each pass 117/79,
and all 50 focused methods pass per role (one debug-only skip in release).
Related integration passes 13 modules per role plus the full remote/delegated
pair's 84 methods per role. The six inherited debugger-jump errors remain exact
in both roles; no fresh full-system pass is claimed.

Native controls establish zero intermediate delegation handlers at depths2/8/32,
direct VM SEND/FOR entry, all six direct-backend NULL fallback cases, numeric
native recursion/error cleanup, and the actual unchanged Tree's retained count1
own-yield/resume/right-append cycle. Real RUNNING remote reads/refusal and
parked-owner profile changes pass. Suspended remote coverage combines source,
synthetic reader cases and actual Tree suspended images; no separate suspended
remote-process sample is claimed.

The final native functions match the reviewed bodies. Main release .text grows
13,120 B, native generator-entry stack grows, and every generator subtype pays
an 8 B membership pointer. Those costs are charged; they predict no speedup.
The stable 32-row screen has GM 0.989788, but DeltaBlue 1.101089,
coroutines 1.072083 and logging 1.239007 fail protection. Generators 0.708007 and
spectral_norm 0.892976 improve; this does not offset protected losses. Root stops
without confirmation, all29, PMU, full suite or timing rescue. There is no fresh
upstream result or parity claim. The next grouped review69 is running in the
[Astra session](http://127.0.0.1:7878/s/zfr7ow1e) from this complete evidence;
no new implementation lane is approved yet. The [trial report](issues/register-semantics59/report.md)
owns evidence and limits; [handoff](HANDOFF.md) owns execution logistics.
Selected57 remains unchanged.

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
