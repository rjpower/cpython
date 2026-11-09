# Coroutine first-resume NULL ingress: source peer

Direction accepted for the bounded coroutine-only packet, subject to the
conditions below and actual-source review. This is a read-only source review;
it claims no new native result or implementation admission.

The follow-up created-throw audit found an omitted ingress in the initial
proposal. The entry conditions below are mandatory before source admission;
queueing only pc1 is insufficient by itself.

The exact initial shape is optimized CO_COROUTINE, excluding generator and
async-generator flag combinations, with RETURN_GENERATOR at0 (NoDst and
BASE=nlocalsplus) and RESUME at1. Constant scratch homes are not NULL seeds.
Closure/setup prologues and other initial shapes remain unsupported in this
first packet. Existing executing real-LINE PRE, context, save, all-alias,
binding, physical NULL and warning-generation checks stay unchanged.

`RETURN_GENERATOR` copies the initialized frame into the heap frame and sets
its recipient to NoDst. Both native `gen_send_value_ex` and VM SEND_GEN use
`_PyFrame_SetReturnValue`; the created-state None check prevents a non-None
normal first send. Discarding that first None does not populate scratch.
Public frame-local mutation changes named bindings, not this scratch seed;
an arbitrary external entry must not be treated as a successful first send.

Initialize `reached[1]`, its row and queue at1 only. Do not queue0 or merge an
invented unknown RETURN_GENERATOR0→1 transfer into this initial row. Any later
reachable CFG predecessor of1 must intersect the seed normally; it must not
reset it. If the first implementation requires no alternate entry to1, check
the existing sparse reverse graph and refuse unsupported geometry rather than
silently ignoring a predecessor. Unreachable rows remain conservative.

There is a precise throw-offset detail. `_YIELD_VALUE` increments the saved
frame PC to its successor. The evaluator's throw path uses that PC as
`next_instr`, and generated exception unwind looks up `next_instr - 1`.
Thus a suspended throw/close uses the YIELD site's handler (created throw
uses RETURN_GENERATOR0), not an assumed RESUME handler. The existing ordinary
EH edge at the YIELD site already forgets NULL facts. Do not add a fresh
normal seed to a catch path or replace that PRE error transfer by a successful
yield transfer.

In particular, the code constructor does not prohibit an otherwise valid
exception-table range covering RETURN_GENERATOR0. `validate_completion_words`
validates table bounds and retirement contracts, but has no pc0 handler ban.
A created `.throw()` can therefore enter that range's handler through offset0
without passing the seeded normal pc1. Likewise `validate_source_map` permits
an event.from0 with a handler; its RESUME barrier constrains event.to, not
event.from. The analysis cannot assume every excluded-prologue source recipe
is absent merely because the ordinary compiler emits none.

The smallest first-scope rule is to withhold the optional coroutine NULL
matrix when `analysis->handlers[0].target >= 0` or any source-error record has
from0. This refuses only the new proof capability, not code construction or
execution. An alternative is to seed every such handler target as a separate
all-UNKNOWN root (including clearing an existing pc1 seed if they alias), then
run the same worklist. That alternative must never queue pc0's normal edge.
Either rule covers the omitted ingress; ignoring it does not.

Normal YIELD's generated code clears outgoing BASE, stores only the encoded
future recipient A, and leaves other heap-frame homes in place. Its combined
suspend/resume physical transfer can therefore mark A UNKNOWN, BASE SET_NULL
only when it differs from A, and other scratch homes PRESERVE after exact
shape validation. A=NoDst leaves BASE NULL; A=BASE must give the incoming value
priority. This is physical state, not restored origin or setter authority at
PY_YIELD. Instrumented and plain yield need the same actual final-store proof.

SEND completion may need another exact shared fact: positively declared
CompleteStep completion drains its receiver/cursor/input window and publishes
the result at its recipient. Result aliases override NULL, and outside-window
homes remain UNKNOWN unless independently proved. Neither a SEND name nor a
suspension flag establishes this fact. Its body/resume path must not inherit
completion clears. Do not promise the three async trace witnesses pass until
their actual masks show that these facts cover the missing requirements.

Meaningful controls are native and VM first-send entry, non-None created-send
refusal, throw into an actual catch, arbitrary later sent value at A (including
A=BASE alias), constants, malformed first geometry and an alternate-entry
join. Existing active-save/iterator refusal controls must remain unchanged.
