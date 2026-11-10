# Immediate created-result contract for exact Python calls

**Design iteration, not the current first-scope contract.** The
[scope calibration](entry-scope-calibration.md) supersedes the mandatory native
success-tail extraction and general precommit classifier below. A proved
callback-free first scope uses MISS/ERROR/COMPLETE and ordinary return-publication
semantics. Retain shell/code/GC and actual callback-boundary questions as review
inputs; do not implement the broader machinery by following this draft.

Design only, against the current execution-boundaries source. This supplements
the revised [owning-entry proposal](owning-generator-entry-proposal.md) and
[Astra review](candidate43-and-generator-entry-review.md). No runtime edits,
native runs, builds or commits were performed. Shell layout and entry-descriptor
ownership remain with that proposal. The interfaces below are proposed work,
not existing APIs or completed implementation proofs.

The hot route is a new bounded CALL-family member, provisionally
CALL_PY_CREATED, selected instead of CALL_PY_EXACT_ARGS only for the admitted
empty-prefix entry. Ordinary exact calls retain their current handler. Sharing
`_PyCallResult` alone does not integrate this route: the new terminal producer
must consume an immediate result through the existing publication tail and
explicitly distinguish precommit retry from completed/cancelled operations.

## One success publisher, extracted without changing native ordering

Extract the success tail of `call_transaction_native`, beginning with
`call_transaction_detach` and ending at its final return, into one out-of-line
`call_publish_immediate(call, owned_result, resources)` operation. Leave native
invocation, C_RETURN/C_RAISE monitoring and unsuccessful-provider handling in
the native adapter. A failed native call must not enter the success publisher.

The resource record belongs to the out-of-line C activation, not a frame. It
contains the existing native callable owner, monitor argument, preallocated
retirement storage/capacity and whether that storage is heap allocated. Its
fields are explicit owners or NULL, consumed once by the helper. Native
retirement capacity is reserved before invoking the native provider, exactly
where the present code does it; the publisher introduces no allocation.

Keep this exact native success sequence:

1. Detach the actual call window into its reserved retirement storage.
2. Assert canonical result ownership. Publish a temporary destination now,
   holding its displaced owner in C. Hold local/discard results in C instead.
3. Publish the existing operation PC, close displaced temporary, then monitor
   argument (except immortal MISSING), callable pin, then the detached window
   in reverse order; free retirement storage only after its owners are NULL.
4. Perform the existing refresh, existing `call->periodic` check and refresh.
5. On pre-local-publication failure, put the held result in NULL window[0] for
   the existing error/traceback/RAISE/unwind recipe, leaving the local intact.
   Otherwise discard the result or replace the current local, then close that
   displaced local and perform the existing final refresh.

Do not move native-private releases behind window retirement, close a captured
old local instead of the callback-current local, or store a local before the
native periodic check. Error cleanup can share the resource-release operation;
it must not become a second result publisher.

The created consumer supplies NULL native-private owners and **zero residual
window owners**: exact NULL-self admission and the callback-free argument/
function move have already NULLed every call input cell. It passes no retirement
buffer and asserts that complete original window is NULL. Do not allocate an
argc-sized retirement array after commit or iterate a second binding engine.
Set `periodic = false`, matching CALL_PY_EXACT_ARGS/RETURN_GENERATOR. The common
local-store point then follows only NULL retirement and non-final pin releases,
so there is no callback before local publication. The displaced local is closed
after installation, exactly as `_PyFrame_SetReturnValue` does today. Discard
retirement may warn/finalize the new coroutine after the completed call state
is installed. No native-style extra periodic poll is introduced.

Successful code/function preparation pins can be released before this tail:
the complete result object's frame now owns both, so those releases cannot be
last-owner callbacks. Alternatively keep them until the zero-resource tail's
pre-publication release stage with that same proved non-final ownership. Do
not quietly reuse the native callable-pin slot for a possibly final arbitrary
owner; that would invalidate the zero-callback local-order argument.

## Precommit position and callback admission

