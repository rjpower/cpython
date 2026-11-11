# Merged 3+4 integration review

Reviewing clean trial `0e5e61f4c6b` in `BASE/worktrees/round51-golden` against
bf810. Source/native review finds no integration blocker. Release/debug builds
and focused qualification pass. Performance evidence supports the authorized
increment with the losses below. Final release/debug diagnostics introduce no
additional failing group or increased known error count. Recommend promotion
under the authorized incremental policy, with the diagnostic limits below.
Options 1, 2 and 5
are absent from this trial.

The merged call implementation is unchanged from option3's final d135 source;
the arithmetic proof/test implementation is unchanged from option4's final
4bb source. The actual generated diff contains both call families' reservation/
initialization refactor and only ADD/SUB's final canonical destination changes.
No conflicting generator helper, serialized layout, effect change or accidental
selection of another option was found.

Exact calls retain PEP523, function-version/arity, capacity and recursion
admission before reservation. Thread callers select overlapping argument homes;
heap-owned callers select the thread-stack top before any stack arithmetic.
Reservation preserves the caller-tail NULL proof, uses max(callee end, saved
top), truncates initialized storage before writing a header, and fills only the
uncertain suffix. Existing pop restores saved top and does not resurrect an old
frontier. No live owner is retired or callback introduced in that interval.

One header initialization consumes function ownership and acquires its separate
code owner. Non-thread arguments move afterward with the same self/explicit-arg
rules; zero-argument no-self calls preserve initialized constants. Source owners
clear before activation publishes the child. Continuation, observation, unwind
and displaced-local retirement remain shared. The new helpers separate actual
storage reservation, cold suffix filling and cold heap-window argument transfer;
they do not add a competing evaluator or transaction abstraction.

The four newly classified helper calls were inspected: frame address conversion,
reservation, initialization and argument moves cannot allocate, retire owners or
invoke Python. The classification accurately preserves the pure exact-init uop
contract. Option4's proof remains behind scalar destination/input geometry and
requires complete original guard/producer/retirement bodies bound to the emitted
AST. Its scaled-overflow predicate establishes the matching BoxInt64 argument's
precondition; NULL stays an error and altered predicates/arguments/calls/ASTs
fail closed. Call effect classification does not widen this arithmetic proof.

The source/native/focused gates and subsequent direct measurements are reviewed
below. Full diagnostics retaining the documented GDB-only quarantine remain the
open behavior gate. Report material regressions even if the aggregate improves.
Do not infer dynamic savings or microcode/layout causes from smaller handlers
or timing differences alone.

Inspected merged build logs `E/combined34-{release,debug}-build.log`: both check
117 modules with no missing/import failures. `combined34-focus-checks.json`
records clean merged HEAD, matching native revision, source Lib and magic 64418
in both roles. Each behavioral focus log passes 821 tests with 9 skips across
the nine requested modules. The five selected generator cases pass in both
roles; the debug log completes in 124 seconds with no failed cases. The strict
value API log reports 306 units, zero failing findings and zero parse errors.

Read-only inspection of the actual merged release binary confirms exact CALL
at 0x37a660 occupies 0x3cd bytes and has one header/constant initialization region.
The cold argument-transfer call at 0x37a926 is reached only from the non-thread
branch; initialized-frontier fill at 0x37a954 is reached only when its suffix is
missing. The initialized thread-owned route contains neither helper call.
ADD at 0x355ed0 and SUB at 0x355c40 remain 0x282 bytes each, retaining scaled
overflow, BoxInt64, NULL error and displaced-owner handling while omitting
generic result normalization. Composition has preserved both native mechanisms.
These static observations establish code shape, not application speed or a
hardware cause for any timing differences.

This reviewer launched no native execution, builds, regeneration or timing;
root owns those operations.

Focused disposition: approve unchanged trial 3+4 at `0e5e61f4c6b` for matched
measurement. Source composition, actual release native behavior and merged
release/debug focus are independently reviewed, with no unresolved finding.
Performance and full diagnostic dispositions follow below. The separately
measured 2+3+4 trial is not selected; this source approval covers 3+4 only.

PMU harness review: approved the narrow extension-count parameterization in
`frame-call-model-20261109/capture-closing39-gap-perf.py` and new round51
`capture-perf.py`. Historical defaults remain upstream=78/candidate=213. The new
wrapper pins selected `0e5e61f4c6b`, requires exactly 79 candidate shared-extension files,
selects four cases and uses the completed 29-kernel upstream campaign for calibration.
Every extension path/content remains in the before/after snapshot; nothing is
filtered out to make the count pass. The 116/117 module/import gates, executable
and imported-module build IDs, clean source/configuration and child conditions
remain intact. Both underlying script and wrapper are hashed inputs. This
accommodates the clean build's actual census without weakening identity checks;
it does not itself validate a future PMU capture. Review was source-only.

## Final performance and PMU evidence

Independently read the actual timing identity, status, command and summary JSON
for all three `final51-all29-*` campaigns, `combined34-targets-prior50`,
`final51-deepcopy-memo-prior50` and `final51-targets-upstream`. Before/after
identities match with no errors and candidate HEAD is `0e5e61f4c6b`. Recomputed
every paired ratio from the raw seven-sample values, checked equal loop counts
and alternating control/candidate order, and reproduced all reported means.
Each 29-kernel campaign has 116 unique invocations; the six-pair prior target,
memo and upstream target campaigns have 84, 12 and 48 respectively. CPU 15,
hash seed 0, JIT-off and native revision metadata remain consistent.

