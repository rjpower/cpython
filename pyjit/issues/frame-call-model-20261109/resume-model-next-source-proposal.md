# Resume model after shared completion34

Read-only proposal. Root's accepted counters against golden show generators
1.032996 instructions/1.084226 cycles and coroutines .990849 instructions/
1.029274 cycles. They justify further inspection, not attribution to one source
check. No implementation is admitted here.

## The apparent duplicate state load is already gone

Current34 `_TAIL_CALL_SEND_GEN` is 743 bytes. The GIL release assembly loads
`gi_frame_state` once at function-relative +0x81. For CREATED it checks the sent
value is None; for other states it checks the same loaded byte against the
executable threshold. Both successful paths reach the single EXECUTING store
at +0xa3. `gen_try_set_executing` was inlined and the compiler coalesced its
source load with the earlier CREATED check. Thus helper consolidation cannot
be justified as removal of a second load in this measured build.

A cleaner shared primitive remains possible: a `gen_try_resume(gen,
accepts_created)` accepts a literal true for FOR (implicit None) and the actual
None predicate for SEND. One state read/validation loop checks both input
legality and the transition. The free-threaded nonunique path must revalidate
the updated state after every failed CAS, retain its LOCKED refusal, and leave
errors to the existing fallback. The unique/GIL path must not write EXECUTING
until legality is established. This is a source-contract consolidation with no
demonstrated GIL machine saving, so it is not the first performance experiment.
Do not delete CREATED/non-None, PEP523, running/finished or recursion checks.

## Smallest positive producer fact: yielded home transfer

The current plain YIELD_VALUE chain takes its yielded owner from a canonical
frame home, saves/clears its input, suspends/unlinks the child, switches to the
parent and hands the unchanged owner to `IR_STORE_RETURN`. That last macro
calls Publish again. Actual `_TAIL_CALL_YIELD_VALUE` is 654 bytes and contains
the object/type checks plus an out-of-line Canonicalize call before recipient
publication. This work is real for the compiled path, although its dynamic
frequency/cost remains unmeasured.

The existing `IRStack.store(reloaded)` already selects
`IR_STORE_CANONICAL_RETURN` when SlotLayout.canonical_return is proved. The
smallest experiment is an additional bounded source proof for the exact plain
yield chain, using that same field and store primitive. Require exact TAKE
prefix, unchanged result identity through the full yield/suspend/switch body,
input clearing before publication, and no extra callbacks/rebinds/aliases. A
renamed equivalent is eligible; a known YIELD handler name alone is not.
Unsupported chain mutations retain ordinary Publish. The instrumented yield
chain requires its own exact event-prefix proof or retains the current path;
do not certify all yields as a family by name.

No state check, slot selection, NoDst handling, pending redirect, profiling
update, exception-state link, recursion balance or frame switch moves. Native
object-boundary sends keep their normal input preparation. This changes only
the proof-selected final transfer, not the ownership or stopped-phase model.
Its likely coverage is delegated generator yields; recursive coroutines which
complete without yielding will not benefit. That limit must remain explicit.

## Why recipient work and direct binding are larger

`IR_ENTER_GENERATOR` detaches SEND's input window owner, saves the parent's
return recipient/physical PC, commits child/current/handled-exception links,
then checks recursion and installs/discards the incoming owner. A NoDst incoming
value can be the last mortal owner, so moving its close ahead of that commit
would expose an invalid phase. `_PyFrame_SetReturnValue` also resolves genuine
pending redirects before installing or retiring the value. Its branch is not a
redundant checking artifact merely because most benchmark resumes do not jump.

On YIELD, SEND's ordinary recipient is a NULL temporary; FOR can select a live
local; pending redirects can change the intended continuation. Removing the
generic recipient's old-owner load/close requires a positive caller/recipient
fact and an error/redirect adapter. Re-decoding the parent's operation on every
yield, or adding a saved header flag on every resume, might cost more than the
current NULL test. Retain that route until a measured, shared recipient proof
can replace it. No benchmark-only switch is appropriate.

Direct heap binding could remove thread reservation/RETURN_GENERATOR/frame
copy on creation, helping both workloads. It still needs the prior separate
partial-frame GC, failure cleanup, defaults/kwargs, required entry prolog,
PEP523, monitoring and caller-result contracts. It is not a bounded resume
validation change and should not be bundled with a yielded-home proof.

## First gate and ranking

1. If root selects a small next performance experiment, prototype only the
   yielded-home proof through the existing canonical transfer infrastructure.
   Require host whole-chain mutation negatives, input/output/helper shadowing
   and extra-effect refusals; inspect the generated diff and assembly for only
   proof-selected Publish removal.
2. Keep combined state validation as a shared source-contract followup unless
   another build/profile demonstrates uncoalesced loads or meaningful work.
3. Keep recipient specialization and direct heap binding separately designed
   and measured; neither may borrow unproved canonical or NULL-recipient facts.

Native gates must include initial non-None sends, repeated/running/exhausted
generator and coroutine sends, recursion failure, yield-from/await, NoDst
resume input last-owner finalizers, used/unused yielded values, pending-child
redirects, monitoring/error payloads and ordinary generator/coroutine tests.
No prompt-finalizer timing guarantee is introduced. Root owns generation,
builds, native checks and an isolated paired elapsed/PMU decision.