Use a C-local position snapshot containing caller frame/code identity, numeric
CALL offset, executing words/feedback identity, current instruction word,
return PC, pending_redirect, and frame-object `f_lineno_generation` if present.
An initially absent frame object has generation zero; mere later frame-object
materialization is not a redirect. A successful setter increments generation,
including a same-PC redirect. Never retain an unowned frame-object pointer
across preparation or infer redirect intent from PC inequality alone.

Check this snapshot **before** `call_record_refresh` or
`call_transaction_publish_pc`: both can restore the old CALL PC. Current
`value_operation_refresh` unconditionally assigns `frame->instr_ptr = current`.
It is therefore not the new precommit classifier or the created consumer's
post-finalizer continuation selector.

The safest first scope excludes callback-capable allocation rather than adding
a broad debugger capability to allocation callbacks. A concrete admission check
must recognize the built-in RAW/MEM/OBJ allocator tables, debug wrappers and
their underlying tables, **and the separate arena allocator**. The existing
`_PyMem_GetCurrentAllocatorName()` checks the first tables but does not prove
the arena hook is built-in. Name equality alone is insufficient. Propose one
internal boolean predicate owned by `Objects/obmalloc.c`, using its actual
private built-in function/context tables; wrapped/custom allocators decline.
Its synchronization and cost must be reviewed, not hidden in a public code flag.
The ordinary GIL build is the first scope; no lock may be held across allocation.

With those allocators, exact gen/coro types, zero origin depth, no observers,
DTrace/low-level tracing or prologue work, the admitted allocation/link/track/
field setup must be audited to contain no Python callback or interrupt poll.
`gc_alloc` calls the allocator and `_PyObject_GC_Link`; the latter schedules GC
rather than collecting. Pending work remains deferred to its existing first
RESUME boundary. Immutable-name increfs and canonical owner duplicates do not
allocate. This is a finite source proof to review, not a claim that all
installed allocators are non-reentrant.

Still classify post-preparation state before reading the current window. An
unchanged site can lose function/code/provider/descriptor/observation eligibility;
a changed executing handler requires current-wrapper redispatch. An active-frame
or code/exec identity change outside the supported contract is a revalidation
error. A legitimately committed debugger redirect has priority over any normal
miss and must not be overwritten during private cleanup.

## Concrete terminal outcomes and cleanup

Keep the two-word helper result shape; give the new helper's status values
named meanings rather than interpreting all positive values as generic miss:

| Outcome | Generated continuation |
| --- | --- |
| ERROR | One existing CALL error edge at the captured site, with original argument homes still owning on allocation failure |
| COMPLETE | Result already published, no child; dispatch the real next word |
| MISS | Unchanged uninstrumented site, original window intact; deopt to existing CALL family head once |
| REDISPATCH | Same numeric site after wrapper/instrumentation change; reload current words/slots and dispatch its actual wrapper once |
| REDIRECTED | Operation cancelled before commit, or newer redirect after committed publication; dispatch the selected current real landing without replaying CALL |

Only CALL_PY_CREATED's checked terminal emitter interprets these extra outcomes;
ordinary transaction consumers retain their current result interpretation. This
does not add a frame field, alternate value ABI or fictitious child pointer.

Before a miss/redispatch/redirect returns, destroy the tracked **cleared** shell
and private pins while leaving source homes untouched by the provider. Hold an
allocation exception with `PyErr_GetRaisedException`/SetRaisedException across
cleanup; do not let cleanup replace it with an unrelated incidental exception.
Reclassify position/epoch/generation after cleanup before selecting a continuation.
Never refresh stale PC first. Do not read old window values after a redirect:
the setter may have replaced/retired them legitimately. No post-transfer MISS
or REDISPATCH is legal.

For a valid precommit redirect, cancellation requires the existing setter's
real committed landing: no active child, `pending_redirect == 0`, and instruction
PC equal to the setter's return PC with a newer generation. Pending-child
redirect machinery is not borrowed by a constructor with no child. Anything
else is an explicit unsupported-state/revalidation error; do not manufacture a
recovery by restoring the old PC. First-scope allocator/observer exclusions
should make this precommit redirect outcome unreachable; negative controls must
verify the classifier rather than silently omit it.

