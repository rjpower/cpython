# Actual protected-context inventory at source23

Coordinator receipts are E11 `ownership23-{release,debug}-debugger-context-inventory.{json,log}`.
The release metadata receipt identifies ELF
`668693169bfe07b35b153341cbebd7dcb833cf9ceb741d93eecea47b5b955a5c`.
The probe is compile-only: it records physical words, slots, source events and
EH tuples; it does not demonstrate a successful debugger transfer. Both logs
contain nine fixtures, including the bare-reraise addition. No worker native
execution was used.

## Ordinary try has no additional owned context

`ordinary_try` has divisor0/value1, nlocalsplus2. PC2 loads10 into home2;
PC3 divides it by local0 and stores value1. PC4/5 increment value1, and PC6
returns it. The protected interval [2,6) enters handler7 with exception home2.

Handler7 is `PUSH_EXC_INFO A2 B2`: its **actual** outputs are previous handled
exception in2 and the new exception in3. Normal matched exit at PC13 consumes
saved home2 through POP_EXCEPT. The unmatched path reraises exception3 at16;
cleanup17 COPY produces a distinct saved-previous owner in5, POP_EXCEPT5 at18,
then RERAISE at19. COPY cannot be modeled as a fresh unrelated exception.

For ordinary try-body to try-body/outside transfers, there is no active caught
owner to restore. Selecting a different EH interval is not by itself owner
creation. The global nonempty-exception-table guard therefore rejects more than
necessary. It cannot simply be removed: the same code contains real active
caught contexts at its handler sites.

## Caught exception and its named binding are separate state

`caught_exception`: divisor0/value1/error2, nlocalsplus3. Handler5 PUSH_EXC_INFO
uses previous-exception home3 and new exception4. STORE_FAST at9 moves4 to named
error2. PC10/11 only write value1. POP_EXCEPT3 at12 restores the previous handled
exception; PC13/14 clear/delete error2; PC15 loads value1 for return.

A same-real-PC identity inside PC10/11 preserves every home and exception link.
A nonidentity move inside that same caught context must preserve the saved
previous owner3 and the active exception identity, even when normal live values
are only named locals. Exiting the handler requires an explicit saved-exception
owner transfer and a reviewed policy for the exception-as binding cleanup.
The cleanup suffix duplicates saved owner3 via COPY into6 before POP_EXCEPT6;
the real slot map, not frontend output counts, must drive provenance.

`bare_reraise_context` makes the missing distinction sharp. PC4 PUSH_EXC_INFO
establishes previous home2/new exception3; after matching, PC9 is RAISE_VARARGS0.
A bare raise has no explicit exception input but depends on the active caught
exception link. An empty normal-operand bitmap is not sufficient authorization
to jump into it. This negative must accompany any removal of the global guard.

## Finally has distinct normal and exceptional copies

`try_finally` normal finally code is PC4/5 with ordinary inherited exception
context, followed by RETURN6. Its exceptional copy starts PUSH_EXC_INFO at7,
keeps previous owner2 and propagating exception3, executes body at8–10, then
RERAISE3 at11. Both copies have the same source line6. Picking the first physical
word for a source line is not a context-selection rule.

`caught_finally` further reuses **home4** for two different context producers:
PUSH_EXC_INFO6 is the matched except context; PUSH_EXC_INFO24 is the exceptional
finally context. The normal finally body is PC4 and has no newly established
caught context. Equal physical owner homes do not establish context identity.
Constant home3 is part of nlocalsplus4, independently of those saved owners.

## With owns an exit callable/optional self

`with_owner`: manager0/item1/value2, nlocalsplus3. LOAD_SPECIAL at3 obtains exit
method/self and SWAP4/5 place them in3/4. The enter call at7 uses separate window5
and publishes item1. The body is PC8–10. Normal exit CALL14 consumes window3,
whose remaining slots5–7 are None arguments; result is discarded.

The exceptional path PUSH_EXC_INFO16 uses previous home6/new exception7, with
lasti5 and exit callable/self3/4 preserved. WITH_EXCEPT_START17 reads window3.
It is wrong to identify with ownership from the EH floor alone or to treat all
five window positions as fresh outputs. A body jump out needs explicit exit
owner retirement under the existing debugger policy, not an invented __exit__
invocation. Entry from outside lacks real owners3/4 and must refuse.

## Comprehension restores an actual saved owner

`comprehension_restore`: values0/marker1/result2, nlocalsplus3. LOAD_FAST_AND_CLEAR
at3 saves old marker1 into4. SWAP4 moves that saved owner to3. BUILD_LIST5 and
SWAP6 put list4 and iterable5 above it. GET_ITER7 establishes pair5/6; FOR_ITER8
publishes each item directly to marker1. LIST_APPEND9 uses list4 and local1,
while saved owner3 is an untouched hole.

Normal completion stores list4 to result2 at11, then saved owner3 to marker1 at12.
The error handler [17..21] moves exception5 through SWAPs, retires list5 and
restores the saved marker through home4 at20 before reraising exception3.
Therefore a debugger exit from the body must restore saved home3 to local1;
merely dropping scratch would leave the loop item in marker1. A cell-valued
saved binding must restore the cell owner itself, as codegen already specifies.

## Yield observation precedes actual suspension

`yield_resume`: value0, nlocalsplus1. PC3 borrows local0 into outgoing home1;
YIELD4 has A=NoDst/B1; RESUME5 is its next activation guard. The sent value is
intentionally discarded. The enclosing PEP479 handler keeps lasti1/exception2;
CALL_INTRINSIC8 converts StopIteration and RERAISE9 propagates it.

`caught_yield`: nlocalsplus0. PUSH_EXC_INFO6 saves previous handled exception0,
new exception1 is consumed after matching, and outgoing value1 is produced at11.
YIELD12 has A=NoDst/B1; RESUME13 precedes POP_EXCEPT0 at14. Its generator
exc_state holds the active exception across suspension; slot0 is still the saved
previous owner needed when that context exits.

The existing PY_YIELD observer is `_YIELD_VALUE_EVENT` **before** `_YIELD_VALUE`
marks the frame suspended, publishes the resume recipient and unlinks exception
state. A successful redirect in that callback dispatches the selected body now,
cancelling the pending yield. The existing test_jump_from_yield's [2,2,5] result
requires that behavior. Model YIELD_PRE, SUSPENDED_AFTER_YIELD and
PY_RESUME_ENTERED separately; do not implement all three by changing instr_ptr
and guessing a resume recipient.

## Exception restoration inside a callback

`probe-debugger-caught-callback.py` is ready for coordinator execution. It puts a
callback-owned LookupError over a target-owned ZeroDivisionError, then attempts
to leave the target handler. Source evidence already establishes why a direct
write to tstate->exc_info is insufficient: callback PUSH_EXC_INFO may have saved
the target exception in its own previous-owner cell, and POP_EXCEPT later
restores that owner. A correct transfer must leave callback sys.exception()
unchanged and prevent its return from resurrecting the target's old context.

A reviewed implementation must resolve the target's actual exception link
through intervening callback/activation/generator contexts, or keep caught-context
exits deferred until such a link is available. It must not inspect or mutate a
live child overlap to find that link. Same-context identity needs no restoration
and can remain the independent first extension.
