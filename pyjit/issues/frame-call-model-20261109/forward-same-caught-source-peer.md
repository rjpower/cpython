# Forward same-caught source peer

The two-file N diff in `Objects/frameobject.c` and `Lib/test/test_code.py` is
**accepted for root-owned native qualification**. The shared slice sets
`first=target, stop=current` for the old backward case, preserving its
dominance walk and owner/context/error-floor cuts. The new forward case sets
`first=current, stop=target` and rejects every induced edge unless it is a
normal edge (`phase >= 0`, `floor < 0`). It still rejects a backedge or
preserved-root read, definition or child overlap. Plan admission requires a
fresh exact-PC PRE phase, both save vectors empty, the same positive caught
context, equal positive origin labels at the same homes, actual exception/None
values and target ALL demands contained in the preserved-root set. The
existing commit transaction and backward recipe remain in use.

The positive fixture skips `value += 2`, executes `value += 4`, and expects
result 5 while `before`, `after` and the caught ZeroDivisionError are the same
object. It performs the setter inside a distinct handled LookupError and
checks that callback and outer exception identities are restored. The negative
fixture requests a forward loop-body landing before its iterator roots exist,
then checks refusal, unchanged PC/local/exception state and normal result 3.
Its source assertion only establishes that a FOR_ITER word exists; root's
compile/IR receipt should pin the exact target owner home before describing
this as an owner-specific witness.

Host AST and diff checks are source evidence only. The unchanged original
within-except case, new controls and existing backward/NULL PRE gates still
need root-owned native execution. No finally payload or between-arm permission
is added.

## Post-integration check

The final committed source is `d7abf803c607b0de3cec58d7e91ba91ff1699792`
and changes only `Objects/frameobject.c` and `Lib/test/test_code.py`. The
negative fixture now derives one actual scratch-backed `FOR_ITER`, checks its
target line lies within that loop's body before the exit, and identifies the
backedge to the same FOR word. This removes the earlier weak any-FOR claim.
The forward slice still rejects exceptional edges and uses the same caught
owner, save, binding and Plan gates reviewed above; the backward path remains
the existing dominance proof.

Root's release and debug builds checked 117 modules each. The focused
`test_code` plus `test_ir_contract` run passed 136 tests with one skip in each
profile; the original within-except and selected new positive/refusal methods
also passed in both profiles after fixture strengthening. Committed identity
rebuilds passed with a clean source checkout. Evidence is retained under
`E11/forward37-*`. This is a bounded source and focused native acceptance,
not full release/debug qualification or a performance result.
