# Final throughput evidence review

2026-11-06–07. **COMPLETE: all selected measurement stages and the final report
are ACCEPTED as diagnostic evidence, with the limits below. B has no admitted
standalone throughput benefit.** This is an
independent read-only audit of the frozen A/B/C composition at runtime
`7780e32e0e2a03f829028128a0b07e90ed97659b`. The reviewer runs no tests,
builds, benchmarks, profilers or runtime probes and does not alter raw evidence.

Evidence root E is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/interpreter-throughput-20261106`.
The selected campaign root is **`E/final-v4`**. Its `kernels` symlink resolves
to the accepted complete `E/final-v3/kernels` stage; `campaign.json` explicitly
records that reuse and its source identity. The fresh subset is exclusively
`E/final-v4/pyperformance`. Earlier `final-kernels`, `final-v2` and the failed
`final-v3/pyperformance` partial subset remain excluded. Completed functional
validation is recorded separately; it does not establish a performance improvement.

## Completed kernel collection audit

The completed atomic journal contains **261 accepted cells and one retained
environmental rejection**, at round three upstream comprehensions. The rejection
has exit one, empty stdout, no report at rejection, and the exact normal progress
line followed by the load guard. Its full post-load is `4.4775390625 /
3.98095703125 / 5.4609375`. The next attempt is that same cell's attempt two;
it succeeds and supplies the sole accepted value. The rejected progress value
26,763.7 ns/loop is excluded; its accepted replacement reports 18,421.5 ns/loop.
No speed comparison was used to select that replacement. The accepted protocol is in
[measurement-retry-source-review.md](measurement-retry-source-review.md).

Independent checks reconcile all 261 raw reports with all 261 saved round-vector
cells, their role/source identities and the exact three-round rotation. Each
raw report contains five positive timing values, and its reported value equals
their median. Every scheduled role/kernel/round has exactly one accepted row;
the previous accepted prefix remains present after rejection. All command
vectors retain `--max-load 4`, the exact role/kernel/build/report arguments and
the new campaign output directory. No older attempt is an aggregate input.

All 354 saved quiet-wait observations retain full load tuples; the final wait
sample before each invocation is below three. Accepted pre-load reaches
2.98583984375 and accepted post-load reaches 3.482421875. The saved first and
last invocation wait timestamps span about 1,207 seconds, within the declared
45-minute window. This does not remove environmental censoring or host drift.

Kernel preflight has no errors and clean source status for all roles:

| Role | Source HEAD | Executable build ID |
| --- | --- | --- |
| Upstream | `3a440b65695615e9fd9912342b79cdd54fced0d2` | `ff51c387c04ccd5a2664e5a572bc68e491ec0db1` |
| Interim | `eba351fee7dacef3f7616e4c0b6fc21c7082eb5d` | `6b25bff25ab86337e0eb1284483cdd6350186b9a` |
| Candidate | `7780e32e0e2a03f829028128a0b07e90ed97659b` | `8c621301ec1ca453bca4c2405f64a80fa187e571` |

Full before/after build snapshots are equal, both errors lists are empty,
all source statuses are clean, and final kernel status has phase `postflight`,
no timing error, no identity differences and `identity_unchanged=true`.
Module receipts remain 116/116/117 total and 78/78/79 shared, with no missing
or failed imports. Eligibility is false as expected for diagnostic collection.
Hardware stat and six recordings are accepted below. The first
`final-v3/pyperformance` attempt stopped before native invocation nine on a
prelaunch guard sample `4.37158203125 / 3.89697265625 / 4.5498046875`.
Its postflight records unchanged build and measurement-source identities,
empty errors and no identity differences. Its partial results are excluded;
only a separately declared complete fresh subset may supply final aggregates.
This does not invalidate the completed independent kernel stage.

## Completed fresh pyperformance audit

**ACCEPT diagnostic collection** at `E/final-v4/pyperformance`. All 24 native
launches have a zero-exit finish and validated-result record, in the declared
eight-module/rotating-role order. There are 24 guard observations and 28 saved
quiet-wait observations. Commands retain six processes, three values, one
warmup and minimum time 0.1 seconds. Each role has eight module files and twelve
unique results; SciMark supplies five. Every result has six timed workers with
three positive values and one warmup each, yielding eighteen timed values.

The raw metadata identifies the expected configured live executable for each
role, revision, GCC compiler, hash seed zero and pyperf 2.10.0. Launchers retain
`-S`, per-process bytecode caches and the intended live build interpreter.
Build snapshots are byte-identical before/after, have no errors and match the
accepted kernel stage's source/build IDs. Measurement-source manifests are
identical before/after and also match the historical 6a subset's manifest.
Status records no stage error, unchanged identities and sources, and diagnostic
ineligibility as expected.

The independent arithmetic mean of every raw eighteen-value vector matches
the wrapper-generated `aggregate.json`. Equal-weight geomeans of ratios of
these per-result arithmetic means are:

| Comparison | Elapsed factor |
| --- | ---: |
| Candidate / upstream, twelve results | 1.084919549201 |
| Candidate / interim, twelve results | 0.902390951791 |

| Result | Candidate / upstream | Candidate / interim |
| --- | ---: | ---: |
| typing_runtime_protocols | 1.521286322 | 1.257648146 |
| mdp | 1.466678453 | 1.182017210 |
| async_tree_none | 1.428943995 | 1.239406075 |
| unpickle_pure_python | 1.364986205 | 1.030264710 |
| hexiom | 1.270080447 | 0.977439863 |
| pyflate | 1.122406074 | 0.894043039 |
| scimark_sor | 1.095678435 | 0.784705401 |
| regex_v8 | 1.060772013 | 1.073294556 |
| scimark_monte_carlo | 1.012220914 | 0.698676654 |
| scimark_lu | 0.889404194 | 0.916340927 |
| scimark_sparse_mat_mult | 0.723085206 | 0.659125260 |
| scimark_fft | 0.566540180 | 0.494556680 |

**35 of 36 results emit the insufficient-samples precision warning**; candidate
scimark_monte_carlo is the exception. This is a diagnostic subset, with no
significance claim. Waiting and work finish in about 515 seconds, within the
30-minute stage budget. Prelaunch guard load reaches only 2.81591796875, but
worker metadata records load up to **4.54**. The guard is prelaunch-only; it
does not establish an under-four load bound throughout each worker. These
workers remain included without selective removal. Quiet-wait observations
reach 4.33740234375 before launch is permitted.

Raw current/6a elapsed geomeans are candidate 0.986533804484, upstream
0.997006724451 and interim 0.998201280408. The change in candidate/upstream
factor is 0.989495637581. These remain sequential sensitivity comparisons;
warnings, host variation and the composed changes prevent attribution to B.

## Independently recomputed kernel results

Ratios below are candidate elapsed divided by baseline elapsed; lower is better.
The equal-weight geomean uses each role's median of its three round medians.
The reviewer recomputed directly from retained JSON with JavaScript arithmetic,
independently of the results analyzer:

| Comparison | Elapsed factor |
| --- | ---: |
| Candidate / upstream, 29 kernels | 1.263135840141 |
| Candidate / interim, 29 kernels | 1.054876851559 |

The largest upstream gaps and all four candidate wins are:

| Kernel | Candidate / upstream | Candidate / interim |
| --- | ---: | ---: |
| deltablue | 1.800073884 | 1.345952671 |
| logging_silent | 1.676252514 | 1.425470507 |
| comprehensions | 1.547040888 | 1.185884715 |
| generators | 1.508093101 | 1.040091158 |
| xml_etree_process | 1.505687721 | 1.276509576 |
| float | 0.808977747 | 0.657319421 |
| nbody | 0.850820195 | 0.684526941 |
| chaos | 0.977819077 | 0.824947424 |
| spectral_norm | 0.983849248 | 1.118310779 |

Round spread is `max/min - 1`, not a confidence interval. Candidate spread
reaches 29.8657% for spectral_norm, 22.9131% for nbody and 22.5430% for
coroutines. Upstream logging spread is 28.7563%; interim xml_etree_iterparse
is 24.5883%. These observations preclude treating small differences as proven
repeatable improvements.

Against the older 6a campaign, raw candidate elapsed geomean is 0.994506651315,
while upstream is 1.013115248262 and interim 1.013789314480. The change in
candidate/upstream ratio is 0.981632299998. This is a sequential-campaign
sensitivity comparison, not a correction for host drift or isolation of A/B/C.

## Completed hardware-counter audit

**ACCEPT** `E/final-v4/perf-stat`: all 54 unique cells cover three rounds,
three cases, two roles and three grouped event pairs. The journal follows the
declared rotating case/role/group order exactly. All invocations are valid,
exit zero, contain no errors and remove their control FIFOs. Each setup/warmup
precedes enable acknowledgment; disable acknowledgment follows the hot region.
Both acknowledgments match the accepted exact bytes. Commands execute the
live configured interpreter, not the retained symbolization copy.

All **108 original raw event rows** independently reconcile their counter
strings, event runtime and running percentage with the saved validation rows.
Dividing each raw count by its saved loops reproduces every per-loop value.
Loops equal `ceil(2e9 / fresh role/case median)`, and the saved median matches
the accepted kernel stage. Every event ran **100%**, above the 99% threshold.
Enabled runtime and hot wall time span 1.860–2.234 seconds,
with process CPU/wall at least 0.99815. Acknowledgment latencies span
53.71–105.161 microseconds. Saved pre/post load reaches 2.5361328125.

Before/after executable identity receipts are equal and match the kernel and
subset stages. Retained ELF notes identify candidate
`8c621301ec1ca453bca4c2405f64a80fa187e571` and upstream
`ff51c387c04ccd5a2664e5a572bc68e491ec0db1`, matching the execution receipts.
Status is diagnostic-complete with all 54 scheduled cells, no failed
invocations, no fatal/identity error and unchanged identity.

Ratios below divide candidate medians of three per-loop values by upstream
medians; CPI is the cycles ratio divided by the instruction ratio:

| Case | Instructions C/U | Cycles C/U | CPI C/U | Candidate instructions / old 6a candidate |
| --- | ---: | ---: | ---: | ---: |
| deltablue | 2.001519802 | 1.902425330 | 0.950490386 | 0.977911994 |
| logging_silent | 1.774820702 | 1.865215719 | 1.050931915 | 0.932549469 |
| comprehensions | 1.605447350 | 1.480662903 | 0.922274345 | 0.985576036 |

Instruction max/min spread is at most 0.0310% in any role/case; cycle spread
reaches 2.2488% for upstream logging. These targets still execute materially
more native instructions per workload loop than upstream. Reductions relative
to 6a are composition observations, not per-bytecode costs or B's isolated
contribution. Old/current upstream instruction factors are
0.999940436 / 1.000000045 / 0.999520323, respectively.

Generic cache counters remain unsuitable for causal claims: upstream logging
cache-reference max/min is 18.62-fold, despite stable retired instructions.
The median cache-reference C/U factor of 53.86 must not be sold as a cache
mechanism. Upstream comprehensions cache-miss max/min exceeds twofold too.

## Completed recording and symbolization audit

**ACCEPT** all six role/case recordings at `E/final-v4/perf-record`.
Each exits zero with valid control acknowledgments, fixed loops from the fresh
median targeting five seconds, and no recorded error. Commands preserve
`cycles:u`, frequency 499, `--user-callchains`, DWARF stack size 8192 and delayed
FIFO enable. Hot wall time is 4.858–5.417 seconds; saved pre/post load reaches
2.40478515625. Before/after executable identity receipts are equal.

The coordinator's initial 24 offline report/script commands all exit zero with
empty analysis stderr and `DEBUGINFOD_URLS=''`. Capture stderr contains normal
enabled/disabled and capture-size messages. All reports say zero lost samples.
An independent awk scan of every retained script reconciles the exact sample
count and summed header period with report totals and finds zero lost records:

| Role / case | Samples | Sum of sampled cycle periods | Actual sampled IP unresolved | First recovered frame unresolved | Samples reaching `_start` |
| --- | ---: | ---: | ---: | ---: | ---: |
| Candidate comprehensions | 2,431 | 20,692,195,170 | 2.31765% | 2.19579% | 2,389 |
| Candidate deltablue | 2,673 | 22,026,125,837 | 1.88160% | 1.69368% | 2,653 |
| Candidate logging | 2,703 | 22,048,141,537 | 3.91720% | 3.73066% | 2,655 |
| Upstream comprehensions | 2,629 | 21,503,593,676 | 1.52841% | 1.52841% | 2,608 |
| Upstream deltablue | 2,452 | 20,964,078,830 | 1.18935% | 1.02541% | 2,432 |
| Upstream logging | 2,438 | 19,795,588,123 | 0% | 0% | 2,422 |

Both unresolved-share columns sum event periods, not sample counts. Six added
`perf script --hide-call-graph` extracts expose actual event IPs independently
of the recovered user chain. A separate awk scan reproduces every count,
period total and actual-IP unresolved total, with no unparsed nonblank rows.
The preliminary unsupported `--no-call-graph` attempt exits 129 and is retained
as an offline-command failure; no recording is replaced or excluded because of
that spelling correction. The six supported extracts exit zero with empty stderr.

Only one candidate comprehensions sample has fewer than two distinct recovered
PCs; other captures have none. Reaching `_start` is a limited unwind check,
not proof of complete logical Python stacks. Earlier handler history is lost
to tail dispatch. Inline names describe compiled ancestry rather than extra
physical frames; inclusive names overlap and cannot be summed.

All hit main/libc/select identities have matching retained modules. Direct
read-only ELF-note inspection reproduces the five manifest IDs, including libc
`066527e430a32768d82741e00b81eebb1a872294`, candidate select
`9251519adfa7fa73526d5f2c69610479c1167c1f` and upstream select
`08964a4dcc555b09a9778f5f2ea6fa982b68426c`. The libc identity matches the
earlier hardware evidence; bounded disassembly there remains applicable.
The earlier SIGPROF bucket remains opaque because it lacked that identity.

Physical self reports and first-recovered-frame scripts use different symbol
groupings. Actual-IP extracts retain a few unresolved kernel-looking high PCs
despite the requested user-only scope; recovered-user-chain output does not
expose those as first frames. Candidate counts are three/five/five for
comprehensions/deltablue/logging, with weighted shares 0.121858% / 0.187924% /
0.186544%. Upstream deltablue has four, totaling 0.163942%; the other upstream
cases have none. Preserve this limitation rather than claim every original
sampled IP is a symbolized user-space leaf. One recording per case, adaptive
unequal periods and sampling skid limit site precision.

Candidate logging's physical self rows include `call_transaction_push_python`
16.18%, `_PyFrame_ClearExceptCode` 9.38% and method attribute loading 6.32%.
These bodies include inlined work; those shares do not prove prologue cost.
The first recovered libc/frame-initialization bucket instead totals
822,540,378 / 22,048,141,537 = **3.730656%**, entirely through exact overlapping
calls in these recovered samples. The physical libc PC `0x19842d` alone is
2.75%, so it must not stand for that whole bucket. The old hardware bucket
was 7.22%; the share change is descriptive composition evidence, not B's
causal speedup. Its old exported-neighbor label remains insufficient to name
the libc routine. Different samples or symbol groupings must not be added.

## Final report and analyzer review

The report's distinction between static code removal, retired instructions and
elapsed improvement is appropriate. Its integration-screen ratios agree with
the earlier independent admission review. It correctly distinguishes owning
homes from future-read liveness and corrects the old blanket-clear description.
Its initial pending campaign prose and `final-v2` evidence path have been
replaced with the complete selected `final-v4` and explicit accepted-kernel reuse.

The first analyzer draft was returned for these source fixes:

- Compare the report's short `build.git` identity with the preflight interpreter
  probe identity, separately preserving full source HEAD checks. Direct equality
  between short report SHA and full source SHA rejects valid real reports.
- Select explicit completed stage directories. Recursive uniqueness searches
  for generic `round-medians.json` and preflight names collide with recordings,
  stat, failed attempts and other stages in the real baseline/campaign layout.
- Reconcile each raw kernel report value with the corresponding saved
  round-vector entry before computing from that vector.
- Read eight module files per pyperformance role, including all five results
  within `scimark.json`. The first draft instead expected twelve single-result
  files and common-only names; the real scimark names are benchmark metadata.
  Merge metadata or use the official loader and verify twelve unique results.

The revised actual source fixes all four issues. Candidate/baseline elapsed
ratio direction, ratio of round medians, equal-weight geomean of per-result
arithmetic means, and CPI arithmetic are accepted for the declared layout.
The author reports the archived self-check reproduces kernel C/U
1.286770861292, subset C/U 1.096436919978 and instruction ratios
2.046606029 / 1.903192100 / 1.628161800, with self-sensitivity 1.0.
This reviewer inspected the revised source and actual archive shapes; it did
not execute the analyzer. Its stat matrix/normalized-value checks do not replace
independent raw-row, collection-journal and build-identity checks. Current
`E/final-analysis/analysis.json` and `.md` now agree with the independent
raw-derived kernel, subset and stat calculations in this receipt (within ordinary
floating rounding). All 29 kernel and twelve subset ratio rows reconcile.
This accepts the derived arithmetic, not attribution or a precision claim.

Final claim review **ACCEPTS**
`pyjit/bench/results/interpreter-throughput-20261106.md`. All 29 kernel and
twelve subset table rows reproduce the retained analysis to six decimals, as
do the three hardware rows. Independent checks reproduce four upstream wins,
ten interim wins, and over-10% spread counts of fourteen candidate, thirteen
upstream and eleven interim kernels. Full-suite log summaries match the
reported 54,361 tests, skip counts, durations and 537 passing files for each
profile. Build IDs/flags, failed-attempt exclusions and selected campaign
paths are consistent with the retained receipts.

The report correctly distinguishes physical self symbols, recovered-frame
libc ancestry and actual-IP unresolved shares. Its 3.73% libc bucket represents
the whole recorded frame-initialization bucket, not just the 2.75% single-PC
row. It treats the call-wrapper annotation as prioritization evidence, not an
isolated prologue cost. No unsupported broad elapsed win, per-bytecode cost,
boxing cause, complete-symbolization claim or isolated B attribution was found.

## B admission recommendation

The source remained frozen through this campaign. B's isolated seven-kernel screen
did not establish a repeatable elapsed benefit: raw B/A geomean was 1.003425,
with substantial between-campaign control movement. Conditional tail clearing
is present in retained GCC output, but bytes skipped are not a timing result.

**Do not admit B as a measured performance improvement from this composition.**
A final C-versus-6a result cannot isolate B from A and C. Even a favorable
combined result does not cure that attribution gap. Unless additional existing
evidence specifically supports B, prefer removing only its clear-elision bounds
arithmetic from the subsequently selected runtime, retaining the independent
declined-provider assertion correction and ownership comment. Preserve B's
reviewed artifact and receipts. This is a maintenance/admission recommendation,
not proof that B caused a regression. Root owns any source selection and its
required validation after the frozen campaign; no midcampaign change is advised.

Keeping B for its bounded shared primitive is an available coordinator judgment,
but should explicitly record **unproven throughput benefit**, rather than claim
admission by the combined ratio. C's reuse of an existing postcondition has a
different maintenance tradeoff and adds no analogous bounds path.

The final coordinator report chooses that retention judgment and records this
reviewer's preference to drop the clear-elision arithmetic absent specific
benefit evidence. That disagreement remains. The source's known-NULL proof and
diagnostic collection are accepted; neither this receipt nor final report
acceptance converts B into an independently measured throughput win. No further
extension of that optimization is justified by this composed campaign alone.

## Retained offline profile audit method

The coordinator ran these after collection, using the six retained `*-perf.data`
paths as DATA and `DEBUGINFOD_URLS=''` to prevent fetching. Retained argv,
return code and separate stdout/stderr identify each command. This reviewer
read their outputs and did not rerun perf:

```text
perf buildid-list -i DATA --with-hits
perf report --stdio --stats --no-children -g none --sort dso,symbol --show-nr-samples --percent-limit 0 -i DATA
perf report --stdio --no-children -g none --sort dso,symbol --show-nr-samples --percent-limit 0 -i DATA
perf report --stdio --children -g graph,0.5,caller --sort symbol --percent-limit 0.5 -i DATA
perf script --show-lost-events -F comm,pid,tid,time,event,ip,sym,dso,period -i DATA
perf script --hide-call-graph -F comm,pid,tid,time,event,ip,sym,dso,period -i DATA
readelf -n RETAINED_MODULE
```

On the installed perf, the `--stats` report is statistics-only; retain a separate
report without that option for symbol rows. Reconcile script event count with
record/report counts, inspect lost-event records and all command stderr, and
sum sampled periods separately from counting samples. Verify every hit module
against a retained matching build ID, including libc/select and each main ELF.
Do not symbolize against whichever live path happens to exist afterward.

Quantify unknown leaf weighted-period share and whether its mapped module is
known; inspect distinct recovered PCs and stacks reaching `_start` as limited
unwind checks. Tail dispatch prevents recovery of earlier handler history;
inline names are compile-time ancestry, not extra physical callers. Inclusive
shares overlap and must not be summed. One recording per case limits stability.
The old SIGPROF libc bucket remains opaque because its identity was not retained.
Any new unresolved libc symbol needs bounded ELF evidence and actual recorded
callers before attribution; a nearest exported objdump label is insufficient.

## Final scope

All selected kernel, fresh subset, hardware-stat and recording audits are
complete above, including derived-analysis agreement and final report claim
review. No remaining measurement-review blocker was found. The reviewer used
read-only JSON/text arithmetic, awk scans and ELF-note inspection; it ran no
benchmark, test suite, build, perf command or new capture and changed no runtime
or raw evidence. Retired instructions are per workload loop, not per Python
bytecode; neither reduced
counts nor stack-share movement alone isolates one patch's causal contribution.
