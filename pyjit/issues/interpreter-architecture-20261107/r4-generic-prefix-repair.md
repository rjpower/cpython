# R4 generic-reader prefix repair

2026-11-07. Follow-up review round for the R4 result-publication contract.
The earlier source review remains preserved in
[contract-implementation-review.md](contract-implementation-review.md); a new
cold probe found an uncovered prefix case and required a bounded repair before
commit. This follow-up does not change the runtime contract or generated C
behavior.

## Finding

The generic `IR_COMPLETE_ATTR_READER` checker validated the helper call,
receiver `TAKE()` initializer and direct result assignment, but did not inspect
all statements before the helper. A producer could close `receiver` or `pending`
and then continue to the helper, causing a second close or publication from a
consumed input. The real `_LOAD_ATTR` producer has one valid exception: its
flat null-result branch closes the receiver and terminates with
`IR_ABORT_VALUE_READER()`; it never reaches completion.

The explicit result checker also accepted a single top-level statement whose
left-hand side assignment was embedded in a conditional expression, and it did
not reject a comma-expression that consumed the assigned result before the
publication marker.

## Repair boundary

The generator now rejects known protected-input consumption, mutation, or
address-taking on any path that can continue to generic completion. It allows
only a flat `if` block with no `else`, simple cleanup statements, and an exact
terminal `IR_ABORT_VALUE_READER()` call. The recognized input consumers are the
existing view release/take operations, `DEAD`, and
`value_operation_close3`; this is a bounded DSL/API check, not arbitrary C
ownership inference. Loops and unrecognized control statements mentioning a
protected input are rejected conservatively. The receiver initializer remains
the checked top-level `TAKE()` assignment.

The explicit output assignment must now be a direct top-level assignment, and
its statement must name the result only once. This rejects nested conditional
assignments and comma-expression consumption without adding C provenance or
ownership analysis.

## Validation state

The focused test patch covers both valid terminal abort cleanup and invalid
generic prefixes (receiver/pending/pending-self release, the reader-close
helper, view take, conditional and loop-header consumption, member/compound
mutation and address escape). It also covers conditional-expression and
comma-expression result assignments. The address case is rejected by the
existing handler-local-address guard before this new prefix checker. R4 landed
at `b58036e9718`; the repaired release and debug generator/contract suites
passed 97 tests per profile, and regenerated runtime products were identical.
The original 1,100-test focused runs and 306-unit lint are pre-repair results.
The final basic/cold receipt was still in progress when this note was updated;
this repair makes no performance claim.
