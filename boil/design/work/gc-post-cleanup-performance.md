# Landed GC improvements: performance and next steps

2026-10-30. Integrated in **aee46aacae2**, qualified source/native **0b15b4bbc9b**.
Independent comparison, finite-outcome and phase reviews pass. The combined
change improves coroutine throughput **1.417×** and comprehension throughput
**1.621×** against the previous qualified fork. Generator throughput is1.085×
with overlapping observed ranges; Richards is0.987×, a small regression with
disjoint observed ranges. These are combined-change results, not individual
patch attributions.

Across the matched 29 workloads, fork throughput is **0.871038× upstream**
geometrically, with 22 slower and 7 faster rows. Broad parity remains unfinished.
The previous0.8243 result is historical context, not a contemporaneous causal
comparison. Several sample ranges are wide: float50.6%, logging-silent79.4%
on the fork. Small rankings and aggregate changes deserve that limitation.

The existing invariant arithmetic diagnostics show raw elapsed ratios of about
**2.97× for int addition**, **1.51× for cached-small-int addition**, and **2.56×
for float addition**. Both interpreters execute and specialize the arithmetic;
it has not folded away. These loops assign invariant x+y expressions and do
not measure i+=1 recurrences. The results support keeping the value/register
model; they do not establish the roughly2× soft goal for general programs.

## What landed and how it was checked

Selective death processing classifies actual work once and skips absent
weakref/finalizer/hook/buffer stages. Guarded native cleanup clears eligible
suspended-generator frames before ordinary sweep, using the existing close-only
proof and fallible-preparation/irreversible-commit boundary. Shared cell, tuple
and frame ranges allow direct managed scanning while native traversal remains
an explicit adapter. Warning delivery and pre-tracking origin initialization
repairs close two observable failures without relaxing lifetime checks.

[Final landing review](reviews/gc-cleanup-final-landing.md) records the explicit
source-delta composite: three predecessor fulls of 51,255 tests each retain their
original source, while four fresh build/audit cohorts and successor origin,
affected-module, debug-stress, ASan and closure checks cover the two-store repair.
Both fresh postmerge smokes pass 1,937 tests, with 80/50 release/debug skips and
zero fallback. The declared support scope remains Linux/main interpreter/GIL
and ordinary threads. A moving nursery is not implemented here.

## Measurement method and retained data

Upstream321925 and the qualified fork use matched GCC15/O3, evaluator
-fno-gcse, frame pointers and zero initialization; JIT, PGO, LTO, BOLT, debug,
stats and tail dispatch are off. Each uses its own source, standard library
and extensions. Implementations, allocators and collection policy differ.
This comparison is not an isolated reference-counting toggle or a claim about
CPython's fastest production configuration.

The29 comparison retains 290 raw elapsed values; the same-fork four retains 40.
Each arm/row gets one fresh process, one warmup and five serial samples, with
first-arm order alternating. CPU0/sibling8 and both machine slots/build locks
are controlled; the existing max-load admission stays at4. Samples within a
process are correlated. Spread is100*(max/min-1), not a confidence interval.

Four profiles run one warmup and 50 hot calls at 32 MiB headroom. FIFO/ACK gates
199 Hz user CPU sampling and user cycles/instructions. Attribution uses actual
PID and monotonic hot bounds, one period-weighted category per sample, and
retains unknown physical leaves. All four have zero lost samples and 100% PMU
running coverage. RSS covers the whole process; counters include control
boundaries. Inclusive GC timers omit queued delivery and overlap root timers;
sampled CPU percentages are not an elapsed-time partition. The unchanged CLI
supplies four200/600-loop instruction differences; elapsed diagnostics retain
raw200-loop medians, including the empty-loop control, with no subtraction.

[Final index88bf6c7f](../../../build/gc-post-cleanup-performance-v2/finite-completion.json)
binds 550 verified artifacts. [Report data](../../../build/gc-post-cleanup-performance-v2/report-data.json)
retains all raw/normalized vectors and spreads;
[independent timing replay](../../../build/gc-post-cleanup-performance-v2/comparison-independent-review.json)
reproduces330 values. The first launcher failed CPU admission before any
workload/witness ran because its host affinity was narrowed toCPU2. Its seven
failed outputs remain archived. Removing that extra host pin left the reviewed
programs unchanged; the successful measured batch ran once,03:19:38–03:24:32UTC.

## All 29 and same-fork results

