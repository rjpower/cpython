# Independent discard-completion code review

2026-11-09 UTC. Reviewed the actual `boundary-ir-20261108` working diff against
golden `0765e9be47f`, including shared handler descriptors, generated exact and
general call bodies, recipient helper, transaction commits, f_lineno setter,
metadata and focused tests. No native build or timing by this reviewer.

## Source disposition

No blocking ownership or control-flow finding remains in this source pass.
Release/debug compilation, focused runtime tests, combined regeneration and
compiled/net-performance inspection remain gates before integration.

- The ordinary return takes one range branch and directly uses the shared slot
  publisher. It does not run a tag test followed by the previous no-destination
  test. The tag/IR_NO_DST cases are separate. This is a source cost assessment;
  GCC may still change common handler layout and transaction initialization.
- Exact entry sets the tag after unchanged guards and child preparation. General
  entry keeps the numeric destination through preparation and native fallback;
  only successful child commits install the tag. The actual generated terminal
  adapter receives the constant flag. There is no after-effect deopt/replay.
- Completed direct disposal clears the tag, validates the live POP handler and
  its input against the saved temporary slot, then releases the owned result.
  It writes no result home. Instrumented/declined completion publishes the home
  and preserves the actual existing continuation. The result remains owned in
  C across child retirement; no duplicate or missing GC edge is introduced.
- Selection checks matching canonical POP input, temporary destination, source
  line equality and synchronous caller/callee code. Generator-owned callers are
  excluded, so yield-from and remote awaited-slot readers cannot receive a tag.
  Existing argument, frame and ordinary exception cleanup stays intact.
- Successful f_lineno writes cancel a pending tag before changing the PC.
  A failed setter does not change the tag. This permits the existing return
  continuation to honor its current coordinate without a new saved-PC field.
- Canonical opcode/physical-handler domains remain distinct. Keep the operand
  increment's old/new feedback normalization and adaptive-shadow repair when
  merging this work; the boundary worktree does not independently include that
  whole integrated runtime patch.

Focused tests cover both entry routes, mutable guard failure, result liveness,
child exceptions, generator exclusions, cleanup-activated opcode observation,
local monitoring callback errors, pre-completion parent f_lineno mutation and
negative descriptors. A requested stable-maker inline-result test now covers
None, bool, int and float on both warmed aliases. Actual runtime discovery and
execution remain unverified here.

## Result-finalizer f_lineno question

CALL PC during ordinary disposal is an explicit implementation-coordinate
choice. Frame homes are already valid in this state during child retirement;
a result finalizer can inspect them without requiring a dormant POP PC. Exact
CPython decrement timing is not a Python language mandate.

The helper currently keeps its saved continuation if the result finalizer
starts tracing and a trace callback writes a different parent frame's f_lineno.
Golden POP_TOP likewise dispatches its saved next_instr after XCLOSE. Source
inspection confirms the setter's event check is thread-wide and can permit
such a write, but the documented API limits f_lineno jumps to the bottom-most
frame. See the [official frame-object reference](https://docs.python.org/3/reference/datamodel.html#frame.f_lineno).
Consequently this cross-parent write during finalizer execution is an incidental
extension, not a new supported jump guarantee. Ordinary bottom-most caller
observation takes the materialized POP path before its event callback.

Do not add a hot post-close PC reload merely to support that extension. A naive
`new instr_ptr + 1` is especially unsafe after the result has already been
retired: it can skip the target's producer without a result home. A supported
extension would need its own reconstruction/resume contract. Retain the cheap
pending-tag cancellation for accepted writes before completion, and document
the narrower post-close boundary. No new owner/GC safety defect was found in
the unchanged-continuation policy.

A matched golden/candidate probe can record the accepted setter write and final
returned branch after a warmed discarded Result.__del__ enables tracing, then
its LINE trace writes the saved caller's f_lineno to another live return line.
Keep such a probe diagnostic; it must not silently establish a new compatibility
mandate for cross-frame jumping or immediate finalization.

## Operand integration evidence checked

Coordinator commit `f2c5ac3052b` contains the reviewed operand source and repairs.
The retained `operand-v3-release-focus.log` and `operand-v3-debug-focus.log`
each report all six modules passed, 465 tests, SUCCESS. Stats build/smoke is
reported separately as 142 tests. The integration page correctly calls this
focused qualification and keeps broad/full-suite and performance admission
pending. Earlier failed runner, adaptive-view, fixture and history-test results
remain retained; they are not relabeled as passes. The source-only repairs did
not change the compiled C/generated handler bytes inspected earlier.

Follow-up evidence read: `operand-v3-stats-smoke.log` directly reports both
modules passed, 142 tests, SUCCESS. The first operand elapsed screen is not a
logging win; the compiled instruction result must remain separate from it.
