# Construct resumable frames in their owning object

**Scope update:** the [entry calibration](entry-scope-calibration.md) is the
current first-scope contract. It removes mandatory native-tail extraction and
general precommit redirect machinery where a callback-free proof holds. This
draft retains shell/ownership and eligibility questions; it is not implementation
approval. Follow the current plan and calibration before the older details.

Read-only proposal, 2026-11-10. No implementation, native execution, build or
commit is authorized. Astra approved census/design only in
[candidate43 review](candidate43-and-generator-entry-review.md); this revision
answers its six interface questions without claiming implementation proofs.
Source audit uses the current execution-boundaries checkout;
[retained current41 profiles](current41-profile-analysis.md) own measurement
identities. This is a bounded call-provider/storage experiment, not a JIT design.

## Why this can precede SEND-recipient work for coroutines

The recursive coroutine kernel awaits Fibonacci(25) without yielding to an
external scheduler. Current41 has no YIELD_VALUE self samples. It executes
1.379018× upstream instructions and 1.342659× cycles. Exact calls are 10.28%
self samples, RETURN_GENERATOR 8.25%, make_gen 5.88%, GET_AWAITABLE 6.09%,
SEND_GEN 5.52% and CompleteGeneratorReturn 5.89%. Selected construction/allocation
symbols sum to 21.19% self samples; they are not a removable cost fraction.

The concrete candidate is to reserve the actual generator/coroutine object,
initialize its embedded frame and bind directly there, instead of first entering
a thread frame and immediately allocating/copying it at RETURN_GENERATOR.
Argument homes still move into the heap frame; allocation, await classification,
resume and completion still occur. The typed cell-getter port removes a distinct
bridge and does not answer this application-level creation/call/completion gap.

## Actual entry sequence and timing

| Stage | Current source and behavior | Required timing |
| --- | --- | --- |
| Call preparation/binding | Exact/generic providers ensure the executing copy, select/pin code, initialize an owning thread frame and bind arguments. `_PyFrame_InitIRInstrPtr` starts at executing word zero (`pycore_interpframe.h:67,300`; `ceval.c:call_transaction_*`, `initialize_locals`). | Call. Argument evaluation/validation and binding failures do not move to first resume. |
| Closure/cell prefix | `flowgraph.c:insert_prefix_instructions` prepends optional COPY_FREE_VARS, then ordered MAKE_CELL operations, before the original entry. COPY duplicates closure-cell owners; MAKE_CELL can allocate and replace an argument/local home (`bytecodes.c:2145,2216`). | Call, before the object is returned. An argument captured as a cell is wrapped before first resume; closure cells are retained even if the generator is never started. |
| Generator prefix | `codegen.c:codegen_enter_scope` emits RETURN_GENERATOR; POP_TOP; RESUME at function start. RETURN_GENERATOR allocates `_Py_MakeCoro`, advances the suspended instruction pointer, moves the header/entire slot vector with `_PyFrame_Copy`, sets the initial incoming recipient and FRAME_CREATED/GENERATOR ownership, then leaves recursion and pops the thread reservation (`bytecodes.c:5222`). | Call. This constructs the object without running its body. |
| Initial IR continuation | `assemble.c` proves POP_TOP is the sole consumer of the initial resume input and elides it; `codeobject.c:validate_completion_words` requires RETURN_GENERATOR's destination to be IR_NO_DST. `_PyFrame_Copy` retains executing-code/feedback pointers; the saved instruction is the real word after RETURN_GENERATOR, normally RESUME. | Established before returning the object. Never jump to a guessed bytecode offset or replay RETURN_GENERATOR. |
| First activation | Native send or IR_ENTER_GENERATOR installs/discards None through the saved recipient, links caller and handled-exception state, checks recursion, and starts at the saved real continuation. IR_SKIP_RESUME may skip a neutral RESUME after its instrumentation/eval-breaker guards. The body then runs. | First resume. Preserve created/non-None rejection, throw/close, instrumentation and ordinary resume behavior. |

