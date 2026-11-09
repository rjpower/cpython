# Ignored Python call result completion

Proposed and independently reviewed 2026-11-08 against golden `0765e9be47f`.
Coordinator admitted both routes and CALL-PC policy; implementation is in the
boundary-ir worktree, pending independent actual-diff and native validation.
This follows the separately reviewed boolean edge increment.

## Constraint classification

Python permits delayed/omitted garbage collection. Exact decrement timing is
not a universal language requirement, and a register-release instruction is
not inherent to frame ownership. Language requirements here concern live
references, frame observation, exceptions and instrumentation. Prompt finalizer
ordering is current CPython behavior and is retained as this bounded change's
compatibility policy. Bounded retention is a memory/lifetime quality concern.
The selected runtime's owner/null-root and cleanup rules are safety invariants
until a reviewed replacement exists.

Deferring ignored results to caller-frame exit is possible in principle. It
needs either storage proportional to the number retained (unbounded in loops),
a bounded retirement region, or overwriting a single owner home at later calls.
The latter can delay one result's finalizer until another call and requires an
explicit GC edge and exception cleanup. Direct completion-time disposal instead
needs one existing C owner and no retained result home. It is the cheaper first
experiment under the selected RC architecture, without claiming a language
mandate for exact release timing.

## Bounded representation

Add execution-only CALL_PY_EXACT_ARGS_DISCARD and CALL_PY_GENERAL_DISCARD
variants, shadowed to their respective ordinary specializations, without
changing canonical call/POP words. Admit only an
ordinary synchronous Python callee and an immediate canonical POP_TOP whose
input is precisely the call's temporary destination. Exclude local destinations,
IR_NO_DST, native completion, bound-method/keyword/ex routes,
generator/coroutine code, and unsupported
observer shapes. Other predecessors still execute the original POP.

The caller's uint16 return_slot has twelve meaningful bits. Reserve a high-bit
POSTCALL_DISCARD tag while preserving its original result slot in the low bits.
The exact call variant sets the tag at the existing _SAVE_RETURN_SLOT point,
after all guards and child preparation but before switching frames. This is
recipient metadata, not a hidden stack effect or an extra owning slot. Exact
call deoptimization before commit uses the original handler and original slot.
No result owner is created in the caller while the child runs.

The return still takes its result into a C owner, makes the caller current, and
retires the child before touching the recipient. Only then does completion read
the pending recipient tag. Ordinary recipients keep the current publication
path. A tagged recipient computes its logical retirement boundary from the
saved call PC + 1 and checks the live handler there:

- Ordinary POP_TOP and an admitted observer mode: advance next_instr past the
  logical POP boundary and release the C-owned result as CALL completion.
  Keep caller instr_ptr at CALL; do not publish a dormant POP PC.
  Do not write or clear the result temporary; do not dispatch POP.
- Wrapped/instrumented POP, lltrace, or another declined observer mode: remove
  the tag, publish to the preserved original temporary, and dispatch the
  original POP at its original coordinate. Its existing callbacks, f_lineno
  reconstruction, exceptions and ownership cleanup remain authoritative.

Completion must inspect the live target after child cleanup, since a child local
finalizer can enable tracing/monitoring after the callee body finished. This
return-time guard avoids proactively repairing all suspended recipients and
covers code-local monitoring changes. Direct disposal can run Python, unlike the bool interval. The selected model
must therefore define its observable completion position rather than assume
that no observer can run. The proposal below makes disposal part of CALL and
retains CALL as the last attempted instruction in ordinary mode.

### Completion PC choice and observer cost

The ordinary completion PC is CALL, whose source line matches the admitted
canonical POP. No language rule requires a PC store for a nonexecuted POP.
The existing child-cleanup phase already calls Python with the caller current,
CALL PC saved, argument temporaries consumed and the result in a C owner. Direct
result disposal uses that same valid completed-call state. Local/GC/frame owner
consumers see the same valid homes. Under active instruction/line observation,
the existing POP fallback remains responsible for its own PC and reconstructed
result home. Test callbacks that activate tracing during result disposal and
cross-frame f_lineno attempts against this explicit completed-call state.

When the logical POP has no active line/opcode action, this deliberately
permits raw caller f_lasti inspection from a finalizer or profiler to report CALL
instead of the old standalone POP. Its source line and destruction order
remain unchanged. Treat that as an implementation-coordinate compatibility
choice requiring peer/coordinator admission, not as a language necessity or an
unnoticed consequence. If an actual supported debugger consumer proves it needs
the old POP PC even without active observation, retain that PC store with its
measured cost; do not add it merely to mimic an operation that no longer runs.

