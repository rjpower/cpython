# Remaining debugger cases: semantic scope audit

Proposal only; no test/runtime changes or native execution by B. The case list
comes from ownership30-wide-release.log's 24 JumpTestCase errors and four Pdb
outcomes, checked against current test bodies and decorators. These are retained
failure receipts, not a new Native32 rerun of those modules. Native32's full
monitoring pass is separate evidence.

## Contract used

Doc/reference/datamodel.rst describes a trace-function write to the bottom-most
frame's f_lineno as a source jump. Doc/library/sys.rst places a line event before
the new line executes. Doc/library/pdb.rst allows skipping/repeating code but
explicitly says not all jumps are allowed, with entering a for body and leaving
a finally clause as examples. It does not promise CPython's entire accepted
cross-context matrix.

Accordingly, ordinary code replay within the same active protocol remains a
capability we owe. It cannot be replaced by a generic refusal merely to clear
a test. Skipping a required context exit, requiring absent handler/manager roots
or bindings, cancelling a yield transaction, or replacing the handled exception
link is a separate debugger feature. Conservative refusal is a
reasonable proposed contract for those crossings, provided it preserves the
whole stopped state and normal subsequent execution. This is not permission
to weaken safety assertions or qualify all refused jumps.

O below means an ordinary positive to preserve/repair. R means a proposed
intentional refusal requiring stronger state tests and independent admission.
U means unresolved until actual context/owner/binding proof is inspected.
The source intent does not identify the physical stopped phase: future probes
must still distinguish actual LINE/PRE, committed branch, source cursor and
child overlap. In particular, neither a coroutine function flag nor an async
source spelling proves that an observed executing frame is suspended.

## All 24 trace methods

| Actual method | Relative request | Class and reason |
| --- | --- | --- |
| test_jump_across_with | 2→4 | O: skip the first not-yet-entered with header and execute the second header normally. Do not manufacture or reuse a manager. Verify the event really precedes the first enter. |
| test_jump_across_async_with | 2→4 | O: same pre-entry skip while the coroutine is executing, before an active await/manager transaction. |
| test_jump_backward_over_listcomp | 3→1 | O: completed comprehension to earlier ordinary assignments; all save obligations should already be discharged. |
| test_jump_backward_over_async_listcomp | 3→1 | O: completed async comprehension to earlier assignments, at an executing line boundary. |
| test_jump_backward_over_async_listcomp_v2 | 8→2 | O: replay after completed comprehension; preserve the named flag and original result sequence. |
| test_jump_back_over_comprehensions | 5→2 | O: completed list/dict comprehensions to their earlier setup; no active binding restoration is requested. |
| test_jump_within_except_block | 5→6 | O: same handled exception and saved-previous chain, skipping one ordinary statement. Forward same-context survival must be proved, not blocked by a backward-only recipe. |
| test_jump_in_nested_finally_2 | 6→7 | O: stay inside the currently executing finally and preserve the original ZeroDivisionError for its eventual re-raise. |
| test_jump_in_nested_finally_3 | 6→11 | O: stay inside that same finally, skipping a nested try/finally which has not been entered. Preserve the outer exception; do not skip its unwind. |
| test_jump_out_of_async_for_block_forwards | 4→5 | O: abandon the inner iterator at a body line and retain the outer ordinary FOR. No awaited child is active at this source point. This needs retained-root proof, not a fabricated iterator. |
| test_jump_out_of_async_for_block_backwards | 5→2 | O: same inner-loop abandonment and outer-FOR retention before replaying its body. The existing clean_asynciter wrapper retains/acloses its generators. |
| test_jump_out_of_with_block_within_for_block | 4→5 | R: old expectation omits the active inner manager's exit while keeping the outer loop. |
| test_jump_out_of_async_with_block_within_for_block | 4→5 | R: skips the active inner asynchronous exit rather than executing its await protocol. |
| test_jump_out_of_with_block_within_with_block | 4→5 | R: skips inner exit while retaining the outer manager. |
| test_jump_out_of_async_with_block_within_with_block | 4→5 | R: skips inner async exit while retaining the outer manager. |
| test_jump_between_with_blocks | 3→5 | R: enters the second body without its enter and later exits the first manager; the old expected log explicitly reuses the wrong lexical manager. |
| test_jump_between_async_with_blocks | 3→5 | R: same lexical-manager reuse with asynchronous protocols. |
| test_jump_between_except_blocks | 5→7 | U: arms can share the same caught generation and saved-previous owners. If the target's actual requirements are compatible, this is an ordinary gap. Do not invent a lexical-arm ban. |
| test_jump_between_except_blocks_2 | 5→7 | U: same proof question, additionally inspect the actual target exception-name binding requirements. A missing required binding can refuse; an unused arm label cannot. |
| test_jump_from_except_to_finally | 5→7 | R: requires leaving the caught context and installing the proper enclosing finally context through the actual restoration link. |
| test_jump_out_of_bare_except_block | 7→1 | R: exits an active caught context before replaying the try; current callback-owned exception links must not be overwritten. |
| test_jump_out_of_qualified_except_block | 7→1 | R: same caught exit requirement. |
| test_jump_out_of_complex_nested_blocks | 8→11 | R: skips the pending finally output10 as well as inner loop protocols. Mere operand retirement cannot implement that semantic crossing. |
| test_jump_from_yield | return-event 3→2 | R: cancellation of the generator's yield transaction, not an ordinary real-LINE jump. Needs a separate yield-ready owner/continuation contract. |

