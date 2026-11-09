# Execution boundaries and operand modes: checkpoint report

2026-11-09 UTC. Source is `4b3d739d867b06ea4c9ed936982cfeb3503e93be` in
`build/worktrees/execution-boundaries-20261108`. Release and debug each passed
54,421 full-suite tests, debug on retry with an unexplained earlier profiling
failure retained. **Performance promotion is withheld:** logging is flat and
several broad regressions repeat. This is a reviewed candidate checkpoint.
Golden `0765e9be47f` is immutable.

The first three golden-review ideas now have bounded implementations. They
remove demonstrated execution/slot work while retaining canonical words and
existing owner/observer contracts. They do not establish upstream parity.

## What changed

| Commit | Result |
| --- | --- |
| `f2c5ac3052b` | One LOAD_ATTR_SHAPE alias fixes local-receiver/temporary-result ownership mode |
| `5bfd1e7e457` | Three bool-edge aliases omit admitted NOP traversal; exact/general Python-call aliases complete ignored results directly |
| `242dd307982` | Cache magic 64411 and handler-table pin follow the table policy |
| `4b3d739d867` | import.o depends on its private magic header, preventing a stale exported token |

`POP_TOP` names a register owner to retire; it is not an operand-stack pop.
Ignored synchronous Python results stay owned in C across child cleanup. A
tagged caller recipient then either closes the result at CALL completion or
publishes its original home for the live POP observation path. General entry
keeps destinations numeric through callbacks/native fallback and tags only
committed Python children. Generator callers/callees and native completion are
excluded. Existing ordinary return, error and ownership paths remain explicit.

Bool aliases encode taken/fallthrough skips once. Generated SAVES_IP facts
exclude unsafe successors; instrumentation invalidates dependent aliases before
patching anchors. These aliases are disabled in debug/free-threaded builds.
Canonical NOP/POP words, jumps and source/exception coordinates remain: this is
execution elimination, **not physical word compaction**.

Python permits delayed collection. Frame/region-owned ignored results and later
retirement remain valid alternatives; they were neither implemented nor rejected.
The chosen CALL completion keeps prompt disposal as an implementation policy.
Without a live POP line/opcode action, raw f_lasti can now identify CALL. POP
actions active before completion materialize the home; actions enabled inside
the result finalizer begin at the next real instruction, without retroactive POP.
See [lifetime policy](../../design/lifetimes-and-language-semantics.md).

## Demonstrated work, not inferred speedup

Stats census at 0/1,000/2,000 logging iterations is linear after empty subtraction:
**142 entered handlers per outer iteration versus golden's 151; zero POP entries;
20 Python frame entries unchanged.** Warmed IR selects nine general discard
aliases, two shape-mode sites and one bool taken-skip site. Physical selection
is not a per-site hit counter. Golden already elided NOP dispatch; do not count
another NOP dispatch saving. Evidence: `census-v2/summary.json` under the build
packet `pyjit-link-wave1/execution-boundaries-20261108/`.

[GCC operand review](operand-compiled-review.md) verifies two removed class
branches and a representative 70→48 instruction path; the added alias is 224 bytes.
[Boundary compiled review](boundary-compiled-review.md) confirms no NOP test/PC
store in skip aliases and one recipient comparison on ordinary return. The five
new boundary/discard handlers add 2,213 symbol bytes; RETURN_VALUE grows 144 bytes.
The transaction remains 64 bytes, using old padding for its new field. Extra
flag initialization, checks, text and changed branch layout still have costs.

The early **operand-only** two-round screen was logging/golden 1.01372,
Richards/golden 0.98583 and spectral/golden 0.99995; logging/upstream was 1.39889.
It demonstrated no logging win. The source-identical prepared baseline was
logging/golden 0.99827. These short screens and static counts do not decide the
combined change or satisfy the balanced six-round parity protocol. Keep native
JIT-off identities, matched workloads/build paths and CPU-pinning receipts with
all final comparisons. The combined results below determine admission.

## Elapsed result and admission

All ratios are candidate/control; lower is better. GCC 15.2.0, native JIT off,
CPU 15, fixed hash seed and unchanged bodies/loops/driver. Each invocation has
seven timings after warmup. Roles alternate by round; all final before/after
identities agree, with clean source and separate matched modules. No native
build or test jobs ran alongside timing.

