# Coroutine NULL facts need an explicit first-resume ingress

Proposal only; no runtime change or native result is claimed. Current
`register_resume_analyze` skips the optional NULL matrix for every generator,
coroutine and async generator. That is conservative, but it classifies a
physically NULL optional input as VALUE in an otherwise eligible real-LINE
PRE request. The stopped-phase, context, saved-local and actual-home gates
should remain unchanged.

## Source and request geometry

The constructor requires `RETURN_GENERATOR`'s encoded destination to be
`IR_NO_DST`. The three recorded async code objects start with that word at
pc 0 and `RESUME` at pc 1. `RETURN_GENERATOR` copies the initialized frame to
`gi_iframe`, clears the old cells, sets the heap frame's return slot from that
NoDst field, then returns the coroutine object to its caller. A successful
first `send(None)` enters the heap frame. The native sender converts a NULL
argument to `None` and calls `_PyFrame_SetReturnValue`; VM `SEND_GEN` also
checks a CREATED receiver's sent value is `None` before entering and uses the
same setter. Because the created frame's return slot is NoDst, this first
value is discarded. It does **not** define a scratch owner. Thus pc 1 has
the initialized scratch NULL pattern, except immutable constant homes.

`register_resume_successors` currently emits pc 0→1 as a generic fallthrough.
For a coroutine this is a lifecycle gap, not execution of the next word in
the creation call. The NULL matrix must seed pc 1 as an explicit *successful
first-send* ingress; it must not inherit facts by treating every future
resume as a fresh pc 0 entry. The existing graph can still carry normal,
ordinary-error and source-error edges. Its `YIELD_VALUE` successor represents
a later send: the outgoing value is taken and a future input is written to
the encoded return slot. That destination is UNKNOWN; unaffected scratch
homes may survive only under the existing exact physical-transfer proof.
`throw()` and `close()` enter evaluation through `throwflag`'s error path at
the current instruction, bypassing the normal RESUME path. Their handler
edges must kill scratch NULL facts before any reachable catch target; no
normal first-send seed applies to them.

The 36 diagnostic gives different cases, not a single lexical rule. In
`test_jump_across_async_with`, the real LINE stop is pc 5 (`LOAD_GLOBAL`),
before any `YIELD_VALUE`; target line starts at pc 36 (`LOAD_GLOBAL`). The
code has real EH entries, so ingress facts alone cannot grant this jump:
the existing context and target demand checks still decide it. In
`test_jump_backward_over_async_listcomp`, stop pc 32 follows SEND 15 /
YIELD 16; the target line is pc 2. The v2 variant stops at pc 43 after
SEND 21 / YIELD 22, targeting pc 3. Those are post-resume states and must
meet the later-send and throw/error paths; neither can borrow the created
frame's all-NULL row. Their inline comprehension also carries saved-local
and iterator obligations that the NULL Plan cannot create.

## Smallest safe analysis increment

First admit only optimized `CO_COROUTINE` code with an exact pc 0
`RETURN_GENERATOR` (A=NoDst, B=`co_nlocalsplus`) and pc 1 `RESUME`. Verify
the initialized frame/constant-slot layout and no alternate normal entry to
pc 1. Start the existing forward NULL worklist at pc 1 with the initialized
scratch row, clearing constant homes. Do not queue pc 0 for that matrix.
Keep the same graph, same intersection at joins, and same backward
VALUE/NULL_ONLY lanes. Audit the current `YIELD_VALUE` transfer as a resume
ingress: it must make the actual return-slot home UNKNOWN while preserving
only homes proven untouched by heap-frame suspension and later send. An
ordinary or source error, including injected throw at the resumed PC, loses
scratch facts as it does today. Any unsupported producer, frame overlap,
malformed first word, non-None CREATED send, unknown external entry or
inconsistent source/EH mapping falls back to no optional NULL matrix.

Keep this permission limited to actual executing real-LINE PRE with inherited
context, no pending child, empty current/target saves, zero VALUE demands,
ALL contained in NULL_ONLY and physical NULL at every required stopped home.
The existing warning-time rebuild and generation check must reprove the
same ingress/target facts after callbacks. No suspended frame, PY_YIELD,
SOURCE_POST, active async-with exit, or exception-arm transfer gains authority.
`CO_GENERATOR` and `CO_ASYNC_GENERATOR` require their own entry/wrapper
census before sharing the seed; this first packet need not enable them.

Positive witnesses should include a CREATED coroutine's first normal
`send(None)` reaching a real PRE line with a required optional-NULL target,
with both native and VM send routes. Negative witnesses need a non-None
CREATED send, throw into a catch before that line, an intervening YIELD with
an arbitrary sent value at the return-slot home, a post-yield async
comprehension with saved roots, and malformed raw RETURN_GENERATOR B. The
`across_async_with` request is an unchanged integration witness, not a
promised pass: if context or a VALUE demand remains, refusal with unchanged
state and normal cleanup is correct. A warning callback that changes the
generation must cancel or reprove the whole Plan.

This first increment makes one lifecycle edge explicit in the existing cold
graph. Missing finally/RERAISE roots and compatible same-arm transfers are a
separate owner/context question; NULL ingress does not manufacture them.