This is 11 ordinary positives, 11 proposed crossing refusals and two unresolved
same-caught arm cases. The initial source-only 11/13 split was corrected after
A's peer identified shared caught setup across arms; preserving that count is
not a contract. All retained
errors are setter ValueErrors; these receipts alone establish no remaining
LINE/source event-generation bug. They also do not prove which particular
owner fact blocks each ordinary positive. The three completed-comprehension
NULL cases and the current NULL-only proposal are useful witnesses, not a
licence to label every refusal the same defect.

## Four Pdb outcomes, two logical cases

| Method and duplicated backends | Classification |
| --- | --- |
| test_pdb_basic_commands: two errors | O: jump8 skips an unexecuted second loop and continues at print('after for'). It must not run never_executed. This is an ordinary positive; diagnose the actual phase/NULL or root requirement rather than changing its body/output expectation. |
| test_pdb_issue_gh_127321: two transcript failures | Presentation difference to review, not an owner or source-event failure. Current entry shows the real call line3; the old transcript expects next line4. Since3.13 set_trace is documented to enter immediately. A proposed transcript correction must assert that the displayed call line has a valid position, interaction occurs once, and continuing executes a=1 once; it must not just accept either line. |

The second row does not fit a conservative-refusal category: no jump was
requested. Calling it an event bug from the old transcript alone would hide
the immediate-entry contract. Any change still needs its own focused peer and
admission, with both backends retaining the same behavior assertions.

## Stronger tests for proposed intentional refusals

Catch the expected ValueError inside the actual callback and then continue the
original program. Check exact logical PC/line, locals by identity and existing
manager/iterator owner witnesses immediately before and after refusal. The
failed setter must not issue a target binding warning, invoke target protocol
enter/exit, or retire its target/protocol owners. This is not a promise that
unrelated allocation-triggered GC or finalizers cannot run. Tests must not
substitute a broad exception assertion or skip execution.

- **With crossings:** require unchanged active manager identity and unchanged
  enter/exit log immediately after refusal. Normal continuation must execute
  each original body and matching exit once. For inner-with/outer-for use
  [1,3,4,-3,5,6]; for nested managers use [1,2,3,4,-3,5,-2,6]; for two sequential
  managers use [1,2,3,-2,4,5,-4]. Async variants additionally prove each original
  exit is awaited once and the outer protocol remains live.
- **Caught exits:** have the target hold its actual exception and
  the callback hold a distinct LookupError. After the caught setter refusal,
  sys.exception() in the callback must still be that LookupError; after callback
  return, the target must retain its original exception. The except→finally
  case must finish with [4,5,7,8], and replay-refused bare/qualified cases with
  [1,3,6,7]. The two unresolved arm cases retain their original positive tests
  pending proof census; if compatible, support them through the common
  same-context proof. If actual roots/bindings are missing, a separately
  admitted refusal control must preserve normal output[4,5,8] and must not
  manufacture e. No per-method lexical-arm exclusion is proposed.
- **Pending finally crossing:** require the original exception/outer iterator
  roots unchanged on refusal, then run both inner iterations and both finally
  clauses. The original complex program must produce
  [1,3,5,8,8,10,5,8,8,10,11,12].
- **Yield cancellation:** reject without changing the yield payload, PC,
  recipient or active exception. A controlled fixture must still yield3,
  resume normally and finish, with its original output[2,5]. Do not confuse a
  return tracing callback before suspension with an already suspended frame.

These are proposed strengthened tests, not already executed receipts. Ordinary
O positives keep their existing output/warning/exception assertions. The next
structural capabilities are pathwise NULL-only incoming requirements, general
same-protocol retained roots (including forward same-caught/finally survival),
and separately reviewed yield/context-exit actions. No per-test setter branch
or test rewrite is admitted by this audit.

## Ordinary gaps: probes before attribution

| Shared question | Cases | Required next actual-state witness |
| --- | --- | --- |
| Pathwise NULL-only requirements | Four completed-comprehension replays; Pdb basic jump8 | Record the stopped physical PC/handler, requested line's real candidate PCs and actual optional-input aliases. Show original-path must-NULL and stopped physical NULL for the needed homes, plus zero active save columns. A similar source shape is not proof of the same NULL home. |
| Same-protocol owner survival | Same-except, both finally-internal moves and the two unresolved except-arm cases | Record source/target encoded PUSH/POP/save lineage, actual binding requirements and current inherited exception identity before a callback introduces its own exception. Prove the same generation survives; include forward targets and the original exception's eventual re-raise. Do not infer a different generation from different lexical arms. |
| Retained outer loop roots | Both async-for exits | Record actual body stop, selected target, outer iterator/cursor homes and child absence. Preserve the outer iteration identity while retiring only the inner iterator; show ordinary async cleanup remains correct. |
| Pre-entry cancellation versus source publication | Both across-with-header skips | Record callback event, physical handler/PC, source-map matches and manager enter log. If the first manager has already entered before the advertised new-header LINE, investigate that event ordering; do not call its active exit an unstarted setup. Otherwise cancel preparation and enter the second manager normally. |

These probes must retain the public callback name separately from actual owner
site and scoped phase; a legacy LINE can be delivered by a committed branch.
Use existing read-only IR/position inspection and bounded event caps. If those
facts cannot distinguish a scope, ask for a coordinator-owned phase witness
rather than infer it from a line ordinal. This audit requests no runtime
instrumentation during the current performance freeze.