After successful created publication, the first possible Python callback is
the displaced-local/result-discard retirement. Capture its generation before
the release, and validate current frame/code/exec afterward **without assigning
the old CALL PC**. A newer generation selects the current committed landing;
otherwise dispatch captured CALL+1. Reload the executing word at that selected
numeric position so newly enabled instrumentation is honored on the next
operation; do not replay the completed call's CALL event. The native consumer
keeps its existing refresh policy during this extraction. This policy choice
belongs to the shared tail's explicit consumer contract, not a copied publisher.

## Allocation failure and error recipes

Pinned function and code are independent owners. Slot capacity, constants and
creation/first-resume offsets derive solely from the pinned code. No window
pointer survives allocation. After preparation, reload frame/base and validate
function identity/version, func_code, provider, exact count/NULL self, descriptor,
exec and observation state, before callback-free transfer. No generic binder
is retried after successful object construction.

An allocation failure leaves the caller window untouched, restores the primary
exception after private cleanup and takes the existing CALL-site error edge.
There is no speculative normal result in window[0] on that precommit failure.
The old RETURN_GENERATOR allocation failure occurs in a thread frame before
its first traceable instruction: `label(error)` skips traceback insertion for
`_PyFrame_IsIncomplete`. The entry descriptor must certify that condition and
exclude local exception handlers and monitoring so the omitted construction
frame supplies no missing traceback/handler/event. Do not apply this reasoning
to a MAKE_CELL prefix, observed entry, or generator expression. Argument owner
retirement timing is governed by the existing error recipe, not an invented
exact-finalization transcript.

## New producer contract and staged implementation boundary

Use one dedicated terminal marker, provisionally
`IR_INVOKE_CREATED_PYTHON_CALL`, in a marker-only invoke uop. Its generated
external IO is `(callable, self_or_null, args[oparg] -- result)` in the CALL
window with a folded local/temp/discard destination. The placeholder output is
never subsequently flushed. Mark the operation escaping/error-without-pop,
sync/terminal/frame-capable as required by its fallback; a marker name alone
does not grant those facts.

Validate the exact prefix: PEP 523, function version, exact count, NULL self,
recursion <=1 rejection and a validated created-entry guard, followed only by
the terminal invoke. Descriptor/observation misses occur before resources.
There is no `_INIT_CALL_PY_EXACT_ARGS`, `_SAVE_RETURN_SLOT`, `_PUSH_FRAME`,
end-periodic uop or publication suffix in this family member. Reuse the existing
terminal call emitter after separating result-outcome dispatch from invocation;
its COMPLETE arm cannot run a delayed output store. The ordinary exact opcode
keeps its current prefix and body. Do not import stack-space admission for a
temporary thread frame the new route will never allocate; preserve recursion
admission independently.

Compiler positives cover each destination and exact window shape. Negatives
change/remove every guard, permit self, add a release/callback, alter consumption,
append a store/poll, change entry/exception/source metadata, or add a fictitious
child dispatch; generation must reject them. Native tests must independently
show eligible exact recursive calls reach this provider, local replacement and
discard callbacks observe completed state, and allocation/wrapper/redirect/error
cases execute once with correct roots. Shared native-tail tests must show its
private/window/local/periodic order is unchanged.

A reviewable sequence is: first extract the native success tail with identical
generated behavior and focused tests; then add the checked terminal created
consumer, shell and code descriptor together. Do not call either integrated
before root's merged release/debug validation and actual hit census.

## Precise remaining gates

This resolves where the hot opcode enters and how one publisher can serve it;
it does not authorize implementation. The allocator predicate's complete hook
coverage/synchronization, the no-callback allocation audit, and the new position/
generation classifier still need independent review. Without that predicate or
an independently accepted general precommit reentry protocol, **construction
implementation remains blocked**. Shell field/cleanup proofs, final-IR descriptor
derivation on compile/marshal/code.replace and eligible-hit census are also
outstanding in the owning-entry packet. Do not replace any of these gates with
an unconditional generic `_PyCallResult` success claim.
