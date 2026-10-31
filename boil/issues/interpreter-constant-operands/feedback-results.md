# First native feedback evidence

Root activated the [reviewed helpers](reviews/feedback-helpers-20261031-v1.md)
for exact source `7f0b1819f84ee11b354d60394a4975e1fcb0a188` at
2026-10-31 **21:20:45.868428 UTC**. The sole sixty-minute batch ends at
**22:20:45.868428 UTC**, with first-failure stop and no automatic retry.
The GC invocation had terminated and verified idle cleanup before this launch;
GC source/fixture diagnosis continues without a second GC native invocation.

The root-owned exec session is **71460**. Evidence is under the isolated
checkout's `build/constant-operands-feedback/`:

| Artifact | SHA-256 |
| --- | --- |
| `plan.json` | `32c27a4025cfc9f0c10bee19a3111ecf114dd9458945d32d8d3a23d0e97ff70e` |
| `launch-held.json` | `a70b94aec5d6ee0f1b857e09230d9bc30552815a00505ed45e0b52dfe8d38517` |
| `activation.json` | `f6024cfc993788931a9cb3538fe57568b02462e01300e50797490a03ee88803e` |
| `feedback-batch-start.json` | `ffd89d889dbd50cc13dc65412ab4c66ddbd4c6372a40397ecbe6d108d419503f` |

At 21:22 UTC, tool admission and configuration had completed and debug
compilation was running. No target-test, emitted-work, qualification or
performance result is recorded yet. The owner monitors this invocation;
changes or a retry require a separate bounded decision and review.

## First selection failed; source unchanged

The debug build and post-cleanup verification passed. The exact eleven-case
selection then ran in 0.140 seconds: ten passes, one failure, zero skips.
`test_shared_or_canonical_producer_stays` failed at its assertion that none
of the three source examples has an immediate RHS. Its loop did not identify
which function failed. The target returned 2; the enclosing row returned 1.
The batch stopped at **21:22:30.262695 UTC**. Session 71460 is terminal, and
post-gate cleanup verified an empty PID set under the exact build lock.
Full IR, the five compatibility modules, O3 and emission rows did not run.

Independent source inspection identifies a concrete hypothesis: an existing
flowgraph pass inlines a small exit block into an unconditional predecessor.
The conditional expression's arithmetic/return join can therefore become a
same-block, fresh, single-use constant consumer before the new register pass.
That would make the source-spelling negative oracle incorrect while preserving
the actual raw-CFG canonical-register rejection, whose test passed. This
interpretation still needs the exact emitted words; the failing assertion is
preserved and neither the compiler nor test has been changed.

Root accepted preparation of one held, compile-only diagnostic extracting the
three exact nested CodeTypes from the frozen test method. It must report their
original words without calling the nested functions or tests, changing source
spellings or altering conditions. Independent probe review and root activation
precede execution; its five-minute exposure remains within the original
22:20:45.868428 UTC deadline. No performance or behavior acceptance follows
from that observation alone.

| Artifact | SHA-256 |
| --- | --- |
| `debug/build-result.json` | `946f4917fbf722a6b656dbe72824d78d23e2c773799c69368cbe42aebed0ec12` |
| `debug/post-cleanup.json` | `17da99d273c356edfd861ff382436bf97beaa889c0960465e9f1fade26f393a7` |
| `feedback-batch-failure.json` | `78a4512e810a6f93f933624c1be4d9dc159b6dfe1ce2e4e15aaa2b8ac1c5c7aa` |
| `debug-0-new-cases.post-gate-cleanup.json` | `faaaa262017e40b40fa5b551fd598f32a717e112fb62d96464154d8e2c97279f` |
| `affected/debug-0-new-cases.result.json` | `1db0a2ad18c5ba81efba9494da2ee6bc2c4902e0daea2d838df3911a90480104` |
| `affected/debug-0-new-cases.log` | `1115089156461e410d31cc8c1b4bba9db87e4f3a8422c82b6b3420f55f3b47ab` |

## Observed words and reviewed test-only continuation

The exact-code observation ran once in session 36051 at 21:33 UTC, returned
zero and passed verification/idle cleanup. It recorded 27 collections and
zero fallbacks. It called neither the three nested functions nor their test.
Shared/dynamic inputs retain ordinary forms; only the inlined true branch of
`canonical` has ADD_SMALL_RHS literal 1. Its false branch retains literal 0's
load and ordinary ADD. The [test correction review](reviews/test-oracle-correction-20261031-v1.md)
binds all inner and external outer receipts through audit
`d7bf9af6357bc56c9f7b778c250715c72fc028f98204cabc9883832c75a53efd`.

Commit `a475acbf46d117137f7201adcb4557ce92fb9ec5` changes only
`Lib/test/test_ir.py`: the renamed control keeps shared/dynamic rejection,
asserts the witnessed branch forms, and retains both branch outputs. Raw-CFG
rejection cases and all runtime/compiler/generator sources are unchanged.
The original failed assertion is preserved; the correction has its own scope.

Root accepted the [remaining helpers](reviews/remaining-helpers-20261031-v1.md)
and activated them at **21:40:36.097965 UTC**, session **7327**. Debug execution
uses the explicitly bridged 7f0 native image and a475 test source; a later fresh
O3 build will name a475. The original deadline remains **22:20:45.868428 UTC**.
No debug product or old receipt is rewritten. New plan SHA-256 is
`630f7d4408d7709c93c885ae1685ed1391d72ff934be9e49940e1cb662036b39`;
manifest is `2a436739b8846b3825466a913741d397c6c1e2e09141d9c1606b9e378e96a2d4`.

