# Round54 qualification snapshot

Completed checks. The coordinator rejects all performance candidates and selects
only cold correctness checkpoint54
`61015a9452d9069646fb5fede9ff963896833b89`, with the limits recorded below. Focused
qualification does not imply a green full suite, free-threaded qualification or
a performance win. [Benchmark tables](benchmark-tables.md) retain the distinct
campaign sizes and candidate identities.

E=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/frame-state54`.
Build/source roots follow [plan](plan.md). The checked JSON manifests record
clean source, matching native `sys._git`, expected magic 64418 and return code 0.
The component, compact, R2, combined and cold build pairs listed below checked 117
modules with 0 failed imports. The counts below reproduce filtered-test logs; separate groups are
not claimed as deduplicated totals.

## Completed component focus

| Candidate | Reviewed/current head | Actual GIL header | Release and debug focus |
| --- | --- | ---: | --- |
| R: recipient | `612fb4e9d170532947c007f0ccaaa04aff623329` |112 bytes |567 tests each role |
| F: feedback | `7ea40d5d67ccb5a9250b8313646efb537c7ff934` |104 bytes |578 tests each role |
| N: namespaces | `c2cc6777e4d72e6688854c0423459971fc81b355` |96 bytes |772 tests, 6 skips; separate 27 environment tests, each role |
| L: mapping | `b48fe2d39896fef83d795bba3ec9c95af68cc235` |104 bytes |84 frame/C-API tests, 8 skips; 640 core; 2 generated-effect tests, each role |
| S: initial overwrite | `4fbd16a14621f2b5327a0bfde6c2198190e71428` |112 bytes |245 runtime tests; 2 generated-proof tests, each role |
| R2: recipient NONE | `fcc82220ddd42fc49ffd1885dcd79e6964889703` |112 bytes |813 tests per role; 10 skips release, 9 debug; separate remote test per role |
| Combined R2+S2 | `f3c464a54c89783e50af1ce9f3cffec6f5e7279d` |112 bytes |1,152 tests per role; 19 skips release, 18 debug; separate strict value-API gate |
| S2: validated overwrite | `e95328ba1d6f3ea74dd92d0870213367287d64ef` |112 bytes |453 runtime tests, 9 skips; 2 generated-proof tests, each role |

Independent source/native details, ELF identities and mechanism limits live in
[R](option1-post-review.md), [F](option2-post-review.md),
[N](option3-post-review.md), [L](option4-post-review.md),
[S](option5-post-review.md), [S2](option5-validated-post-review.md),
[R2](recipient-default-post-review.md) and [combined](combined-post-review.md).
Header sizes are fixed bytes before owning homes,
not sizeof including the declared trailing home or total heap allocation.

| Check manifest under E | Matching focus logs |
| --- | --- |
| `recipient-corrected-focus-checks.json` | `recipient-corrected-{release,debug}-focus.log` |
| `feedback-focus-focus-checks.json` | `feedback-focus-{release,debug}-focus.log` |
| `globals-{core,env}-focus-checks.json` | `globals-{core,env}-{release,debug}-focus.log` |
| `locals-corrected-{frame,core}-focus-checks.json`; `locals-effect-proof-focus-checks.json` | `locals-corrected-{frame,core}-{release,debug}-focus.log`; `locals-effect-proof-{release,debug}-focus.log` |
| `clears-overwrite-{runtime,proof}-focus-checks.json` | `clears-overwrite-{runtime,proof}-{release,debug}-focus.log` |
| `recipient-default-focus-focus-checks.json`; `recipient-default-remote-focus-checks.json` | `recipient-default-focus-{release,debug}-focus.log`; `recipient-default-remote-{release,debug}-focus.log` |
| `combined-focus-focus-checks.json` | `combined-focus-{release,debug}-focus.log` |
| `clears-validated-{focus,proof}-focus-checks.json` | `clears-validated-{focus,proof}-{release,debug}-focus.log` |

R's one-test remote check passed both roles at the earlier corrected ancestor
`daab07a69851edeb9632caadc5b2651c48a22c2c`, recorded in
`recipient-remote-focus-checks.json`. It is not added to the current head's 567
tests. Matching build evidence is in
`feedback-initial-{release,debug}-build.log`,
`recipient-assert-fix-{release,debug}-build.log`,
`globals-trace-fix-{release,debug}-build.log`,
`locals-effects-fix-{release,debug}-build.log`,
`clears-overwrite-fix-{release,debug}-build.log` and
`clears-validated-initial-{release,debug}-build.log`.

## Compact composition

F+N+L source head `82afe7aef2d86c0c01f60c681b082712171a92b4` has
[cold source GO](compact-peer.md). Root's non-running GDB check confirms the
actual 80-byte GIL header; this replaces the source review's predicted geometry
only, without a performance inference.

Both roles pass all 10 focused modules: 946 filtered tests per role, 16 skipped
in release and 15 in debug. Evidence:
`E/compact-focus-focus-checks.json`,
`E/compact-focus-{release,debug}-focus.log` and
`E/compact-initial-{release,debug}-build.log`.
Those 10 modules are frame, frame C-API, call, opcache, generators, coroutines,
monitoring, GC, IR contract and warnings. This is merged focused evidence.
Full qualification was omitted after compact failed the performance screen;
it is not an outstanding admission gate for this rejected experiment.
The coordinator rejects compact after the completed two-pair hot screen
in `E/compact-screen-hot-golden/summary.json`; an 80-byte header does not by
itself justify the measured cost. This does not select another candidate.

## Preserved failures and next candidate

Earlier failed diagnostics remain evidence: R's helper-purity/generated-owner
finding, N's eager LLTrace sentinel decode, L's premature record visibility and
generator effect/first-input findings, and S's ordinary temporary assertion on
the intentionally uncleared inline home. Their corrections and matching
current identities are described in the component post-reviews. The successful
native S census uses the four `clears-hits-snapshot-*.log` files; the earlier
failed diagnostic attempt is not a runtime failure or an admission result.

Constructor-validated S2 at `e95328ba1d6` now has matching
[source](option5-validated-post-source.md) and
[generated/native](option5-validated-post-review.md) GO, completed builds and
the focused results above. Its own hot screen, six-pair hot confirmation,
two-pair all29 screen and matching native census appear in the benchmark tables.
Original S evidence remains separate. The coordinator rejects S2 along with
all other performance candidates; no performance candidate is admitted.

R2-NONE at `fcc82220ddd` has [proposal](recipient-default-proposal.md),
[peer](recipient-default-peer.md), [source](recipient-default-post-source.md)
and [matching native GO](recipient-default-post-review.md). Its 13-module
focus passes 813 filtered tests per role (10 skips release, 9 debug); the
separate one-test remote check passes both roles. Matching build logs are
`E/recipient-default-initial-{release,debug}-build.log`. R2's own completed
six-pair hot confirmation and two-pair all29 screen appear in the benchmark
tables; no result from original R is relabeled as R2.

### R2 constructor allocation-failure probe

`E/capi-allocation-probe.py` injects each of 48 single-allocation failpoints
for each of generator, coroutine and async-generator public construction,
with coroutine origin depth 2. Each iteration collects and checks code and
namespace reference counts against the established baseline. Both role logs
record the same outcomes:

| Subtype | Injection points per role | MemoryError outcomes | Successful constructions |
| --- | ---: | ---: | ---: |
| generator |48 |3 |45 |
| coroutine |48 |5 |43 |
| async generator |48 |3 |45 |

This is 48 points × 3 subtypes × 2 roles, not 48 observed failures per subtype.
Both commands return 0 in `E/recipient-default-allocation-commands.json`;
matching identities and raw counts are in
`E/recipient-default-allocation-{release,debug}.log`. This bounded probe
qualifies these injected constructor paths, not every possible allocation.

### Combined focused qualification and strict value API

Combined `f3c464a54c8` passes all 18 requested modules in each role: 1,152
filtered tests, 19 skips release and 18 debug. The manifest confirms clean
matching source/native identities and returncode 0. Modules are frame, frame
C-API, debugger entry, call, generators, coroutines, asyncgen, yield-from,
monitoring, IR completion, discard completion, completion payload, GC,
operand modes, code, marshal, opcache and IR contract. Matching evidence is
`E/combined-focus-focus-checks.json`,
`E/combined-focus-{release,debug}-focus.log` and
`E/combined-initial-{release,debug}-build.log`.

The separate strict value-API AST gate reports 306 value-native units,
0 failing findings and 0 parse errors; informational categories are home-copy 93,
slot-read 151 and unchecked 1. Its command uses the combined source and rebases
selected51's compilation database to that checkout. Evidence is
`E/combined-value-api-command.json`, `E/combined-compile-commands.json` and
`E/combined-value-api.log`. This is a source/AST gate, not 306 runtime tests.
Combined's initial two-pair hot screen, completed six-pair confirmation and
two-pair all29 screen appear separately in the benchmark tables. The
coordinator rejects the performance candidates because material coroutine
regressions persist; broader aggregate wins do not override that gate.
The separate cold correctness-only extraction below supplies the selected
checkpoint; no performance experiment is absorbed.

## Separate cold correctness-only extraction

Cold candidate `61015a9452d9069646fb5fede9ff963896833b89` extracts only the
public generator/coroutine/async-generator construction correction onto
selected51. It contains no R/NONE/S hot change. Independent
[source review](cold-construction-post-source-review.md) and
[matching native review](cold-construction-post-review.md) approve the bounded
mechanism. Both actual GIL headers remain 112 bytes; both-role normalized hot
CALL/SEND/YIELD/RETURN and normal _Py_MakeCoro sequences match selected51.
That comparison is not a throughput or full-suite claim.

Its own 13-module focus passes 808 filtered tests in each role, with 10 skips
release and 9 debug. The matching manifest records clean source, actual native
`sys._git` at `61015a9452d` and returncode 0 in both roles. The modules are
frame, frame C-API, debugger entry, call, generators, coroutines, asyncgen,
yield-from, monitoring, IR completion, discard completion, completion payload
and GC. Evidence: `E/cold-construction-focus-focus-checks.json`,
`E/cold-construction-focus-{release,debug}-focus.log` and
`E/cold-construction-initial-{release,debug}-build.log`.

The extraction also has its own execution of the bounded constructor failure
probe, rather than inheriting R2's result. For each role, the probe injects
48 points per subtype with coroutine-origin depth 2, checks code/namespace
reference counts after each attempt and records:

| Subtype | Injection points per role | MemoryError outcomes | Successful constructions |
| --- | ---: | ---: | ---: |
| generator |48 |3 |45 |
| coroutine |48 |5 |43 |
| async generator |48 |3 |45 |

Thus 48 points × 3 subtypes × 2 roles completed; only the listed attempts
failed allocation. Matching identities and results are retained in
`E/cold-construction-allocation-{release,debug}.log` and
`E/cold-construction-allocation-commands.json`. This bounded evidence does
not cover every possible allocation failure.

The extraction's strict value-API AST gate reports 306 value-native units,
0 failing findings and 0 parse errors, with informational home-copy 93,
slot-read 151 and unchecked 1. The command uses this candidate's source head
and a rebased selected51 compilation database. Evidence:
`E/cold-construction-value-api.log`,
`E/cold-construction-value-api-command.json` and
`E/cold-construction-compile-commands.json`. These are AST units, not runtime
test counts.

Full release and debug diagnostics are complete. Both run 54,692 filtered
tests. Release has nine debugger-jump errors and one GC-verifier failure;
debug has those nine debugger errors, one LLTrace failure and one profiling
failure. These are failed full diagnostics, not passing suites. Logs are
`E/cold-construction-release-full-release-full.log` and
`E/cold-construction-debug-full-debug-full.log`, with respective
`cold-construction-{release,debug}-full-full-checks.json` manifests.

Verbose targeted repeats have exactly the same nine debugger error identities
and debug LLTrace failure as selected51's prior diagnostics. Evidence:
`E/cold-diagnostic-repeat-commands.json` and role logs. The GC-verifier failure
is demonstrably unstable on selected51 too: six alternating release runs fail
four times on each build. See [triage](gc-verifier-diagnostic.md); this does
not establish the exact conservative root or broaden a quarantine.

The profiling failure concerns fake-clock event accounting, not elapsed-time
noise. Three targeted repeats per debug build pass, as do whole tracing-module
and complete-package reruns on each: ten passing subprocesses in total.
The original full-run finding remains unexplained; it is neither proven
preexisting nor fixed. See [profiling triage](profiling-diagnostic.md).
The coordinator selects the cold correction with this open qualification
limit. Published branch `pyjit/round54-golden` and annotated tag
`pyjit-golden-20261112-frame-state` resolve to `61015a9452d9069646fb5fede9ff963896833b89`.
This is a correctness checkpoint, not a clean-full-suite or performance claim.

These focused runs retain the narrow
`test.test_gdb.test_pretty_print.PrettyPrintTests.*` quarantine where the
orchestrator specifies it; they do not broaden it. No round54 full-suite pass
is claimed. Selected51's documented debugger-jump/LLTrace diagnostic limits
remain unchanged until coherent full diagnostics establish otherwise.


## Final measurement and retention closure

Final prior-golden all29 and hot6 campaigns complete with clean stable
identities, as do upstream all29 and the replacement
`final54-hot6-confirm2-upstream`. The original `final54-hot6-upstream` failed
its load guard and remains excluded. Both `final54-stat-cycles` and
`final54-stat-branches` complete 18/18 valid invocations; `final54-record`
completes 6/6 with stable identities and no lost records. See
[final tables](benchmark-tables.md); these diagnostics do not make the failed
full suites pass or establish performance improvement.

All nine rejected branches are published. E/retired-experiments retains
patches, commit logs and 18 verified archives containing 3,042 files; only
27 task-owned rejected source/build directories were removed. Selected54,
golden51, upstream and older controls remain. Round54 is closed; active
[round55](../call-model55/plan.md) is architecture research and plan only.
