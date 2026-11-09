# NULL-only debugger requirements: bounded implementation interface

This is a read-only design checkpoint. It does not authorize a runtime edit or
claim a passing test. The first target is the three `test_setting_lineno_*`
peepholer refusals in `ownership31-remaining-phases.json`. Their line-9 stop is
an ordinary `INSTRUMENTED_LINE` PRE state; no SOURCE_POST authority is needed.

## Shared physical fact

Add one cold query for an encoded word, actual argument, normal edge, scratch
home, and frame bounds. Its answer is `PRESERVE`, `SET_NULL`, `UNKNOWN`, or
`TRANSPORT(source_home)`. `TRANSPORT` reads the entire old row before any write.
The answer describes only whether a physical home must contain NULL. It never
creates an owner, restores an old input origin, or certifies an observer phase.

The initial exact cases are small: `COPY` transports B to B+oparg, `SWAP`
exchanges B and B+oparg-1, `STORE_FAST` sets its consumed scratch A to NULL,
and FOR completion sets B and B+1 to NULL. Other actual output homes and
consumed inputs become `UNKNOWN` unless their success-side clearing is proved
from the emitted store/close sequence. Untouched homes preserve. An opaque
child overlap forgets the affected scratch tail. Malformed geometry and
unsupported transports forget rather than preserve. Existing
`_PyIR_ObservedHomes.old_cut` remains the separate provenance fact.

For an ordinary exception edge, start from PRE and forget at least every home
the failing word might touch before unwind; forgetting all scratch is a safe
first implementation. Exception payloads are `UNKNOWN`. A certified source
POST error starts from the selected successful edge, then applies its payload
writes. The first three witnesses have no EH or source records, so this rung
needs no precise partial-error publication claim.

## Consumer

Replace the whole-code `null_home` exclusion in `register_resume_liveness()`
with a request-owned per-PC forward *must-NULL* bitset on the existing sparse
graph. Entry scratch starts NULL except constant homes; ordinary function entry
is the first admitted case. Joins intersect reachable predecessors. No fresh
entry assumption is made for generator resume, pending-child overlap, or
external entry. Bound memory and checked products like the existing owner
analysis.

At each actual read, use `_PyIR_RealHandler` and `_PyIR_NullableHome` across all
aliases. A read is `NULL_ONLY` only if its original incoming forward fact is
must-NULL **and** every executable alias permits NULL; otherwise it requires
`VALUE`. Keep these as separate incoming bitsets in the existing backward
owner walk. At one read, a required alias makes the whole read require VALUE.
At a control-flow join, retain both bits: if VALUE and NULL_ONLY reach the
same incoming home, refuse the redirect unless a later proof discharges one.
An unrelated non-NULL owner cannot satisfy an original NULL expectation.
COPY/SWAP map demands to their old source homes. A successful guaranteed
definition discharges its incoming demand; an ordinary PRE error does not.
Named-local binding checks remain independent of this ABI scratch-home proof.

The Plan may omit an owner for a target's NULL_ONLY home only when the selected
stopped view also proves must-NULL and the actual current cell is NULL. Recheck
both facts and the actual executable handler after warning callbacks or
monitoring changes. A NULL optional self at generic CALL does not authorize a
missing method receiver on a path whose original read required VALUE.

The encoded peepholer path shows why a global ever-written test is too coarse:
`COPY` writes scratch 6, then `STORE_FAST` clears 6; the later FOR exhaustion
clears cursor 6 again. The line-3 target reaches a CALL that accepts a NULL
optional self in 6. The forward fact proves the original expectation, and the
stopped-view/physical checks prove the proposed replacement. A required alias,
specialized nonnullable call, active saved-local token, or EH path with lost
must-NULL knowledge must still refuse.

The first implementation should change only the shared cold query and the
existing liveness/Plan consumers, with positive versions of the three pinned
cases and negatives for required method receiver, aliasing, EH, and changed
runtime handler. It must not add an opcode-name admission table, a second
unwind engine, or a hot-dispatch transfer program.