CO_GENERATOR/CO_COROUTINE flags alone cannot prove this sequence. Generator
expressions insert RESUME, LOAD_FAST and GET_ITER/GET_AITER **before**
RETURN_GENERATOR (`codegen.c:5237`); iteration can call Python at construction.
Moving that work into first resume changes behavior and error timing. Async
function cleanup can also insert a handler after RETURN_GENERATOR. Do not infer
that a RESUME/exception edge may be dropped from an apparent adjacent prefix.

## Minimal experiment: empty prologue, exact positional construction

First admit only exact ordinary generators and native coroutines whose final
validated IR has RETURN_GENERATOR at word zero, followed by its validated
initial-input discard and a real RESUME continuation, with no closure/cell prefix,
pre-creation observer/event, branch, handler entry or other work to execute.
Use fixed exact positional arguments with no omitted defaults, variadics,
keyword-only arguments or virtual kwargs. This includes Fibonacci's one-argument
coroutine. Support both thread-owned and generator-owned callers: recursive
coroutine calls originate in generator frames. Exclude async generators, PEP 523,
active instrumentation/tracing and nonzero coroutine-origin tracking depth for
this first admission; they retain the existing provider.

The actual hot path is `CALL_PY_EXACT_ARGS`: its `_INIT_CALL_PY_EXACT_ARGS`
directly initializes a thread child, `_SAVE_RETURN_SLOT` names its result,
and `_PUSH_FRAME` enters it (`bytecodes.c:4286–4361`). A generator-owned caller
uses `_PyFrame_PushUnchecked` and moves arguments; a thread caller uses overlap.
This route does **not** call `call_bind_known_python` or accept an immediate
result merely because the generic `_PyCallResult` can represent one.

### Exact-call integration, without taxing ordinary hit paths

Propose one bounded CALL-family member, provisionally `CALL_PY_CREATED`, nominated
where `specialize_py_call` currently selects CALL_PY_EXACT_ARGS. Require its
validated entry descriptor, exact count and NULL self; bound-method wrappers
remain unsupported. Ineligible functions keep their current opcode: the extra
descriptor test is in specialization, not every ordinary exact-call hit.

The new handler retains PEP 523/function-version/count/recursion guards and uses
a terminal invocation marker for `call_try_construct_resumable`, an out-of-line
helper with the existing caller/base/destination/argc contract. It bypasses
`_INIT_CALL_PY_EXACT_ARGS`, `_SAVE_RETURN_SLOT` and `_PUSH_FRAME` on success.
The cases generator must explicitly admit/prove this CALL-window shape and
terminal publication; a new DSL name alone does not supply that proof. A
successful helper publishes the completed call result and returns `{0, NULL}`
to the same terminal call-completion emitter used by immediate results; no
fictitious child frame or delayed stack flush executes afterward.

Factor the exact positional owner move from the **nonoverlap branch** of
`_INIT_CALL_PY_EXACT_ARGS` into a shared operation that accepts a complete
destination frame and the caller's temporary argument window. It validates no
new signature rules: the existing exact-shape proof already establishes count,
no variadics/keyword-only omissions and NULL self. Move args once and NULL their
sources. Existing nonoverlap calls and direct heap construction use it; overlap
retains its physical-storage proof. The function window owner moves into
`f_funcobj`, independently from the code owner. Do not broaden the thread-only
known binder or introduce a second binding/error implementation.

For results, propose a shared `call_publish_immediate(call, result, retirement)`
contract extracted from `call_transaction_native`'s existing publication tail:
owned result; detached residual window owners plus native-private owners where
applicable; temporary publication before retirement; existing ordered cleanup,
periodic/refresh checks and delayed local/discard handling. Native-private
callable/monitor owners keep their existing order before window retirement;
the constructed route supplies NULL for those fields. All binding-moved cells
are already NULL in that residual window. This extraction is **required work**,
not an existing helper available to call. It must retain displaced-local/error
rooting and newer-redirect behavior in both consumers. Reaching the generic
fallback alone does not establish a hot exact-call construction hit.
The constructed consumer uses the existing exact-call periodic policy: it must
not inherit a native call's end-periodic check or invent an earlier poll. With
all arguments moved into the object, residual input cells are NULL; a local
result is installed before retiring its displaced local owner, as on the current
exact-call return. The native consumer retains its existing policy flags.