At 21:40:46 UTC the corrected eleven-case selection passed with zero skips,
including terminal verification and idle cleanup. Full `test_ir` then started.
Broader behavior, emitted-work, qualification and performance decisions remain
open; this checkpoint does not accept the complete representation.

### Broader checks and first disassembly golden

The same continuation passed all 74 `test_ir` cases. The generator suite ran
83 tests: 82 ordinary passes and one existing expected failure, with no
unexpected results. Their terminal verification and cleanup passed.

`test_dis` then stopped at its first failure after 45 cases (44 passes):
`DisTests.test_disassemble_str` expected a separate LOAD_SMALL_INT for `x + 1`
and a register RHS. Actual output removes that load and displays literal 1 in
the arithmetic word. The failure and complete diff are retained. Session 7327
is terminal; idle cleanup passed at 21:42:25.657210 UTC. Later code/trace/
monitoring, O3 and emission rows did not run.

Root authorized preparation of a separately bounded diagnostic inventory:
the unchanged full disassembly module without fail-fast, plus actual output
for the five exact existing fixture strings in the failed method. Assertion
differences are observations to classify individually, not compatibility credit
or permission to replace expectations blindly. Unexpected native/status/error
results stop the diagnostic. The source, conditions and failed receipts remain
unchanged. Exact helper review and root activation precede execution, after the
GC native window. Its own ten-minute cap also retains the original
22:20:45.868428 UTC ceiling.

### Inventory execution and validator failure

Root activated the reviewed inventory at **21:57:17.423705 UTC**, session
**88372**. The unchanged full `test_dis` executed 147 cases and returned six
assertion failures: `test_disassemble_str` and `test_loop_quicken` in each of
the two disassembly classes, `InstructionTests.test_jumpy`, and
`TestFinderMethods.test_findlabels`. No golden or runtime source was edited.
The host validator then failed while expecting an absent `suite-result`
phase, before the five-string fixture child ran. Root confirmed terminal exit
one; cleanup at 21:57:33 UTC records no live target process. Both native
assertions and validator failure are preserved under
`build/constant-operands-golden-inventory-v1/`, with outer receipts under
`build/constant-operands-feedback-remaining-v1/golden-inventory-v1.*`.

This is diagnostic evidence, not a successful inventory or compatibility pass.
Read-only host replay and individual causal classification are next; the full
module will not be rerun merely to repair evidence parsing. The original
22:20:45.868428 UTC feedback deadline remains unchanged.

### Separate fixture capture and golden-only source

Read-only host replay of the original inventory crosschecks returned stats,
147 unique verbose case IDs and six failures; it preserves the missing outer
result token. A failed unittest suite raises `TestFailedWithDetails` before
the observer can emit that token. The original inventory remains failed.

Independent [fixture preparation review](reviews/golden-fixtures-preparation-20261031-v1.md)
accepted the separate compile-only probe. Root activated it at
**22:03:24.769393 UTC**, session **76380**, and confirmed terminal exit zero.
Completion is 22:03:41.201175 UTC; result SHA-256
`854151f54db7001a77d8f4bc7983bc573a0c53ca8383b409d64458d18c5328bc`.
It records 29 collections, zero fallback/unpinned events and idle cleanup.
No fixture workload or `jumpy` function was called.

Only expression, simple-assignment and compound-assignment strings changed:
each removes LOAD_SMALL_INT1 and uses RHS literal1. Both annotation strings
are identical. Live `jumpy` labels equal its live jump targets. Its 118 versus
119 instructions reflect one removed load and one immediate subtraction;
downstream offsets and eight jump arguments explain the remaining differences.
The existing full-module raw diff separately records loop-quicken literal3
and backedge7-to6 changes.

Root authorized only those expected outputs. Test-only
`c76bc5b5a9b4c4ebe546441b5cd416c3c217343a` froze at 22:05:26 UTC, changing
five assignments in `Lib/test/test_dis.py`; all test bodies/readers remain.
Exact source review and a two-test-file bridge to the original debug image
precede a remaining-only batch. No failed receipt is rewritten and no complete
compatibility, release, emission or performance result is credited yet.

### Remaining-v2 execution and monitoring stop

Independent [source/output review](reviews/golden-correction-source-outcome-20261031-v1.md)
accepts c76 for further feedback. Root's [helper review](reviews/remaining-v2-helpers-20261031-v1.md)
replayed 144 inputs, the exact two-file source bridge and all 588 debug products.
Root activated the seven remaining rows at **22:08:21.858606 UTC**, session
**85999**, retaining the original start/deadline. No old image was relabeled.

Disassembly passed all 147 cases; code objects passed 47 with one expected
skip; tracing passed 442 with one expected skip. All three terminal validators
and idle cleanup passed. Monitoring then stopped after 54 cases with one
failure in `TestBranchAndJumpEvents.test_while_offset_consistency`: four
branch-left3-to5 events match, but the final branch-right3-to8 differs from the
hardcoded3-to9 expectation. Actual target return2 and batch failure are retained.
Root confirmed session85999 terminal exit1; cleanup at 22:09:02.709196 UTC
records no live target process. Release build, release cases and emission did
not run.

The loop contains `n += 1`; removal of its literal producer is a concrete
offset-shift hypothesis. A small exact-original-CodeType observation is being
prepared without calling the method or nested function. No monitoring oracle
or runtime change is authorized merely by this hypothesis. Deadline remains
**22:20:45.868428 UTC**.
