# Final source and qualification review

2026-11-08. Runtime source reviewed:
`4b3d739d867b06ea4c9ed936982cfeb3503e93be`, branch
`pyjit/execution-boundaries-20261108`. The integration worktree was clean.
Evidence directory, abbreviated E10 below:
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/execution-boundaries-20261108`.

**Source disposition: approve. Qualification disposition: sufficient to proceed
to measured admission, with the unexplained debug profiling failure retained
as a limitation. Performance admission remains open.** No new source blocker
was found. A successful retry is not a demonstrated repair of the earlier
profiling failure, and this review makes no speedup claim.

This is read-only source/evidence review. No native probes, builds or tests
were launched by this reviewer. The only write is this review. It builds on
the [cold source review](cold-integrated-review.md), including its independent
review of fixture repairs, and rechecks the final source and retained logs.

## Source assessment

The integrated operand, branch-boundary and call-completion implementation
matches the reviewed protocols. Exact and general Python entry produce the
same tagged completion recipient; ordinary setter users reject that tag;
completion decodes it before publication or re-entrant disposal. Instrumented
completion materializes the original temporary, and a successful debugger
line change cancels pending direct disposal. Suspended callers/callees and
free-threaded selection remain excluded. These are scoped implementation
limits, not Python language restrictions.

Boolean continuation variants require a canonical/live NOP and successors
that save IP. Their type guard executes before continuation advances, and
instrumentation restores dependent predecessors before patching the target
anchors. Debug execution declines the shared skip variants because lltrace
is frame-specific. Canonical code and observer anchors remain present; this
packet does not implement global logical-position remapping or physical
canonical-word compaction.

The final source includes the independently reviewed fresh-code fixtures,
physical-handler encoding and shadow-aware contract checks. The new result
finalizer test makes its selected observation policy explicit: direct disposal
belongs to CALL, and instruction observation enabled inside that disposal
starts after the dormant POP. Assertions that a route actually specializes
and that direct disposal executes were preserved.

The two commits after runtime integration update cache compatibility:
`242dd307982` advances private register-cache magic to 64411 and updates its
handler-table fingerprint test; `4b3d739d867` explicitly makes `Python/import.o`
depend on `pycore_magic_number.h`. The dependency matters because import's
embedded magic token otherwise survives an incremental header change. These
are focused fixes to producer/consumer consistency, not exemptions from import
tests. `git diff --check 0765e9be47f..HEAD` passed in this review.

The canonical [lifetime policy](../../design/lifetimes-and-language-semantics.md)
correctly separates Python semantics, chosen CPython compatibility, owner/GC
safety and retention quality. Python does not require immediate destruction,
the historical decref location or a dispatched POP for each dead register owner.
Finalizer-PC/order assertions here check the chosen implementation and whether
the optimization executed. State must remain valid whenever release can
re-enter Python. Nothing in this packet rules out a separately reviewed
frame/region cleanup policy, or establishes that delayed cleanup would be slow.

## Qualification evidence

Read the following receipts and corresponding full-log results:

| Evidence under E10 | Recorded result |
| --- | --- |
| `boundary-final-v1-full-checks.json`, release log | Exact final HEAD, empty diff, candidate stdlib/modules, magic 64411; exit 0; 54,421 tests; SUCCESS |
| Same receipt, first debug log | Exact final HEAD and magic; exit 2; one `test_profiling` failure |
| `boundary-final-debug-repeat-v2-full-checks.json` and debug log | Same final HEAD, empty diff and magic; explicit seed 1284068202 matching failed debug campaign; exit 0; 54,421 tests; SUCCESS |
| `boundary-final-v1-lint-checks.json` and release lint log | Final HEAD, empty diff; 306 value-native units, zero failing findings and zero parse errors |
| `profile-module-controls/commands.json` and two logs | Isolated `test_profiling` passed on golden-debug and candidate-debug; 481 tests each |

Full release ran with GCC 15.2.0 on Linux x86-64/GIL and reported 2,531 skipped
tests; successful debug reported 2,471 skipped tests. Both report 619/630 test
files, 79 skipped files and 11 resource-denied files. Resources were disabled.
Thus these are supported-build qualification results, not all-platform,
free-threaded or all-resource qualification.

The first debug failure is concrete: while `test_profiling` ran,
`Lib/test/test_profile.py:90` expected `:0(max)` in profiling output for
`max(*([0],), **dict(key=int))`; it was absent. The log instead showed two
function calls and a negative reported elapsed total. Neither that observation
nor the passing isolated controls establishes its cause. The same-seed passing
full retry establishes non-reproduction in that campaign, not a fixed runtime
defect or proof that this change cannot affect profiling. Keep the original
log, the controls and the retry together. Repeated evidence of this failure
would justify reopening observation/profiling admission.

Earlier focused failures and cache-magic build failures remain part of the
history. The reviewed fixture changes and explicit import dependency explain
those specific repairs; do not merge that explanation with the later unexplained
debug profiling event. Final identity receipts are stronger than an earlier
wrong-executable run or a pre-fix build, which are not final qualification.

## Remaining decision

Review matched elapsed campaigns against immutable golden and upstream, then
the broad roster and relevant counters/profiles. Include costs on ordinary
returns, operand/branch controls and spectral rather than inferring benefit
from generated code or removed dispatch. Preserve regressions and measurement
warnings. The source and passing qualification support proceeding with those
measurements; they do not decide performance admission.

## Target measurement addendum

Independently reviewed completed E10 campaigns `final-target-golden` and
`final-target-upstream`: identity snapshots, status, command journals, retained
stdout duration arrays, generated native drivers and measurement runner. Used
host Python only to verify the recorded arithmetic; no benchmark or native
probe was launched.

Both campaigns have identical before/after snapshots, no reported preflight or
postflight errors, and 36 successful native commands each. The candidate is
the qualified `4b3d739d867b` ELF, build ID
`f3337ce0017f1cd65f08df9e477c21155e99bcd5`; controls identify immutable golden
`0765e9be47f5` and upstream `3a440b656956`. Snapshots include compiler/build
configuration, source cleanliness, module paths/build IDs and workload/driver
hashes. Golden and candidate use their own stdlib and extension roots. GCC
15.2.0 and the selected release configurations are recorded; upstream does not
have the fork's shape option, which is an intentional runtime difference.

The protocol uses six paired rounds with alternating candidate/control order,
CPU 15 affinity, one untimed warmup and seven timed calls per child. Logging
uses 60,000 loops, Richards two and spectral one, identically within each pair.
The runner removes inherited PYTHON variables and LD_PRELOAD, sets hash seed
zero and applies native affinity/JIT-off assertions outside each timed region.
All retained command metadata has affinity `[15]`, JIT disabled and hash
randomization disabled. Recorded one-minute load before children ranges from
1.87 to 2.09 across the campaigns, within the runner's quiet-start threshold.
These checks support matched comparisons; they are not a claim of exclusive
machine occupancy or a statistical confidence interval.

Recomputed each command mean from its seven retained stdout durations, each
paired ratio and the geometric mean of six paired ratios. They agree with
the summaries:

| Workload | Candidate / golden | Candidate / upstream |
| --- | ---: | ---: |
| logging_silent | 1.0014236 | 1.3930813 |
| Richards | 0.9829610 | 1.2889149 |
| spectral_norm | 0.8403156 | 0.6921387 |

Logging shows no established gain over golden: paired ratios span 0.98775 to
1.01460 and the aggregate is effectively flat. It remains about 39.3% slower
than upstream, so logging parity is not reached. Richards improves about 1.7%
against golden but remains slower than upstream. Spectral's observed elapsed
improvement against golden is about 16.0%, with all six paired ratios below
one (0.82885–0.85649). That is a result for this combined candidate and these
campaigns. It does not identify an individual component's contribution or prove
the mechanism behind the improvement; operand, boundary, return-path and
generated-code/layout effects remain confounded.

The broad campaigns and counters/profiles are still pending at this addendum.
Do not decide keep/revert from these three targets alone, or describe the
packet as a logging speedup. The completed target evidence is valid input to
the later admission decision, with the qualification caveat above retained.

## Broad-golden concern

Independently checked completed `final-broad-golden`: 29 workloads, 116
successful commands, equal before/after identity snapshots and clean status.
Recomputed all command means from retained stdout arrays and all paired
ratios. The geometric mean across workloads is **1.004277574**, consistent
with the coordinator's summary. This is a two-round screen, not precise
admission evidence for each workload.

The near-flat aggregate hides substantial regressions. Both rounds regress on
unpack_sequence (1.09679 aggregate), pickle (1.08541), float (1.06495),
deepcopy_memo (1.05332), nbody (1.04899) and generators (1.02580). Unpack's two
ratios are particularly close: 1.096565 and 1.097016. Spectral remains improved
at 0.84393. These observations justify the proposed focused repeat; they do not
justify declaring the combined packet a broad throughput improvement.

There is no identified arithmetic or protocol mismatch in this screen. There
is a mismatch between some simple causal stories and the workload source:
`unpack_sequence.do_unpacking` performs 400 local unpack statements per loop,
with no Python call per unpack; pickle's hot loop repeatedly invokes C
`dumps`. Neither is evidence that direct disposal of Python child results
alone caused its slowdown. Conversely, excluding generator code from discard
selection does not eliminate shared completion-path costs: the common
`IR_STORE_RETURN` now calls `_PyFrame_CompleteReturn` for yields and returns as
well. General call transaction initialization also carries the new completion
flag. These are concrete possible exposure paths, not measured explanations.
Generated text growth or changed layout alone establishes no cause; inspect
counter/profile evidence and controlled component ablations before assigning
responsibility.

A **qualification-only reviewed checkpoint, with performance promotion
withheld**, is warranted. It preserves the requested first experiments and
their results for review while keeping immutable golden as the admitted
baseline. A pushed checkpoint branch is not a new golden runtime. Complete
the broad/upstream screen, focused regression repeat and planned counters
before deciding whether to retain the combined implementation or isolate its
components in a focused ablation. Logging parity is still unmet, and this
review does not recommend performance admission at this point.

## Repeat and admission disposition

Independently checked `final-regressions-golden`: four paired rounds for eight
selected workloads, 64 successful commands, stable before/after identities,
CPU 15, JIT off and hash randomization off. Recomputed every command mean from
retained stdout arrays, paired ratios and geometric means; summaries agree.
This is a selected regression diagnostic, so its aggregate is not a substitute
for the full workload roster.

| Workload | Candidate / golden | Rounds above 1 |
| --- | ---: | ---: |
| unpack_sequence | 1.0982761 | 4/4 |
| pickle | 1.0795516 | 4/4 |
| float | 1.0733627 | 4/4 |
| deepcopy_memo | 1.0644611 | 4/4 |
| nbody | 1.0464147 | 4/4 |
| generators | 1.0042134 | 1/4 |
| logging_silent | 1.0035925 | 3/4 |
| spectral_norm | 0.8398654 | 0/4 |

Five material regressions repeat in every pair. The initial generators
regression does not repeat consistently; do not report it as established.
Logging remains effectively flat, while spectral's combined-candidate gain
repeats. The completed two-round `final-broad-upstream` screen also passed raw
arithmetic and identity checks: 29 workloads, 116 successful commands, aggregate
candidate/upstream **1.244567726**. This is about 24.5% slower on that roster;
it does not establish broad upstream parity.

The first `perf-stat` campaign is rejected evidence: its status records 18
failed invocations out of 18, despite stable identity. The driver rejected
missing CPU affinity before the timed hot region. Preserve those receipts;
no instruction, cycle, cache or branch counts from them are admitted here.
The revised diagnostics wrapper starts the perf host under `taskset -c 15`;
the new v2 stat/record/empty campaigns require independent result checks when
complete. Correcting the wrapper is not itself a successful counter receipt.

Suggested admission wording: **The candidate is retained as a qualified review
checkpoint at `4b3d739d867`; performance promotion is withheld and immutable
golden remains the selected baseline. Release and the debug retry each passed
54,421 tests, with the earlier unexplained debug profiling failure retained.
The combined candidate delivers no established logging gain, remains slower
than upstream on logging and the broad roster, and repeats five material
regressions against golden despite a spectral improvement. These results
support focused component ablation, not promotion of the combined packet.**

The pending valid counters/profiles may help choose that ablation; they cannot
erase the elapsed regressions or retroactively admit the failed first perf
campaign. Component causality remains unresolved.

## Final diagnostic review

Read the completed v2 stat, record and empty-calibration invocation journals,
native child receipts, raw stat files, counter summary, retained ELF/input
identities and self-symbol profile reports. Independently verified all 401
recorded input hashes and both live/retained executable SHA-256 identities.
All 18 stat, 18 empty and two record invocations are valid; their before/after
native identities agree on the expected candidate/upstream HEADs, CPU 15,
JIT off, absent natural method-JIT API and hash randomization off. Campaigns
report unchanged identities and no failures. This admits the v2 diagnostics;
the rejected first campaign remains rejected.

Recomputed stat counts per outer logging iteration from raw hardware counts
and each child's actual hot-loop count, then each role's three-run median.
All event running percentages are 100%; arithmetic agrees with
`perf-summary.json`:

| User-mode event | Upstream per iteration | Candidate per iteration | Candidate / upstream |
| --- | ---: | ---: | ---: |
| instructions | 13,943.9833 | 19,561.0536 | 1.4028311 |
| cycles | 4,756.9001 | 6,653.8034 | 1.3987688 |
| branches | 2,257.9949 | 3,446.0119 | 1.5261381 |
| branch misses | 2.0107 | 2.0116 | 1.0004485 |

Derived CPI is 0.341144 upstream and 0.340156 candidate, nearly identical.
The candidate's additional retired work is consistent with its logging elapsed
gap against upstream. These are candidate/upstream diagnostics, not
candidate/golden ablations: they neither isolate this packet's overhead nor
explain the broad regressions or spectral gain. Cache references/misses vary
widely across the three captures, so their medians do not support a cache-cause
claim.

Empty calibration receipts have zero hot loops, successful enable/disable
acknowledgements and zero child errors. Their total cycles/instructions are
orders of magnitude below the hot captures (tens of thousands of instructions
and roughly 78–146 thousand cycles versus billions in the hot captures).
They establish that gross measurement-boundary overhead is small here. The
admitted per-iteration values are raw normalized hot counts; no empty-count
subtraction or zero-loop normalization is claimed.

Both retained self reports state zero lost samples and about 2K cycle samples.
The candidate's largest reported self shares are overlapping Python argument
entry (14.70%), frame clearing (10.90%), method lookup (8.44%), return (8.19%),
exact call (6.08%) and general-discard call (5.49%). They identify areas for
investigation; they are not a causal speedup budget. Upstream's monolithic
interpreter and the candidate's separate handlers also prevent direct
subtraction of symbol shares. Sampling is too small for precise attribution
of small changes.

Retain the `Invalid HEADER_EVENT_DESC` warning from both header-only reports
and their zero first/last sample timestamps. Flat self-symbol reports work,
with the expected executable build IDs and explicit zero-lost summaries.
Use them as bounded symbol-location diagnostics; do not derive recording
duration or event-description integrity from those broken header fields.

**Final disposition is unchanged:** retain the qualified review checkpoint,
withhold performance promotion, and keep golden selected. Valid counters and
profiles strengthen the evidence that logging still performs substantially
more work than upstream; they do not overturn the repeated elapsed regressions
against golden or justify attributing those regressions to a component.
Proceed to focused component ablation under a separate reviewed experiment.
The earlier unexplained debug profiling failure and every rejected measurement
remain part of the retained record. No new source or diagnostic-admission
blocker was found by this final review.