| Workload | / Golden, six rounds | / Upstream, six rounds |
| --- | ---: | ---: |
| logging_silent | 1.00142 | 1.39308 |
| richards | 0.98296 | 1.28891 |
| spectral_norm | 0.84032 | 0.69214 |

The 29-workload two-round geomean is **1.00428 / golden** and
**1.24457 / upstream**. This is no aggregate improvement. Logging does not
improve despite nine fewer entered handlers. Parity clearly fails; another
six-round parity campaign is unnecessary to establish that failure.

A separate four-round golden repeat confirms the broad regressions:

| Workload | Repeat ratio | Interpretation |
| --- | ---: | --- |
| unpack_sequence | 1.09828 | Regression in all four rounds |
| pickle | 1.07955 | Regression in all four rounds |
| float | 1.07336 | Regression in all four rounds |
| deepcopy_memo | 1.06446 | Regression in all four rounds |
| nbody | 1.04641 | Regression in all four rounds |
| generators | 1.00421 | Initial regression did not repeat consistently |
| logging_silent | 1.00359 | Still flat |
| spectral_norm | 0.83987 | Improvement repeats |

**Decision: retain the reviewed source branch and evidence as a qualified
candidate; withhold performance promotion. Golden remains the selected runtime.**
The spectral improvement is real for this combined image, but neither its cause
nor the cause of the broad regressions is isolated. The earlier operand-only
screen did not improve logging or spectral. Whole-image text/layout and shared
paths are hypotheses, not explanations. Unpack has no Python call per unpack;
the new return-recipient branch alone cannot account for its regression by
source inspection. A bounded source ablation comes before wider rollout.

Evidence directories: `final-target-{golden,upstream}`, `final-broad-{golden,upstream}`,
`final-regressions-golden`, `elapsed-summary.json` and `elapsed-table.md` under
`build/pyjit-link-wave1/execution-boundaries-20261108/`. All earlier screens
remain retained and are not substituted for these final-source measurements.
See the independent [admission review](final-admission-review.md).

## Hardware work and profiles

Three warmed-region counter rounds per role, six events in three groups, were
collected with explicit CPU 15 affinity and JIT-off assertions. All 18 captures
are valid, every event ran 100%, and binary/module/workload hashes stayed fixed.
Normalization is per unchanged logging outer iteration, not incomparable stack
versus register bytecode counts. Twenty Python frame entries remain per iteration.

| Event / outer iteration | Upstream | Candidate | Ratio |
| --- | ---: | ---: | ---: |
| instructions:u | 13,943.98 | 19,561.05 | 1.4028 |
| cycles:u | 4,756.90 | 6,653.80 | 1.3988 |
| branches:u | 2,257.99 | 3,446.01 | 1.5261 |
| branch-misses:u | 2.01 | 2.01 | 1.0004 |

Cycles per instruction are **0.3411 upstream / 0.3402 candidate**.
The candidate retires roughly 5,617 extra instructions per logging iteration;
the observed cycle gap follows that added work. Branch misses are similar and
low. These data support reducing executed work before speculating about a
fundamental per-instruction penalty. Generic cache events vary substantially
across repeats and are not a basis for assigning the gap to cache behavior.

The two five-second cycle profiles report zero lost samples and about 2,000
samples each. Candidate self shares locate work in overlap-call preparation
(14.70%), frame clear (10.90%), method lookup (8.44%), return (8.19%), exact
Python entry (6.08%) and general discard entry (5.49%). Shares identify code to
inspect; they are not additive causal speedup estimates. The header reader
still reports `Invalid HEADER_EVENT_DESC` and zero sample timestamps. Preserve
that metadata warning; the native timed-region receipts, build IDs and flat
symbol samples remain available for inspection. No clean-header claim is made.

Evidence: `perf-stat-v2`, `perf-record-v2`, `perf-empty-v2`,
`perf-summary.json`, `profile-summary.json` and `profile-*-self.txt` under the
build packet. Eighteen empty-region controls pass and bound fixed capture work;
no empty count was subtracted from the reported hardware counters. The first
`perf-stat` attempt lacked outer CPU pinning and was rejected before enabling
the hot region in all 18 invocations. Its data are excluded, not repaired or
relabeled. The corrected wrapper pins the host and native children explicitly.

