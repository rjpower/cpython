# Interpreter architecture and cleanup

Runtime `0953757f4e7`, following `b58036e9718`, starts from the qualified
`7780e32e0e2` baseline. The five increments are integrated and full correctness
qualification passes. Frozen throughput collection is complete and R5's
bounded keep decision is accepted. Independent final evidence review is complete, with no remaining must-fix finding.

## Integrated design and implementation

| Commit | Change | Evidence |
| --- | --- | --- |
| `22f49d676db` | Remove the write-only call-transaction effects field and writes; preserve live handler effects | [R1 receipt](../../issues/interpreter-architecture-20261107/integration-r1.md) |
| `25ce1654801` | Outline the consuming Python-call fallback; keep overlap preparation/guards/commit on the common route | [R2 receipt](../../issues/interpreter-architecture-20261107/integration-r2.md) |
| `225480a4250` | Install the existing typed immortal empty tuple directly in the common varargs home | [R3 receipt](../../issues/interpreter-architecture-20261107/integration-r3.md) |
| `b58036e9718` | Replace five handler-name checks with one checked first-result publication contract shared by emission and metadata | [R4 receipt](../../issues/interpreter-architecture-20261107/integration-r4.md) |
| `0953757f4e7` | Prove canonical shape-home input at the writer boundary and omit redundant canonicalization at the result's first temporary store | [R5 receipt](../../issues/interpreter-architecture-20261107/integration-r5.md) |

Calls continue to use the existing frame stack and overlapping call windows.
This batch preserves fallible preparation, final revalidation, once-only owner
transfer and publication before any escaping commit step. The consuming binder
remains necessary for unsupported providers. The preceding batch already
reused known-NULL overlap slots; ordinary frame retirement already cleared
locals only. These are not new wins attributed to R1–R5.

The publication contract describes the first temporary output on success,
before receiver retirement. Generic lookup can call Python earlier. The fact
is neither a root map nor whole-handler purity. Folded locals preserve their
old home until receiver retirement begins; callbacks may mutate it. After
successful refresh, the final store replaces the actual current home,
including callback mutation. Failure preserves callback writes
and cleans up the private result. The canonical-source premise is a manually
audited writer chain plus fatal debug assertions, not static C pointer
provenance. Only the checked first temporary store bypasses publication;
folded-local final publication remains ordinary.

The generator uses an immutable checked record and a shared flush-preparation
routine. It adds no runtime publication cache, sparse roots, value bitmap,
second opcode-name table or second compiler. The initial R4 cold review caught
two unsafe declarations: a continuing generic prefix could consume an input,
and an assignment could also consume its result. Both are rejected by the
repaired checker. Initial reviews and failed logs remain evidence.

The [recovery-interface amendment](../../design/jit-e2e-system.md) passed
[independent re-review](../../issues/interpreter-architecture-20261107/recovery-re-review.md).
It defines precommit transfers, owned homes, raised versus handled exceptions,
fault versus resume PCs, completed effect prefixes and fallible preparation
before infallible installation. The native method/tracing backend, shared
transfer implementation and forced-exit recovery tests remain future work.

## Correctness and collection identity

Release, debug and stats builds match R5 and each check 117 modules: 37 builtin,
79 shared, one unavailable, none missing or failing import. GCC 15.2.0,
`-O3`, frame pointers and mimalloc remain fixed, without PGO/LTO or JIT.
Upstream `3a440b65695` and interim `eba351fee7d` remain the 116-module controls;
upstream uses computed-goto dispatch and the fork uses register/tail handlers.
The final release ELF is `9d98a70694382c4650742915f0560f12346c85bf`;
upstream is `ff51c387c04ccd5a2664e5a572bc68e491ec0db1`.

| Full profile | Tests run | Failures | Skips |
| --- | ---: | ---: | ---: |
| Release | 54,380 | 0 | 2,467 |
| Debug | 54,380 | 0 | 2,405 |

Both report 537 passing files, 79 skipped and 11 resource-denied out of 627.
Stats build/identity/smoke checks pass. The final native API lint covers 306
units with zero failing findings or parse errors. R1–R3 each passed 916 focused
tests per profile with one skip. R4's initial 1,100-test runs preceded its
checker repair; repaired generator/contract runs passed 97 per profile, with
identical generated runtime products. R5 passed 75 isolated generator tests
and 630 focused tests per release/debug profile, with one skip. Full suites
ran once at this coherent boundary.

## R5 keep/revert evidence

