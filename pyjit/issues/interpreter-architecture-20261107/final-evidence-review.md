# Final evidence review

2026-11-07. **Complete: accept all selected measurement stages and the final
report as diagnostic evidence, and accept the bounded R5 keep decision and
follow-up ordering. No remaining must-fix finding.** This is a read-only
audit of saved data, with short host calculations through `uv run`; no native
tests, workloads, interpreter probes, builds or
profilers were run by this reviewer. Runtime, collector and benchmark source
were unchanged by the review.

Evidence root E is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/interpreter-architecture-20261107/`.
The initial kernel review covered `E/final/kernels` and its completed
outer-command record. Subsequent stage acceptances and final-report closure
are recorded below. No acceptance includes an uncollected stage.

The kernel journal contains exactly 261 accepted cells: three rounds,
29 kernels and upstream/interim/candidate. Every attempt number is one;
there are no rejected cells, retries or reused prefixes. I reconciled every
raw report with its unique scheduled cell and retained round vector. Each
has five positive finite time values, one warmup, consistent fixed loop
counts and a reported value equal to its raw median. The total is 1,305
timed values. Commands retain the correct role, build, kernel, report path
and `--max-load 4`. The exact rotating three-role order matches the
collector's round/kernel rotation. The native build identity, hash-seed-zero
record and calling-convention record agree across all reports for each role.

The outer collector returns zero. All 261 wait records permit launch below
one-minute load three; every accepted pre/post load and both internal
one-minute samples per report are below four. The maximum pre, post and
quiet-observation one-minute load is 2.151855. Componentwise maxima of the
full pre/post tuples are `(2.151855, 3.180176, 4.023438)`. The final wait
timestamps span about 282 seconds, within the 45-minute window. The
one-minute rule does not imply every historical load component is below
four or establish an interference-free CPU. The outer journal records
`LD_BIND_NOW=1` and no `LD_PRELOAD`.

Full build snapshots are equal before/after; both errors lists are empty.
Postflight reports no timing error, no identity difference and unchanged
identity. `eligible=false` is expected because this campaign is explicitly
diagnostic, not because a validation gate failed.

| Role | Native/source revision | ELF build ID |
| --- | --- | --- |
| Upstream | `3a440b65695` | `ff51c387c04ccd5a2664e5a572bc68e491ec0db1` |
| Interim | `eba351fee7d` | `6b25bff25ab86337e0eb1284483cdd6350186b9a` |
| Candidate | `0953757f4e7` | `9d98a70694382c4650742915f0560f12346c85bf` |

All source statuses are clean. Saved configurations retain GCC 15.2.0,
release `-O3`, frame pointers and mimalloc; JIT probes report disabled and
the method-JIT API unavailable. The selected stdlib/extension probes resolve
within the intended source/build roots without import errors, with stable
extension build IDs. Module receipts remain 116/116/117 total and 78/78/79
shared, with no missing or failed imports. All raw reports identify the
same host. The canonical benchmark dirty flag is true, as in the direct
R4/R5 check; the existing results-document edit remains distinct from the
benchmark code. This kernel collector's snapshots are build/import probes,
not the all-file hash manifest used by the separate direct R4/R5 experiment.

For each role/kernel, I independently took the median of its three saved
round medians. Ratios below divide candidate elapsed by baseline elapsed;
lower is faster. The equal-weight geometric means reproduce the
coordinator's calculations. The collector's `summary.md` uses the reciprocal
baseline/candidate convention, so its rounded numbers must not be copied
as candidate/baseline factors.

| Comparison | Elapsed factor | Individual candidate wins |
| --- | ---: | ---: |
| Candidate / upstream, 29 kernels | 1.247431104469 | 3/29 |
| Candidate / interim, 29 kernels | 1.044650028241 | 10/29 |

The candidate therefore remains approximately 24.7% slower than upstream
and 4.5% slower than interim on this declared set. Its upstream wins are
nbody, float and spectral_norm. The largest upstream gaps include deltablue
1.801261, logging_silent 1.743034, comprehensions 1.487860,
unpack_sequence 1.420736 and coroutines 1.413193. A favorable aggregate
change does not establish upstream parity.

Three-round spread is maximum/minimum minus one, not a confidence interval.
More than 10% spread occurs in nine candidate, fifteen upstream and ten
interim rows. Their maxima are 22.9526% for candidate json_loads, 28.3034%
for upstream logging_format and 21.5824% for interim go. Small row changes
and narrow aggregate differences cannot be presented as statistically
established improvements from these observations.

The prior qualified runtime comparison uses the already accepted
`interpreter-throughput-20261106/final-v4/kernels` link to that campaign's
complete `final-v3/kernels`, with candidate `7780e32e0e2`. Its exact
upstream/interim build snapshots match the current control snapshots. I
recomputed current/prior raw elapsed factors from the two 29-kernel round
vector files, using the same median-then-geometric-mean estimator:

| Role | Current / prior raw elapsed factor |
| --- | ---: |
| Candidate | 0.971563161421 |
| Upstream | 0.983794812999 |
| Interim | 0.981074485334 |

The prior candidate/upstream and candidate/interim factors are
1.263135840141 and 1.054876851559. Their current/prior factor changes are
0.987566867179 and 0.990305196949 respectively. The candidate's raw
elapsed aggregate is about 2.84% lower, while the unchanged upstream and
interim controls are also about 1.62% and 1.89% lower. These are sequential
campaign sensitivities, not a correction for host drift or isolation of
the R1–R5 batch. The batch does not gain a broad causal throughput claim
from this comparison.

The [direct R4/R5 review](whole-workload-r4-r5-results-review.md) separately
supports keeping the bounded R5 optimization, with its local penalties and
earlier adverse screen retained. This noisier upstream/interim/candidate
stage neither supplies an isolated R5 estimator nor erases that tradeoff.
Subset precision, selected native counters, profile identity/loss/period
accounting and final report wording still require their own completed
evidence review. No native method/tracing backend or recovery implementation
is implied by these interpreter measurements.

Subset review, same day: **accept `E/final/pyperformance` as completed
diagnostic evidence.** The journal contains exactly 24 starts, zero-exit
finishes and validated-result records, each preceded by one quiet observation
and one guard observation. The eight-module sequence and rotating role order
match the declared collector. All commands retain six processes, three
values, one warmup, minimum time 0.1 seconds and the intended role launcher.
Each launcher resolves to the live configured interpreter with `-S` and a
private per-process bytecode cache. No failed or extra launch supplies an
aggregate input.

I read only the eight `role-*.json` files per role as result files, excluding
`pyperformance-manifest.json`. Each role has twelve unique results; SciMark
contributes five. Every result has six timed workers with three positive
values and one warmup each. All 648 timed values are retained. Calibration
records remain in the raw files and are excluded from the arithmetic means.
Native executable/revision, GCC 15.2.0, hash seed zero and pyperf 2.10.0
metadata match the expected roles. Stderr logs are empty; stdout retains the
results and warnings.

Build snapshots are identical before/after with empty error lists and match
the kernel stage after removing the subset's additional pyperf-package
probe. Measurement-source manifests are identical before/after and exactly
match the prior `7780` subset manifest: pyperf 2.10.0 and the declared
pyperformance 1.14.0 benchmark sources. Postflight records unchanged source
and identity, no stage error, and diagnostic ineligibility as expected.

Maximum saved prelaunch guard load is 2.439941; maximum recorded timed-worker
or calibration-run one-minute load is 2.44. Launch waiting and work span
about 484 seconds, within the 30-minute window. The guard applies before
launch and worker metadata samples load; these records do not establish a
continuous whole-worker load bound. All **36 of 36 results** report
instability/insufficient-samples precision warnings. No worker or result was
removed to reduce variability. Maximum all-value coefficients of variation
are about 9.47% upstream, 8.53% interim and 8.95% candidate.

For each result I independently computed the arithmetic mean of all eighteen
timed values and its coefficient of variation. Both match `aggregate.json`
exactly; this also independently checks the saved `derive-results.py`
all-worker aggregation. Equal-weight geometric means of the twelve
candidate/baseline mean ratios are **1.048073598808** against upstream and
**0.878774226324** against interim. Individual candidate wins are 4/12 and
8/12 respectively. These subset aggregates do not establish upstream parity
or significance despite being nearer one than the full kernel set.

| Subset result | Candidate / upstream | Candidate / interim |
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

I also recomputed the prior `7780` subset's means from its raw role files.
Current/prior raw elapsed geometric factors are candidate 0.987579267548,
upstream 1.022298486449 and interim 1.014120087408. The candidate aggregate
is lower while both unchanged controls are slower. These remain sequential
sensitivities with precision warnings and host variation; they neither
isolate R1–R5 nor correct the current results for control drift. Counter,
profile and final-report review remain pending at this update.

Counter review, same day: **accept `E/final/perf-stat`.** All 54 unique
invocations cover three rounds, three cases, upstream/candidate and three
two-event groups. I reconciled the exact rotated case/role/group schedule,
each result against the invocation journal, and all 108 original perf event
rows against the validated counter strings, runtime and running percentages.
Every event ran 100%, above the 99% gate. All commands returned zero, all
results are valid without errors, all control FIFOs are removed, and both
live/retained ELF identity snapshots match the accepted kernel identities
before and after collection.

The child records show setup and warmup before enable acknowledgment, the
timed hot call after enable acknowledgment and before disable, and successful
exact-byte acknowledgments. Each normalized value equals its original count
divided by the retained hot-loop count. Loop counts equal
`ceil(2e9 / fresh role/case median_ns_per_loop)` using the accepted kernel
vectors copied into this stage. Hot wall time spans 1.875–2.158 seconds and
event runtime spans 1.875–2.157 seconds. Every launch passes the quiet-load
rule, maximum saved pre/post one-minute load is 2.371582, and there are no
rejected or failed invocations.

For each event I took the median of three normalized counts per role, then
formed candidate/upstream ratios. CPI is median cycles divided by median
instructions, followed by the ratio between roles. These independently
reproduce the strict analyzer's results:

| Case | Instruction C/U | Cycle C/U | CPI C/U |
| --- | ---: | ---: | ---: |
| deltablue | 1.959790008054 | 1.730619616032 | 0.883063802203 |
| logging_silent | 1.720316738678 | 1.629599753817 | 0.947267277693 |
| comprehensions | 1.590360050181 | 1.489355334288 | 0.936489403214 |

The candidate executes substantially more native instructions per workload
loop while its CPI is lower in these captures. There is **no matched dynamic
bytecode or semantic-operation denominator**; these are not per-bytecode
costs. Branch/cache event pairs are retained and reconcile too, but they do
not by themselves identify a cause for the elapsed gap. For example, a large
cache-miss ratio in comprehensions accompanies small absolute counts and
cannot be treated as a whole-program explanation.

Against the previous `7780` campaign, candidate normalized instruction
factors are 0.979160401546, 0.969290501040 and 0.990566662340 in the same
case order, approximately 2.08%, 3.07% and 0.94% lower. These composed,
sequential measurements do not isolate a helper or the R5 optimization.
Profile identity, loss/period accounting, recovered stack quality and final
report wording remain pending at this update.

Profile review, same day: **accept all six `E/final/perf-record` captures.**
The unique case/role roster and rotating role order match the declared
schedule. All captures are valid, return zero, retain successful enable and
disable acknowledgments around the warmed hot call, remove control FIFOs
and keep stable live/retained ELF identities. Loop counts equal
`ceil(5e9 / fresh role/case median_ns_per_loop)`; hot calls span
5.025–5.331 seconds. Maximum saved pre/post one-minute load is 2.335449.
Commands retain `cycles:u`, frequency 499, user callchains and an 8,192-byte
DWARF stack request.

The coordinator retained thirty postprocessing commands: six each for stats,
self, callgraph, recovered script and explicit sampled-IP output. All
returned zero, all stderr files are empty and `DEBUGINFOD_URLS` is empty.
I did not duplicate those commands. I independently parsed the saved
sampled-IP and recovered-script files, reconciled every sample period with
the self report's approximate event count and the stats sample totals, and
recomputed the summary's unknown shares, top physical symbols and inclusive
recovered names. All six summaries match exactly. Self reports state zero
lost samples; stats and `--show-lost-events` scripts show no loss records.

| Capture | Samples | Sum of periods | Unknown actual IP, period % | Stacks reaching `_start`, sample % |
| --- | ---: | ---: | ---: | ---: |
| Upstream deltablue | 2,568 | 21,381,049,371 | 0.963093 | 99.532710 |
| Candidate deltablue | 2,517 | 20,349,344,633 | 1.772427 | 99.165673 |
| Upstream logging_silent | 2,597 | 21,556,305,958 | 0.196396 | 99.460916 |
| Candidate logging_silent | 2,512 | 20,124,150,060 | 4.733758 | 98.089172 |
| Upstream comprehensions | 2,647 | 21,532,904,226 | 1.109596 | 99.508878 |
| Candidate comprehensions | 2,676 | 21,264,783,027 | 1.431135 | 98.168909 |

All five distinct hit ELF/DSO build IDs independently match their local files
via read-only `readelf` inspection, including both main executables, both
select extensions and libc. The main IDs agree with the frozen retained
symbolization copies and the accepted kernel/subset identities. Unknown
actual-IP periods are retained in the denominator; they are not filtered or
renormalized away. High unresolved, kernel-looking addresses contribute at
most 0.274507% despite the requested user event/callchains. Their existence
does not justify relabeling the entire unknown share as kernel work.

No recovered stack is empty. `_start` coverage is 98.09–99.53% by sample
count, as the report states; its separate period-weighted coverage is
98.12–99.73%. Neither estimator should be substituted for the other.
Recovered first-frame unknown shares also differ from actual sampled-IP
shares because unwinding/inline expansion can omit or replace that first
location. Native stacks do not recover the history of tail-called handlers.
The saved callgraphs remain useful with those limits; there is no claim of
complete symbolization or complete logical Python/handler histories.

Candidate physical-symbol shares, weighted by sampled user-cycle periods,
confirm the report's selected seams: deltablue shape load 11.96%, exact
Python call 8.43% and type-cache lookup 5.88%; logging overlap-call
preparation 13.17%, frame clear 9.02% and method attribute load 8.00%;
comprehensions dictionary lookup 6.56%, list iteration 6.31% and shape load
4.80%. These are observations from one capture per case/role, not removable
overhead estimates. Current `call_transaction_overlap_python` and previous
`call_transaction_push_python` are different compilation boundaries;
outlining/inlining prevents equating their self shares or treating their
difference as saved work. Inclusive shares cannot be added to exclusive
self shares.

Final report review, same day: **accept
`pyjit/bench/results/interpreter-architecture-20261107.md` and the linked
follow-up plan.** I checked the completed source/ownership description,
functional log footers, micro/direct-R4/R5 factors, final kernel/subset
estimators, counter ratios, profile interpretations and stated limitations.
The corrected text places the final folded-local store after successful
refresh, keeps callback-written homes and failure cleanup explicit, and
distinguishes period-weighted physical-symbol shares from sample-count
stack coverage. The report retains all local penalties, the adverse basic
screen, integer outlier, noisy broad rows, precision warnings and rejected
preflight evidence. It makes no upstream-parity or isolated broad-batch
speedup claim.

Keeping R5 is supported by its reviewed canonical-home proof, targeted
temporary-result gains and the declared direct R4/R5 benefit, with the
folded-local cost visible. The broader noisy upstream/interim comparisons
do not supply a stronger isolated R5 claim. The follow-up order is sound:
first inspect the concrete shape/local emission tradeoff with the exact ELF
as control, then count accepted/declined call routes before another call
edit, and separately build the first prepared-owner/recovery slice under
reviewed transfer rules. It preserves canonical owning homes, once-only
commit, observer/error order and one selected execution design. It correctly
keeps native recovery, shared transfer implementation and the cohesive
method/tracing backend as future work. A predeclared common affinity policy
for a later broad campaign is a reasonable measurement improvement; current
noisy rows remain accepted without selective replacement.