## Qualification and retained failures

Independent proposal, source and merged reviews preceded coordinator integration;
Sol 6.1 medium workers performed bulk work. The coordinator owns native gates.
[Source review](merged-source-review.md), [discard review](discard-completion-code-review.md)
and [cold review](cold-integrated-review.md) retain findings and limits.

Operand focus passed 465 tests in each release/debug build. Repaired combined
focus passed both profiles; stats smoke passed 142 tests. Final API lint covered
306 units with zero failing findings. These do not replace final full suites.

The initial 5bfd full release run executed 54,421 tests with two failures:
handler-table cache metadata, and the intentional hidden-array GC-verifier
fixture. The magic/pin repair then exposed a stale native 64410 token because
import.o lacked its header dependency; failed cache-v2 receipts remain retained.
After the dependency fix, both profiles export 64411 and cache-v3 import/GC
focus passed 155 tests each; stats smoke passed 142. Matched fresh-process GC
module controls passed golden/candidate/debug, without any GC code change.
Those controls do not establish the cause of the original GC failure.

Final source `4b3d739d867` passed **54,421 tests in release** in 3m57s. Its first
debug full run completed 54,421 tests with one `test_profiling` failure: pure
Python profiling of a starred-keyword `max` call lacked expected entries.
No runtime repair followed. Whole-module controls and 24,000 actual shared-code
profiling statements across candidate/golden release/debug passed. The debug
full retry, using explicit seed 1284068202 and randomized test order, passed
**54,421 tests** in 10m23s. This is a passing retry, not a demonstrated repair.
The [investigation](profiling-investigation.md) retains the original log and
limits. Both full passes covered 619/630 test files, with 79 skipped files and
11 resource-denied files. Qualification is Linux x86-64/GIL, GCC 15.2.0;
free-threaded, other platforms and disabled resources are not qualified.

Earlier generator, adaptive-display and shared-code fixture failures and their
repairs also remain in the packet. No failing receipt is relabeled as a pass.
Retired component sources are archived in build evidence
`component-source-archive.json` and `*-final-component.patch`; active integration
source, builds and golden controls remain intact.
See [next steps](next-steps.md) for the bounded ablation and ownership/call-cost
ledger before any further optimization is selected.

## Full broad screen

Two balanced rounds per control; use the six-round target and four-round
regression repeat for the stronger conclusions above.

| Workload | Candidate / golden | Candidate / upstream |
| --- | ---: | ---: |
| richards | 0.9768 | 1.2795 |
| go | 1.0061 | 1.2901 |
| nbody | 1.0490 | 0.8929 |
| float | 1.0650 | 0.8531 |
| spectral_norm | 0.8439 | 0.6976 |
| raytrace | 0.9905 | 1.0320 |
| deltablue | 0.9851 | 1.7487 |
| chaos | 0.9708 | 0.9705 |
| nqueens | 1.0062 | 1.2516 |
| fannkuch | 0.9982 | 1.2868 |
| generators | 1.0258 | 1.3821 |
| comprehensions | 0.9976 | 1.4126 |
| coroutines | 0.9987 | 1.3588 |
| unpack_sequence | 1.0968 | 1.5606 |
| deepcopy | 1.0103 | 1.4314 |
| deepcopy_reduce | 1.0010 | 1.4129 |
| deepcopy_memo | 1.0533 | 1.3934 |
| json_dumps | 0.9905 | 1.3574 |
| json_loads | 0.9972 | 1.2196 |
| pickle | 1.0854 | 1.2308 |
| unpickle | 0.9610 | 1.1666 |
| regex_compile | 1.0105 | 1.2128 |
| logging_format | 1.0039 | 1.3258 |
| logging_silent | 1.0092 | 1.3682 |
| logging_simple | 0.9988 | 1.3553 |
| xml_etree_generate | 1.0012 | 1.3024 |
| xml_etree_iterparse | 1.0166 | 1.3091 |
| xml_etree_parse | 1.0019 | 1.1370 |
| xml_etree_process | 1.0010 | 1.4362 |
| **29-workload geomean** | **1.0043** | **1.2446** |