| Workload | Upstream median | Candidate median | Unit | Ratio | Upstream spread % | Candidate spread % |
|---|---:|---:|---|---:|---:|---:|
| chaos | 7.18481e+07 | 6.20263e+07 | ns per loop | 1.158348 | 38.752 | 43.904 |
| comprehensions | 17327.3 | 20463.8 | ns per loop | 0.846730 | 4.807 | 12.183 |
| coroutines | 2.36442e+07 | 3.82587e+07 | ns per loop | 0.618008 | 0.546 | 6.351 |
| deepcopy | 300173 | 442428 | ns per loop | 0.678468 | 3.223 | 16.995 |
| deepcopy_memo | 29528 | 38802.1 | ns per loop | 0.760988 | 7.355 | 19.015 |
| deepcopy_reduce | 3612.61 | 5449.94 | ns per loop | 0.662871 | 1.606 | 22.758 |
| deltablue | 3.75613e+06 | 3.75092e+06 | ns per loop | 1.001389 | 2.847 | 28.217 |
| fannkuch | 4.10043e+08 | 4.31035e+08 | ns per loop | 0.951300 | 8.771 | 3.843 |
| float | 7.69908e+07 | 3.8616e+07 | ns per loop | 1.993754 | 5.867 | 50.595 |
| generators | 2.86412e+07 | 4.74754e+07 | ns per loop | 0.603285 | 1.188 | 31.354 |
| go | 1.13779e+08 | 1.12504e+08 | ns per loop | 1.011332 | 6.038 | 25.746 |
| json_dumps | 1.16491e+07 | 1.39742e+07 | ns per loop | 0.833609 | 1.841 | 3.463 |
| json_loads | 578850 | 811866 | ns per loop | 0.712988 | 8.563 | 10.532 |
| logging_format | 71821.7 | 101674 | ns per loop | 0.706391 | 3.075 | 20.184 |
| logging_silent | 1202.63 | 1749.79 | ns per loop | 0.687299 | 15.634 | 79.372 |
| logging_simple | 65207.1 | 85911.4 | ns per loop | 0.759004 | 1.349 | 15.937 |
| nbody | 9.82674e+07 | 6.03307e+07 | ns per loop | 1.628813 | 15.604 | 0.413 |
| nqueens | 9.36768e+07 | 1.1479e+08 | ns per loop | 0.816074 | 0.989 | 12.902 |
| pickle | 263803 | 366201 | ns per loop | 0.720377 | 0.469 | 4.296 |
| raytrace | 3.26008e+08 | 2.53174e+08 | ns per loop | 1.287684 | 0.603 | 11.095 |
| regex_compile | 1.70124e+08 | 2.12813e+08 | ns per loop | 0.799408 | 5.778 | 12.098 |
| richards | 5.8109e+07 | 3.08058e+07 | ns per loop | 1.886298 | 0.491 | 0.638 |
| spectral_norm | 1.0577e+08 | 1.07528e+08 | ns per loop | 0.983655 | 1.199 | 36.081 |
| unpack_sequence | 14032.5 | 17917.9 | ns per loop | 0.783155 | 0.878 | 1.328 |
| unpickle | 345451 | 527715 | ns per loop | 0.654617 | 0.523 | 5.230 |
| xml_etree_generate | 1.23321e+08 | 1.53997e+08 | ns per loop | 0.800803 | 0.824 | 10.908 |
| xml_etree_iterparse | 1.01765e+08 | 1.60018e+08 | ns per loop | 0.635963 | 3.184 | 36.784 |
| xml_etree_parse | 1.52414e+08 | 1.74037e+08 | ns per loop | 0.875756 | 2.192 | 16.491 |
| xml_etree_process | 8.18142e+07 | 9.87461e+07 | ns per loop | 0.828530 | 35.931 | 23.229 |

| Parent/candidate4 | Parent median | Candidate median | Ratio | Observed ranges overlap |
|---|---:|---:|---:|---|
| comprehensions | 34532.1 | 21302.7 | 1.621025 | False |
| coroutines | 5.47963e+07 | 3.86788e+07 | 1.416701 | False |
| generators | 5.10065e+07 | 4.70145e+07 | 1.084909 | True |
| richards | 3.06144e+07 | 3.10113e+07 | 0.987200 | False |