The live POP handler test is a cheap direct indication that this boundary has
an active observation action, including local events enabled during cleanup.
A coarse global monitoring-active check would force fallback for unrelated code
and would still need exact handling of code-local state. Proactively changing
all pending recipients requires frame walks for local-event changes and shared
code/suspended-generator rules. The completion-time target test avoids that
new protocol. Compare GCC output; if it dominates the saving, only then consider
a prepared per-boundary observation bit maintained by instrumentation.

The completion protocol adds a tagged-recipient test and one admitted-boundary
handler test. It removes result-slot publication, POP handler dispatch/decode,
slot-class discovery, result load and slot clear. It must be rejected if GCC or
measurements show the new tests recreate the removed cost. The required owner
release remains. This first representation retains dormant canonical POP
storage; it removes execution and slot traffic, not physical code words.

## Source interfaces and invariants

- `ExecutionVariant` descriptor gets an explicit postcall_discard property;
  numbers 323–324, canonical shadow/base/contracts remain the original
  specialization/CALL.
- `specialize.c` chooses this variant only for matching canonical immediate
  call/POP destination and synchronous callee code. Existing function-version,
  PEP 523, code-layout, recursion and stack guards remain unchanged.
- Generator replaces _SAVE_RETURN_SLOT emission only for this variant. It must
  validate the actual exact-call commit shape, not infer it from a name.
- `pycore_interpframe.h` defines the recipient tag and a return-completion helper
  that receives the owned PyValue by value and returns the next instruction
  pointer. No address of a handler local is needed beyond existing Publish.
- `IR_STORE_RETURN` invokes completion, preserving the caller's frame/slot
  ownership and C owner across child cleanup. Other SetReturnValue users
  (generator send/resume) retain ordinary recipient semantics.
- All actual return_slot consumers must understand or prove unreachable tagged
  values; debug assertions, frame copy/escape and generator caller cases must
  be checked. Exceptions from the child unwind at the original call PC with no
  result owner in the caller. A later call overwrites stale recipient metadata.

Call instrumentation does not require invalidating this variant just because
its POP becomes wrapped: the completion guard observes that live wrapper. The
call's own event patching still follows normal shadow/base rules. With lltrace,
completion always takes the original POP path; one code image remains shared.
Free-threaded selection remains disabled until its synchronization proof exists.

## Consumer census and tests

Canonical bytes retain f_lasti, line/exception ranges, monitoring positions,
jump targets and debugger stack kinds. The observer fallback materializes the
original result home before any POP callback. Frame/GC consumers see no extra
owner edge on direct completion. A C-held result must remain protected across
child cleanup exactly as it is today. A child cannot resume a suspended caller
before return completion; generator-owned callers are allowed only after their
existing overlap/retirement behavior is verified.

Test the two completion paths with mortal/inline results, nested calls, caller
local mutation, frame escape, child exceptions, call guard decline, tracing and
global/local monitoring enabled in the callee and in child-cleanup finalizers,
observer errors and f_lineno jumps. Compare the current CPython timing policy
explicitly; do not weaken existing timing tests under a language-level claim.
Test bounded retention in repeated discarded calls. Verify no tagged slot reaches
ordinary SetReturnValue. Then independent actual-diff review and coordinator
release/debug compile/runtime, GCC code inspection and timing.

## General guarded Python entry and target census

The retained final `warmed-shape-comparison/candidate-frame-borrows.txt` has nine
CALL_PY_GENERAL + POP_TOP pairs in bench_silent, each using r8 as both call
destination and POP input. Its tenth CALL_PY_GENERAL already has no destination.
The inner Logger.debug -> isEnabledFor call is CALL_PY_EXACT_ARGS and consumes
its result. Thus exact-only disposal covers none of the target nine ignored
results. The coherent increment must support general guarded child entry too.

CALL_PY_GENERAL's generator uses IR_INVOKE_GUARDED_PYTHON_CALL and the existing
call_transaction_run_guarded_python_vector runner; its apparent trailing
_SAVE_RETURN_SLOT is not executed by the generated terminal adapter. Do not
create another runner. The general discard alias adds a constant
CALL_POSTCALL_DISCARD flag to the existing transaction invocation. Store it as a
nonowning boolean in _PyCallTransaction. Destination remains the original
numeric slot throughout preparation, callbacks, revalidation and native fallback.

At the existing Python child commit points (overlap success and consuming binder
success), set caller return_slot to destination | POSTCALL_DISCARD when the
flag is present and the actual selected child is synchronous. Ordinary child
commits keep the original slot. Check code flags again at commit; callback-time
__code__ or provider changes already pass the existing revalidation guards.
Native fallback ignores the flag and publishes its result normally; next_instr
still reaches the original POP. This preserves native argument/input retirement
without admitting a new native lifetime policy.