The 29-kernel geometric means are 0.985699861/prior50, 0.982829049/golden39 and
1.156535259/upstream. The 1.43% aggregate gain over prior50 does not establish parity.
Six-pair confirmation against the actual prior image shows:

| Case | Candidate/prior elapsed | Slower pairs |
| --- | ---: | ---: |
| DeltaBlue | 1.036881 | 6/6 |
| Richards | 0.966903 | 0/6 |
| Comprehensions | 1.000125 | 3/6 |
| Generators | 1.012945 | 6/6 |
| Coroutines | 0.893073 | 0/6 |
| Spectral norm | 0.897685 | 0/6 |
| Silent logging | 0.891657 | 1/6 |
| Deepcopy memo, separate confirmation | 1.019933 | 5/6 |

Logging's range is 0.827254–1.038904; do not call every logging pair a win.
The two-pair screen's DeltaBlue 1.044924 and deepcopy_memo 1.026955 are consistent
with the confirmed losses. Spectral norm remains 1.245339/golden39 in its screen,
despite improving against prior50. The upstream target confirmations still put
DeltaBlue, generators and coroutines above upstream; the aggregate also remains
15.7% slower. The authorized admission accepts these stated costs for repeatable
gains on Richards, coroutines, spectral norm and logging. No timing inconsistency
or undisclosed promotion blocker was found.

Inspected `final51-perf-stat-clean` status, before/after identities and all 24
invocation JSON records. They are complete, valid, unique and stable; every
event reports 100% running, with CPU/hash/JIT/native child conditions intact.
Recomputed count-per-loop candidate/upstream ratios from the event records and
reproduced `normalized-summary.json`. Instructions/user-cycle ratios are
1.352487/1.173937 for DeltaBlue, 1.414550/1.443516 for generators,
1.388163/1.249676 for coroutines and 0.953890/0.730284 for logging. These counters
describe the remaining upstream gap; they do not establish the cause of this
increment's gains or losses against prior50. The initial `final51-perf-stat`
attempt has zero invocations and a failed extension-count preflight; it is not
used as PMU evidence.

`final51-perf-record` has four complete, valid, stable captures. All four loss
files are empty, and each recorded-hit build-ID list contains its role's
executable ID. Candidate generator flat-profile rankings reproduce SEND_GEN 27.41%,
YIELD_VALUE 13.92%, RETURN_GENERATOR 2.39%, completion 1.44% and frame-local
clearing 1.36%. These are sample shares, not removable fractions or comparable
per-handler upstream costs. Profile and counter results support further
investigation of the remaining execution gap without proving a layout cause.

Performance disposition: support retaining unchanged 3+4 under the explicitly
authorized incremental policy. Source/native/focus and measurement evidence are
consistent. Final release/debug diagnostic evidence below closes this review
with the known failures and explicit GDB coverage gap retained.

## Full diagnostic evidence

Release completed in `E/final51-rfull-release-full.log`: 54,689 filtered tests,
2,558 skips, exit 2. The journal records clean `0e5e61f4c6b` source, matching
native revision, source Lib and magic 64418. Only test_sys_settrace fails, with
nine errors; GDB backtrace, C-function, full C-function and miscellaneous groups
pass. The quarantined PrettyPrintTests class runs no tests. This is a filtered
diagnostic run with known errors, not a green full suite.

The nine current errors are JumpTestCase methods test_jump_across_async_with,
test_jump_backward_over_async_listcomp, test_jump_backward_over_async_listcomp_v2,
test_jump_between_except_blocks, test_jump_between_except_blocks_2,
test_jump_in_nested_finally_2, test_jump_in_nested_finally_3,
test_jump_out_of_async_for_block_backwards and
test_jump_out_of_async_for_block_forwards. The inspected exceptions refuse
f_lineno jumps that require an incoming temporary register owner. The prior50
quiet release log also reports nine test_sys_settrace errors but omits individual
IDs; the established module/count limit matches, while case-for-case equivalence
is not established. No additional failing release group appears.

Debug completed in `E/final51-dfull-debug-full.log`: 54,689 filtered tests,
2,496 skips, exit 2, in 14 min 19 sec. Its journal records the same clean full
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254` source, matching native revision,
source Lib and magic 64418. Only test_sys_settrace (nine errors) and test_lltrace
(one failure) fail. The current debug sys_settrace IDs match the nine current
release IDs above; the prior50 quiet debug log establishes only the same module
and error count. The LLTrace failure is specifically
`TestLLTrace.test_lltrace`, line 58, expecting `POP_TOP` in stdout; the prior50
debug log records that same test, assertion and line. No additional failing
debug group appears. All four other GDB groups pass and the quarantined class
runs no tests. Full test_generated_cases passes, taking 7 min 48 sec.

Final independent disposition: approve unchanged 3+4 for the authorized
incremental promotion. Actual merged source/native review, focused results,
timing/PMU identities and full diagnostic results are consistent; no new
integration blocker was found. This is neither upstream performance parity nor
a green full suite. DeltaBlue, generators and deepcopy memo retain measured
costs, PrettyPrintTests remains an explicit coverage gap, and the existing
debugger-jump/LLTrace failures remain unresolved. No source changes or additional
native execution were needed to close this evidence review.
