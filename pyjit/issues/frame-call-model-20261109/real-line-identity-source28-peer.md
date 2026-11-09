# Real-LINE identity after wider source28 evidence

Proposal peer only; runtime remains frozen pending coordinator admission.

The original `JumpStackModelTestCase.test_jump_to_running_line` calls
`frame.f_lineno = frame.f_lineno` at every LINE event. The broader source28 run
hung. The new PRE cancellation capability can admit an earlier real word on the
same line while the current real LINE wrapper is at a later expression operation.
That restarts part of the expression instead of preserving the observed phase.
The earlier first-word-only identity restriction therefore no longer composes
safely with the newly admitted cancellation recipe.

Approve the bounded identity correction: retain exact observed frame, actual
current `INSTRUMENTED_LINE`, no source-event/error cursor, no pending child,
non-suspended state and generation overflow checks. Require the resolved requested
line to equal `PyCode_Addr2Line(code, real_current)`. Preserve current PC, every
owner, handled-exception context and binding; commit redirect generation and
existing identity continuation/cache state. Do not require current to be the first
real word bearing that line. No target availability, restoration or earlier-word
restart is needed because this operation remains in the actual current phase.

Actual LINE wrapper source calls the underlying handler once when PC is unchanged;
this identity does not dispatch the wrapper repeatedly. The source-only transaction
remains distinct: its predecessor/cursor is explicitly excluded, so the existing
same-PC source replay and setter-then-raise attribution tests keep their semantics.
A request for another line still goes through the existing full plan. Identity is
not permission to reconstruct a partial expression at another same-line word.

Preserve the original exhaustive test. Add a bounded event-cap witness that pins
a later real LINE on an expression/comprehension line and proves PC/local/owner
identity with and without setter, including active saved-local state. Keep the
protected identity tests and source-generation regressions. This review approves
the source rule, not a native outcome; the coordinator owns probes and validation.
The earlier failed round and its conservative first-word gate remain historical
evidence rather than being silently rewritten.

## Actual source disposition

The minimal C correction is source-approved: it removes only the first-word scan
and predicate. Actual observed frame, handler, source/error, pending, suspension,
line and overflow gates remain. The new bounded manager fixture covers cold and
20-call warmed code, caps LINE events, requires call/enter/body/exit exactly once,
and asserts every successful setter preserves its PC. The original exhaustive
running-line test is unchanged. I requested that its two distinct header PCs be
counted among successful setters, to prove the later-word identity route rather
than only observing two stops. Native results remain pending.

The requested fixture strengthening is present: a separate applied list contains
only successful setters, and its header-PC set must contain at least two distinct
PCs. Actual source/test peer is approved and frozen for coordinator rebuild.

## Coordinator result post-review

After this correction, release and debug both build with 117 checked modules and
pass 125 focused tests (one skip). The capped 16-row running-line probe completes
correctly in both profiles, and the unchanged original running-line test and new
bounded manager test pass. This resolves the wider-run hang without adding an
instruction or restarting an earlier expression operation.

The wider 999-test runs now complete but are not clean. Reported sys.settrace
errors fall from 65 to 38, with 22 failures: 21 diagnostic-string mismatches and
the separately reviewed obsolete stack-capacity refusal test. Pdb retains two
failures and two errors, down from six failures/two errors. These are retained
failures, not a final Python-behavior qualification. Saved-local restoration,
POST provider repair and suspended/caught-exit contexts remain outside this
increment. No native work was performed by this peer.