Six balanced paired rounds compare retained R4 and R5 on temporary/local shape
reads, five payload categories, five timing values of one million reads per
child. All 120 children check the actual specialized handler and destination
class. Source, ELF, shared imports, JIT-off state and CPU 15 are verified.
Temporary object, bigint and NaN gains repeat; local reads regress in 29/30
pairs. Integer temporary results contain a large first-child R4 outlier:
the full geometric factor is 0.926943, while its median is 0.982012 and the
labeled five-pair sensitivity is 0.982284. Keep the original data; do not
advertise a 7.3% integer speedup.

| Payload | Temporary R5/R4 factor | Folded-local R5/R4 factor |
| --- | ---: | ---: |
| Integer | 0.926943 | 1.024783 |
| Float | 0.983583 | 1.018377 |
| Object | 0.889392 | 1.025985 |
| Big integer | 0.419321 | 1.008540 |
| NaN | 0.715820 | 1.015070 |

These forms have different store/retirement work; the local form is not a
subtraction estimator. R5 removes the first conversion sequence in actual GCC
output while retaining local conversion. The shape handler shrinks from
0x45e to 0x39c bytes. The reviewed local route also executes two extra
companion-slot address calculations. That is concrete remaining work, not
proof that those instructions cause the measured local penalty.

The initial seven-kernel screen was adverse/mixed: raw R5/R4 geometric factor
1.022097, upstream drift 0.998896. The subsequent direct whole-workload check
uses four balanced R4/R5 rounds, fixed loops and five values per cell on CPU15,
sharing identical stdlib and extension files. Its declared 29-kernel paired
geometric factor is **0.992103**; per-round factors are
0.990112/0.994052/0.992346/0.991908, with 19/29 kernels faster. The secondary
ratio-of-medians factor is 0.991790. Worst per-role round spreads are 3.53%
(R4) and 6.59% (R5). The independent review supports keeping the bounded
optimization; alternating whole-suite blocks still leave temporal drift and
fixed kernel ordering. It does not establish a universal win or upstream parity.

The initial whole-workload preflight rejected an incorrect file-count check
before timing. The corrected campaign checks 79 current-ABI shared modules and
hashes all 213 on-disk DSOs, preserving legacy ABI artifacts. Its new directory
contains all accepted data; no failed prefix is pooled. See the
[protocol review](../../issues/interpreter-architecture-20261107/whole-workload-r4-r5-protocol-review.md)
and [microbenchmark review](../../issues/interpreter-architecture-20261107/paired-shape-results-review.md),
plus the [whole-workload results review](../../issues/interpreter-architecture-20261107/whole-workload-r4-r5-results-review.md).

## Final throughput

All elapsed factors below are candidate/baseline; lower is better. The
three-role campaign interleaves three rounds of all 29 kernels, with five
values per cell: 261 accepted cells and 1,305 raw timings, no rejected cells
or retries. Source/build/module probes match before and after. Maximum
pre/post one-minute load is 2.151855, below the declared four threshold.
Unlike the direct R4/R5 check, this reused collector does not pin execution.

| Declared set | Candidate/upstream | Candidate/interim | Wins vs upstream/interim |
| --- | ---: | ---: | --- |
| 29 kernels | **1.247431** | **1.044650** | 3/29; 10/29 |
| Selected 12-result pyperformance subset | **1.048074** | **0.878774** | 4/12; 8/12 |

The interpreter remains about 24.7% slower than upstream on the kernels and
4.8% slower on this selected subset. It has not reached upstream parity.
Three-round max/min-minus-one spreads exceed 10% in nine candidate, fifteen
upstream and ten interim rows; maxima are 22.95%, 28.30% and 21.58%.

The subset has eight modules, including five SciMark results. Its estimator
is the equal-weight geometric mean of twelve arithmetic-mean ratios, with
six workers and three timed values each: 648 values across 36 role/results.
All results and workers remain included. All 36 role/results have precision
warnings. Maximum recorded worker/calibration one-minute load is 2.44;
quiet-launch guards do not prove an interference-free timed interval.

| Subset result | Candidate/upstream | Candidate/interim |
| --- | ---: | ---: |
| async_tree_none | 1.441162 | 1.260453 |
| hexiom | 1.231316 | 0.890281 |
| mdp | 1.437751 | 1.173731 |
| pyflate | 1.015219 | 0.867583 |
| regex_v8 | 1.047120 | 1.056849 |
| scimark_fft | 0.570535 | 0.527192 |
| scimark_lu | 0.749890 | 0.776761 |
| scimark_monte_carlo | 0.990635 | 0.722888 |
| scimark_sor | 1.079833 | 0.767653 |
| scimark_sparse_mat_mult | 0.747627 | 0.644566 |
| typing_runtime_protocols | 1.397486 | 1.205966 |
| unpickle_pure_python | 1.354554 | 0.994229 |