### Compiler-owned entry descriptor

Derive a small immutable code-owned descriptor from the **final validated IR**,
not from a trusted public flag or a scan repeated on every call. It names the
creation site, real first-resume offset and initial recipient (IR_NO_DST), and
certifies the exact admitted empty-prefix and edge/source contracts. Descriptor
absence means fallback. Function/version/provider and pinned code/exec identity
must still match at construction commit. Instrumentation may change the executing
handler, so validate the observation guards too; pristine-word validation alone
does not authorize bypassing active callbacks.

Recompute/validate this descriptor for compiler output, marshal loading and
code.replace construction. An altered prefix, target, exception/observer recipe
or initial recipient must reject fast admission. Keep the descriptor on the
code, with no new per-frame flag or extra completion field. Its concrete field
layout is an implementation-review decision; the experiment needs only one
small admitted entry shape, not a general prologue language.

### Pinned-code allocation interface

Do not call `_Py_MakeCoro(func)` unchanged: it rereads `func_code` for kind,
and `make_gen(func)` rereads it for capacity. Proposed
`gen_allocate_shell_for_code(code, kind)` takes a separately owned pinned code
and its validated exact gen/coro kind; it derives capacity solely through
`_PyFrame_NumSlotsForCodeObject(code)`. It does not read function metadata.
Function and code have independent C owners across allocation; caller argument
homes remain owning and no raw pointer into that window crosses preparation.

After allocation, reload the caller window from its frame/base. Recheck function
identity, function version, `func_code == pinned_code`, standard vectorcall,
exact count/NULL self, descriptor/exec identity, observation/origin/recursion
admission and absence of an intervening redirect. Read/incref the current name
and qualname only after that refresh, with no subsequent fallible preparation.
Frame constants, code owner, capacity and both continuations all come from that
same pinned code. A changed function/code cannot produce an old-sized object
whose frame follows the new code. Mismatch means safely destroy the empty shell
and select the current generic provider once, before any argument transfer.

### Complete shell and heap-frame states

`gen_dealloc` calls the internal GC_UNTRACK, temporarily tracks before invoking
finalization, then untracks and clears fields. Internal GC_UNTRACK requires a
tracked object (`pycore_gc.h:247`); merely decrefing an untracked shell through
that destructor is not the proposed cleanup. After raw allocation, the allocator
initializes every field below without callbacks, then tracks a **cleared empty
shell** before returning it to preparation. No failed shell is FRAME_CREATED.

| Field group | Cleared shell, before any possible safepoint | Callback-free commit and published object |
| --- | --- | --- |
| Object header/capacity | Existing exact-type GC allocator; capacity from pinned code. Allocation failure returns NULL, no object to clean. | Same allocation; no resize/reallocation. |
| gi/cr frame state | FRAME_CLEARED; no frame owners hidden by the traverse fast path. | FRAME_CREATED only after all owning homes/header are complete. |
| weakrefs, names, qualname | NULL. Names/qualname acquire current metadata after final refresh. | Weakrefs NULL; names/qualname owned. |
| exception state | exc_value and previous_item NULL. | Both remain NULL while suspended/never entered. |
| common subtype fields | origin_or_finalizer NULL; hooks_inited, closed and running_async zero, including otherwise-unused gen/coro fields. | Unchanged for ordinary gen/coro and origin-depth-zero scope. |
| executable/function/locals/frame object | f_executable = PyValue_None; f_funcobj NULL; f_locals and frame_obj NULL. | Separate code owner and moved function owner; locals/frame_obj NULL. |
| physical value slots | Every slot NULL, not uninitialized spare bytes. | Canonical argument owners, constants and NULL in every remaining home. No GC/poll/allocation inside the move. |
| globals/builtins | NULL until function is anchored by f_funcobj. | Borrowed views from that owned function. |
| previous/base/saved_top | previous NULL, base = owned_slots, saved_top NULL. | **previous stays NULL**, base stays embedded, saved_top stays NULL. Never call thread pop/restore on this frame. |
| IR fields | instr_ptr, ir_words, ir_feedback and return_pc NULL; return_slot = IR_NO_DST. | Exec words/feedback from pinned code; instr_ptr = real first RESUME; return_slot = initial IR_NO_DST. return_pc starts at exec word zero, matching ordinary initialization before the old frame copy. |
| other header state | owner = FRAME_OWNED_BY_GENERATOR; pending_kwargs = IR_NO_DST; pending_redirect, external, visited and debug lltrace zero. | Same initial values. Do not inherit caller links or flags through `_PyFrame_Initialize` defaults. |