The paired-four median columns use the same ns/registered-loop normalization.
Coroutine/comprehension ranges do not overlap; generator ranges do. Richards'
observed ranges do not overlap, although these serial measurements provide no
independent-process confidence estimate. The historical selective-only1.309×
coroutine gain remains at its own source and is not added to this result.

## Four new hot profiles

| Profile | Hot s | GC duration/hot % | Mark/root closure/hot % | Cycles | Instructions | Whole-process peak RSS KiB |
|---|---:|---:|---:|---:|---:|---:|
| coroutines | 8.393473 | 38.687 | 10.951 | 36019224622 | 78692060378 | 69484 |
| comprehensions | 6.525017 | 26.883 | 6.402 | 28171875916 | 64730382820 | 89228 |
| generators | 4.994999 | 21.965 | 11.420 | 21529239561 | 42985039543 | 76524 |
| richards | 3.255086 | 0.000 | 0.000 | 13811566855 | 35387999056 | 38300 |


| Exclusive sampled CPU % | Coroutines | Comprehensions | Generators | Richards |
| --- | ---: | ---: | ---: | ---: |
| Mark | 23.249 | 13.622 | 16.008 | 0.000 |
| Finalizer predicate/preparation | 0.000 | 1.703 | 0.000 | 0.000 |
| Native clear commit | 0.000 | 0.929 | 0.000 | 0.000 |
| Sweep loop/storage and other sweep work | 9.601 | 5.263 | 3.951 | 0.000 |
| Native deallocator subtree under sweep | 3.261 | 2.322 | 1.013 | 0.000 |
| Post-collect/drainer ancestry unresolved | 2.114 | 1.780 | 0.811 | 0.000 |
| Other collector work | 0.060 | 1.006 | 0.000 | 0.000 |
| Residual user CPU | 61.715 | 73.375 | 78.217 | 100.000 |

The sweep bucket includes storage return and sweep dispatch; it is not a measurement of free() alone. Residual user CPU is not synonymous with interpreter execution: unclassified allocation, lookup and unresolved leaves can be present. No table entry is a fraction of elapsed wall time.

| Reconciliation | Coroutines | Comprehensions | Generators | Richards |
| --- | ---: | ---: | ---: | ---: |
| Selected samples | 1656 | 1292 | 987 | 646 |
| Excluded PID/time samples | 0 | 0 | 0 | 0 |
| Unknown physical leaf samples | 132 | 58 | 32 | 54 |
| Lost sample records/count | 0 | 0 | 0 | 0 |

Unknown physical leaves can still have a resolved outer collector category; they must not be added to the disjoint totals. All four captures reconcile selected sample counts and period sums, with zero PERF_RECORD_LOST/LOST_SAMPLES. Comprehensions has12 bound collector-native-clear stacks; one further native-clear-symbol sample lacks sufficient bound collector ancestry and receives no commit credit. These are sampled executions, not object/admission counts. Absence of other new-symbol samples is not proof of zero executions.


[Independent phase analysis](../../../build/gc-post-cleanup-performance-v2/phase-analysis/analysis.json)
retains exact periods, DSOs, raw perf-script hashes and all reconciliation.
The old parent/upstream13 captures remain unchanged historical context.
Parent→successor instructions are85.275→78.692G coroutines,
84.954→64.730G comprehensions,44.351→42.985G generators, and essentially
35.388→35.388G Richards. Successor RSS is67.86/87.14/74.73/37.40MiB;
the old upstream contexts are16.17/19.35/28.24/16.31MiB. Memory remains a problem.

Previously, comprehension finalizer ordering/retention/predicates consumed
24.36% of sampled CPU. The new profile has 1.70% preparation and 0.93% explicit
native commit, with twelve bound collector-native-clear samples. This supports
the cleanup distinction and the paired gain. It measures sampled execution,
not an admission rate; a thirteenth ambiguous sample receives no commit credit.
The sweep bucket includes dispatch and other work as well as storage return.
Mark includes enumeration/classification. Residual CPU includes allocation,
lookup and unresolved work, rather than being pure interpreter dispatch.

Even eliminating all measured GC duration would cap illustrative speedups near
1.63× coroutines,1.37× comprehensions and1.28× generators if the other costs
held fixed. These are ceilings from profile windows, not predictions for the
29 timing processes. GC improvements alone cannot deliver a general2× result.

### What remains inside marking

The captured stacks separate these subsets of the mark category:

| Exclusive sampled CPU % | Coroutines | Comprehensions | Generators |
| --- | ---: | ---: | ---: |
| Roots plus transitive closure | 10.749 | 5.960 | 11.550 |
| Allocated-block inventory/dead classification | 12.138 | 7.276 | 4.458 |

[Exact subdivision](../../../build/gc-post-cleanup-performance-v2/phase-analysis/mark-subdivision.json)
reconciles all marking periods, including smaller ambiguous/metadata categories.
These are not old/young or live/dead age measurements. Combined inventory and
sweep is 21.74%/12.54%/8.41% respectively. A minor that still processes every
young corpse retains that component, so age separation alone is insufficient
evidence for the next large win. The
[allocator feasibility review](reviews/gc-young-space-feasibility.md) and
[allocation coverage review](reviews/gc-young-space-coverage.md) explain the
single useful young-allocation/reclamation milestone.

## Existing arithmetic and attribute diagnostics

| Micro | Candidate adjusted instr | Upstream adjusted instr | Candidate raw elapsed ns/inner | Upstream raw elapsed ns/inner |
|---|---:|---:|---:|---:|
| empty_loop | 60.771769 | 79.784575 | 4.248520 | 5.629430 |
| int_add | 45.004609 | 166.041161 | 6.174880 | 18.326295 |
| int_add_small | 44.992308 | 101.973442 | 6.565785 | 9.934650 |
| float_add | 48.002565 | 131.999975 | 6.949985 | 17.785195 |
| attr_load | 42.006980 | 71.023159 | 7.890540 | 11.734465 |
| attr_store | 51.990135 | 66.990867 | 7.692245 | 7.931995 |

Both cold images retain BINARY_OP and warmed images specialize to BINARY_OP_ADD_INT/BINARY_OP_ADD_FLOAT; candidate writes a result register, upstream STORE_FAST z. No compile-time sum folding or dead arithmetic removal is observed in these disassemblies. Native instruction attribution within the interpreter is not established by disassembly alone.

All raw vectors, per-arm minima/maxima/spreads, instruction difference vectors and unadjusted medians are in report-data.json. Four-profile disjoint phase attribution is independently decoded separately.

- All timing arms use five serial samples; descriptive ranges and 100*(max/min-1) spreads are not confidence intervals.
- Current29 upstream/candidate is a direct matched source-policy comparison, not a causal comparison against historical29 scores.
- The four profiles and hardware counters cover ACK-controlled 50-call hot windows. GNU time CPU/RSS covers the whole process including setup and warmup.
- GC duration excludes queued Python callback delivery; roots_ns includes mark/root closure. Counter phases overlap, so do not sum inclusive shares.
- The selected kernels do not deliberately install custom weakrefs/finalizers; absent samples do not establish zero invocation counts.
- Micro instruction CLI retains four normalized 200/600-loop difference values, raw pre-skeleton medians and adjusted values; individual PMU process counts are not retained by the unchanged CLI.
- Micro elapsed is raw 200-loop cost divided by 200000, no empty-loop subtraction. Invariant arithmetic is not i+=1.

The empty-loop instruction row is the raw control; the other instruction values subtract its median. All elapsed rows remain unadjusted.

## Recommendation

Keep the value/register interpreter and tracing collector. The results support
the implemented cleanup and show a material repeated whole-heap cost. Keep the
32 MiB major floor; the prior64MiB experiment increased peak RSS52–84% on the
three allocation-heavy rows for only2–7% fewer instructions.

The next collector milestone is a complete, measured young-generation path
sharing the current major/oracle, object-start map, lifecycle rules and failure
semantics. Generation and mobility are separate choices: the design must cover
the actual generator/coroutine allocations, preserve native addresses where
required, and avoid rescanning all old storage on every minor. Moving cells and
tuples alone proves relocation but does not establish coverage of those rows.
The [GC plan](gc-architecture-next.md) records the implementation decision and
its allocation, root, barrier, lifecycle and memory gates.

A trusted managed-edge shortcut is a smaller optional cut, bounded by the
precise-mark timer rather than all sampled marking. Match-once/consume-row
within polymorphic attribute handlers is a justified code cleanup; later-row
frequency and benefit remain unmeasured, while row0 is already optimized.
A coherent native value receiver/result interface remains a broader interpreter
opportunity requiring measured conversion cost and complete generic, specialized
and escaped-method coverage. Preserve orthogonal contracts instead of adding
one-off typed opcode names. Current measurements select no dispatch-backend
replacement or H1/H2 reopening.