Keep all existing post-kwargs-allocation guards and resource/error cleanup.
Preparation can enable instrumentation; the eventual completion guard still
observes the POP's live handler after child cleanup. Consuming-binder success
uses the same recipient metadata and completion protocol, while its argument
copy/detach/retirement ordering remains unchanged. No pending tag is installed
until a Python child has actually committed. Child creation failure leaves no
result owner or new recipient obligation. Bound-method, keyword/ex and native
handlers remain excluded from selection in this first increment.

The exact alias is a cheap implementation/test ladder step, followed by the
general alias under the same completion helper before any logging coverage or
performance claim. Verify all nine actual sites select the general alias in a
fresh warmed IR capture; static evidence identifies potential coverage only.


## Raw-recipient consumer scope

The raw recipient census includes _PyFrame_SetReturnValue; _PyFrame_Copy;
Objects/genobject.c:_PyGen_YieldFromRef (return_slot - 2);
Modules/_remote_debugging/asyncio.c:get_awaited_coro_address (remote raw slot);
Objects/frameobject.c:f_lineno setter (replaces slot when suspended); frame and
trampoline initialization; exact/SEND/FOR_ITER entry writes; and the two Python
transaction child commits. Debug-offset validation exports the unchanged uint16.

For this first increment exclude generator/coroutine-owned CALLER frames as well
as generator/coroutine callees. Selection checks the immutable caller code flags
and current ownership; child commit also declines tagging unless the caller is
thread-owned. Therefore the generator raw-reader and remote awaited-coroutine
paths never receive a tag. Suspended-frame f_lineno writes remain ordinary.
Active frame exposure preserves the current frame; copying a completed ordinary
frame carries numeric metadata but cannot resume it or send it a value. Clear
the tag on either completion path; verify unwind/escape tests and document any
stale recipient metadata on an exceptional exit. Ordinary SetReturnValue should
assert that a postcall tag cannot reach it without completion handling.

## Mutable caller PC and debugger jump invalidation

Do not add a saved-PC field or assume instr_ptr is immutable during cleanup.
The existing mutator is frame.f_lineno assignment. On a successful
f_lineno jump, the setter clears POSTCALL_DISCARD from a thread-owned frame's
return_slot before replacing instr_ptr. This changes only nonowning pending
recipient metadata, preserving the original result slot. Completion then uses
the ordinary recipient path and the existing LOAD_IP(1) continuation derived
from the debugger's changed PC, exactly as the baseline does. Failed f_lineno
assignments leave pending metadata intact. No post-effect replay is involved.

The remaining tagged case therefore still has its saved CALL PC, so CALL+1 is
its actual dormant POP anchor. Instrumentation does not move images or change
instr_ptr, and suspended-generator mutation is excluded from tagging. Census
all direct instr_ptr writes to confirm no other accepted cross-frame mutator
bypasses this invalidation before implementation admission. This adds one
setter-side metadata clear on a rare debugger operation, rather than another
ordinary return check, saved-PC field or frame-header store.

Eligibility must compare CALL and POP source lines through the code object's
line map before selecting the alias. The direct CALL-PC policy is admitted only
when those source lines agree; call/POP columns and raw f_lasti can still differ
as the explicit implementation-coordinate choice above.

The direct-write source census (`rg '->instr_ptr\s*='`, excluding generated
handlers) finds frame initialization/copy and incomplete-frame freevar setup;
current-operation restore helpers in ceval.c; current-frame call/native
publication helpers; normal handler RESUME/NOP/RERAISE PC writes; and the
f_lineno setter. The restore/publication helpers act on their current operation
or newly prepared child, not an inactive caller that has committed a different
Python child. Incomplete-frame setup cannot apply to an active CALL frame.
Consequently successful f_lineno assignment is the existing cross-frame PC
write accepted by this implementation that invalidates pending discard metadata.


## Ordinary return cost contract

The implementation factors slot publication once. _PyFrame_CompleteReturn first
branches on recipient < IR_NO_DST and directly uses that shared publication
helper. Only special recipients decode IR_NO_DST versus POSTCALL_DISCARD.
_SetReturnValue's existing SEND/native direct callers keep their ordinary
protocol. Thus the common Python-return path does not first test a tag and then
repeat the prior IR_NO_DST test. The general transaction flag is a separate,
explicit int; current x86-64 member layout appears to consume old tail padding,
but compile/sizeof evidence remains required. All ordinary transaction records
still initialize the flag, and the recipient range comparison can change GCC
code layout. Count these costs across untagged calls/returns when measuring the
nine removed POP executions; do not credit only the local savings.


The documented f_lineno jump API applies only to the bottom-most frame. The
cold setter invalidation preserves safety for the implementation's currently
accepted parent-frame writes; it does not extend that documented guarantee.
Likewise a parent-frame jump from a result finalizer is not made newly supported:
the golden POP already retains a saved next_instr across result destruction.
The admitted completion preserves that existing continuation model and adds no
post-close PC reload/check solely to broaden this undocumented behavior.