Compared sequentially with qualified `7780e32e0e2`, kernel elapsed factors
are 0.971563 for the candidate, 0.983795 upstream and 0.981074 interim.
Subset factors are 0.987579, 1.022298 and 1.014120 respectively. The earlier
candidate/upstream factors were 1.263136 and 1.084920. Control drift and row
variation prevent attributing these differences to individual changes or
claiming a repeatable broad gain from R1–R5. The direct R4/R5 experiment is
separate evidence for its bounded optimization.

## Remaining machine work

All 54 selected `perf stat` invocations pass, with unchanged identities and
the declared minimum 99% event-running gate; all 108 event rows report 100%
running. Counts are normalized by hot
workload loops, with three rounds and median event counts; CPI is median
cycles divided by median instructions. They have no matched dynamic bytecode
denominator and are not per-bytecode costs.

| Workload | Instructions C/U | Cycles C/U | CPI C/U | Previous instructions C/U |
| --- | ---: | ---: | ---: | ---: |
| deltablue | 1.959790 | 1.730620 | 0.883064 | 2.001520 |
| logging_silent | 1.720317 | 1.629600 | 0.947267 | 1.774821 |
| comprehensions | 1.590360 | 1.489355 | 0.936489 | 1.605447 |

Candidate instructions per loop are 2.08%, 3.07% and 0.94% lower than the
previous candidate in these regions. This is measured machine-work reduction
for the combined batch, not isolated attribution to a source edit.

The measured distinction is extra executed native work: these regions retire
59–96% more instructions despite lower CPI than upstream. These counts support
targeting redundant machine work. They do not allocate the
whole-program gap to any single helper. Generic cache events vary sharply
between rounds and do not support a precise cache-stall explanation.

Six five-second user-cycle DWARF captures pass with zero lost samples and
matching retained ELFs. Actual sampled-IP unknown shares span 0.20–4.73%;
unresolved high addresses contribute at most 0.275%. Recovered stacks reach
`_start` in 98.09–99.53% of samples. Recovered first frames, including inline
frames, are not the actual sampled IP; the two views remain separate and no
unknown samples are filtered or renormalized away.
Native call stacks do not reconstruct the history of tail-called handlers.

Current physical-symbol self shares, weighted by sampled user-cycle periods,
identify these remaining seams:

- Deltablue: shape attribute load 11.96%, exact Python call 8.43%, type-cache
  lookup 5.88%, method attribute load 5.69% and frame clear 4.84%.
- Logging: overlap Python-call preparation 13.17%, frame clear 9.02%, method
  attribute load 8.00%, exact Python call 4.93% and shape load 4.37%.
- Comprehensions: dictionary lookup 6.56%, list iteration 6.31%, shape load
  4.80%, method-descriptor call 4.60% and exact Python call 3.72%.

These are sample shares, not removable overhead estimates. In particular,
the current `call_transaction_overlap_python` symbol must not be compared as
the same unit as the previous `call_transaction_push_python` self share.
Outlining and inlining change physical symbol boundaries.

The [follow-up plan](../../issues/interpreter-architecture-20261107/follow-up.md)
prioritizes the measured shape-read tradeoff, bounded counting of accepted and
declined call routes, and a separate first native-recovery slice. Keep the
shared ownership/phase contracts while reducing redundant guards, conversions
and state traffic one proven path at a time. Native adapter ports remain
profile-driven. Startup RSS and zero-box counts are not admission gates.

Raw evidence is retained under
`python-boil-the-ocean/build/pyjit-link-wave1/interpreter-architecture-20261107/`:
per-step checks/reviews/basic screens, exact R2–R5 ELFs, full logs and identities,
`r5-paired-shape/`, `r4-r5-whole-v2/`, and the final campaign. R1 has identity
notes but no retained exact ELF; code-generation comparisons do not invent one.
The [plan](../../issues/interpreter-architecture-20261107/plan.md) records the
integration sequence and preserved rejected findings.

`derive-results.py` reuses the strict existing analyzer against the actual
previous `7780` campaign, with its historical column label corrected in the
derived output. `summarize-profiles.py` reconciles every actual-IP record and
recovered stack with perf's sample totals and lost-sample report. Full tables
are in the [derived tables](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/interpreter-architecture-20261107/final-analysis/analysis.md)
and profile quality in [profile-summary.json](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/interpreter-architecture-20261107/final/perf-record/profile-summary.json). The independent
[evidence review](../../issues/interpreter-architecture-20261107/final-evidence-review.md)
records acceptance of every stage, the final report and the follow-up order.