`gen_traverse` may visit names, qualname, exception value, coroutine origin and
f_executable even when cleared; the shell supplies NULL/None for every such
field. `gen_finalize` exits on a finished/cleared state, so an aborted shell
neither warns nor executes user close code. `gen_dealloc` sees all its subtype
fields initialized and skips frame clearing for the empty shell. Function,
arguments and code remain rooted elsewhere until the callback-free move.

At commit, `_PyFrame_Initialize` may be reused with **previous=NULL**, embedded
base and the pinned code, but it initially sets THREAD ownership/return_slot=0
and does not initialize every bit or saved_top. The heap adapter must overwrite
those fields with the table's values before FRAME_CREATED; no safepoint is
allowed while a cleared shell temporarily contains newly moved frame owners.
Its constant-slot duplication is infallible under the current home contract,
including mortal constants; it remains real work. Only then may the result
transaction expose the created object. Nothing makes it current or links it
into the active frame chain at construction.

### Observation, recursion and once-only outcome ordering

Proposed provider order is: (1) original exact-call guards; (2) pin code/function;
(3) allocate/initialize the tracked empty shell; (4) reload/revalidate all call,
entry, instrumentation and recursion facts; (5) callback-free argument/frame
commit; (6) common immediate-result publication/retirement. No body, RESUME,
COPY_FREE_VARS or MAKE_CELL runs on this admitted construction path. First
activation continues to perform its separate existing recursion, context and
created-send checks.
Creation entry also executes DTRACE_FUNCTION_ENTRY/LLTRACE through the old
_PUSH_FRAME path, beyond the IR prefix itself. The first experiment declines
WITH_DTRACE builds and active low-level tracing, rather than silently removing
those observations. After allocation, newly enabled instrumentation requires
redispatch through the current call wrapper after empty-shell cleanup; invoking
the generic helper with stale uninstrumented flags would miss its event.

Preserve `_CHECK_RECURSION_REMAINING`'s **<= 1** rejection before shell allocation;
a failure deopts to the existing error-producing call path, rather than returning
a coroutine that would previously have failed at construction. Repeat that guard
after fallible preparation before commit. No net recursion decrement is needed
for the omitted construction frame, but the admission check remains. Do not add
a pending-interrupt poll just to test the proof: default GIL `gc_alloc` schedules
GC via `_PyObject_GC_Link` and does not collect there (`gc.c:2028–2107`). First
RESUME still handles pending work through its current guards. This is a source
observation, not a completed proof for every allocator/debug/instrumentation hook.

| Outcome | Owners and permitted continuation |
| --- | --- |
| Initial guard/descriptor miss | No private shell/transfer. Existing CALL deopt/provider once, with the original owning window. |
| Allocation failure | Original window unchanged. Preserve the allocation exception across private pin cleanup, then one existing call error/unwind; no retry allocation or binder invocation. |
| Post-allocation proof mismatch, unchanged caller/site | Destroy the tracked cleared shell and pins without touching argument owners; revalidate after cleanup too. Enter the current generic call transaction once, with the correct existing preparation flags. No call monitor/method expansion/body was replayed: first scope admitted none. |
| Illegal active-frame/exec change | Use the existing revalidation-error convention, preserving an operation error as cause; do not bind against stale pointers or pretend this is a normal miss. |
| Legitimate pending/newer redirect | Must be selected before any helper restores the old PC; no fallback or stale local write may overwrite it. The exact continuation/root protocol at this precommit phase remains a review item below. |
| Successful transfer | Each argument/function source is NULL after its move; the shell becomes complete/created before result publication. No post-transfer miss/fallback or separate child-return protocol. |
| Retirement/check failure | Shared immediate-result transaction owns result/residual inputs/displaced locals exactly once, preserves exception roots and latest continuation, and returns error/completed outcome. It never reruns construction. |

`call_record_refresh` is not by itself a redirect detector:
`value_operation_refresh` restores `frame->instr_ptr = current`. The new provider
must inspect pending redirect/site/current-exec state **before** invoking that
helper, and must not reset a newer continuation during shell/pin cleanup. The
original proposal's claim that generic `{0,NULL}` alone settled this is withdrawn.

## What the first experiment deliberately does not move

COPY_FREE_VARS and MAKE_CELL stay on the existing call-time route for unsupported
code. A later independently reviewed extension may execute their proved prefix
directly in a fully owning heap frame **at call time**, in the original order.
MAKE_CELL allocation failure and partially completed prefixes need clearable
homes and identical errors; do not postpone them to first resume. Genexpr
GET_ITER/GET_AITER, async-gen hooks, observed entries and origin tracking require
their own admission proofs. `_Py_MakeCoro` currently computes coroutine origin
using a current incomplete construction frame and its caller; invoking it from
the caller unchanged would select the wrong context. Origin-depth-zero admission
avoids that trap without changing the origin protocol.

## Evidence and retention gates

Before implementation, count actual eligible creations by validated entry shape,
caller owner and argument shape. In recursive Fibonacci, the source recurrence
predicts 242,785 coroutine creations per Fibonacci(25) call; verify with counters
rather than treating a 21.19% profile group as a saving. Separate allocation,
thread reservation/initialization, argument copying, frame copying, RETURN_GENERATOR
dispatch, first entry and terminal clearing. Some work shifts into the heap
constructor; include its full generated/native sequence, total text and spills.

Admission requires actual new-provider hit evidence; a CALL_PY opcode alone is
insufficient. Test plain generator/coroutine creation and first None send,
created non-None rejection, throw/close before start, unawaited warnings,
recursion limits, argument failures, code/default rebinding, cell/closure and
genexpr fallback, GC/weakrefs/finalizer reentry, frame/code introspection,
instrumentation/PEP 523/origin fallback, local/temp/discard/alias destinations
and allocation failure at each precommit phase. Compiler-negative tests must
reject changed entry edges/prefixes/recipient and rederive metadata on code
replacement. Independent review includes a real release compile and the emitted
argument/root/cleanup transaction.

Root then compares paired coroutine elapsed/counters, the creation-amortized
resume micro, yielding/delegating generators, logging and DeltaBlue at a coherent
checkpoint. Keep the new route only if removed construction work exceeds added
allocation/validation/transfer work without material unexplained regressions.
This can outrank SEND-recipient specialization for the current coroutine kernel;
the FOR-only resume micro and yield-heavy tree remain separate evidence cases.

## Remaining implementation-review questions

These interfaces resolve the intended route and ownership states, not their
proofs. Implementation is still blocked on a reviewed immediate-result-tail
extraction with exact native-private/window/displaced-owner order; generator
proof/contract admission for the new terminal CALL family member; and a concrete
precommit legitimate-redirect protocol (or a source-backed proof that the
admitted preparation cannot expose that phase, including installed allocators).
Descriptor construction/revalidation on compiler/marshal/code.replace and actual
eligible-hit census also remain undone. A plain function flag, an untracked
shell, calling `_Py_MakeCoro` unchanged, or success through generic fallback does
not discharge those items. Do not start runtime implementation until independent
review accepts that narrow provider/shell/continuation contract.
