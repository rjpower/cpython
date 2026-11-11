# C2: committed completion before owner retirement

Proposal review only. No runtime changes, native validation or performance
claim. This is the primary common-completion design lane; the nonfinal ledger
remains a bounded fallback until this boundary is proved and reviewed.

The nonfinal refcount preflight is sufficient, but not inherently necessary,
for activation elision. Reference counting handles alias multiplicity when each
detached owner is closed exactly once. A completed caller can own the result
before those closes invoke callbacks. Exact CPython finalizer timing and the
appearance of an optimized-away callee are not requirements here.

However, replacing the ledger with `_PyFrame_SetReturnValue` followed by closes
does not satisfy the current phase contracts. There is a concrete observer
classification gap, not just a missing performance optimization.

## Existing boundaries

`Python/bytecodes.c:_RETURN_VALUE` unlinks the child before
`_PyEval_FrameClearAndPop`, keeping the returned value C-owned during child
retirement. Only afterward does `ir_generator.py:return_to_caller` publish the
value with `_PyFrame_SetReturnValue`. Thus unlink-before-release establishes
the current frame, but does not publish a completed caller/result.

`_PyFrame_SetReturnValue` can complete a pending redirect, close a discarded
result, or close an old LOCAL through `_PyFrame_PublishReturnSlot`. It is not an
atomic no-callback preparation primitive. The native CALL transaction already
detaches owners before retirement, but its CALL-site refresh and folded-LOCAL
policy are not a PRE-next committed scene either.

Do not move ordinary result publication before child clearing without handling
overlap: child argument homes can share the caller's destination storage.
Clearing that child after publishing there could clear the new result. A shared
publication primitive may be used after ordinary child retirement and before
elided detached-owner retirement. Sharing does not require identical cleanup
ordering or inventing a child for the elided operation.

## Concrete obstruction: descendant does not imply pending source CALL

`Objects/frameobject.c:f_lineno` walks from the observed frame to a descendant
of the target caller. If such a child exists, it assumes a pending source CALL.
It accepts only CALL/CALL_KW with a discard return slot, then treats the saved
return PC as the successful completion state. This classification precedes the
later stopped-phase analysis.

After an elided operation commits its real next PC, retiring an argument can
invoke a finalizer and a trace callback beneath that caller. The descendant is
a cleanup callback, not a child executing the caller's next instruction:

- If next is not CALL, the classifier rejects the ancestor as an unsupported
  active-call recipe despite its complete PRE-next homes.
- If next is another discard CALL, it can falsely treat that unexecuted CALL
  as already pending and use its wrong completion state.

Setting `instr_ptr` and `return_pc` to next does not distinguish these cases.
Nor does an ordinary `_PyIRObserverSpec` solve this unchanged:
`_PyEval_GetObserverContext` requires the observed frame itself to match and
stops at an incompatible context; nested finalizer tracing observes a descendant.
The current committed-scope proof also excludes frame-pushing CALL formats.
No name-only observer exemption or fake child is justified.
Neither `instr_ptr == return_pc` nor an interpreter/native callback shim is a
unique completion proof: an unfinished C call can have the same PC equality
from a prior discard CALL and the same callback ancestry. Do not infer the
scene from those coincidences or an unaudited frame `external` bit.

## Proposed shared completed-call scene

Introduce an explicit retirement capability describing an already completed
caller at its real PRE-next state. It belongs to the common completion layer,
not the leaf expression evaluator. Its lifetime spans all releases that can
invoke callbacks. Existing observer scopes are precedent, but their current
event/descendant lookup is insufficient unchanged.
An activation/thread-scoped caller-and-phase capability is a candidate, without
a new owner bank or assumed frame flag. The caller's published homes remain
authoritative; this capability certifies their phase, not another copy of them.

The capability must identify the caller, actual landing and validated frame
generation, and prove all completed-call inputs detached and the result outcome
installed. Nested callbacks must find the target caller's capability without
interpreting an unrelated descendant as its source child. A setter supersedes
that landing with its new valid scene; cleanup must never restore an obsolete
PC, result or generation. Audit the observer/redirect classifier explicitly.
No partial CALL-stack reconstruction or synthetic callee is required.
PRE-next commitment must invalidate obsolete source-error/observer authority,
following the setter's `_PyEval_InvalidateSourceErrors` precedent. A redirect
supersedes the capability's original landing; revalidate its generation and
current state afterward rather than treating the old capability as authority.
Existing `value_operation_refresh` restores the old operation's `instr_ptr`;
it cannot serve unchanged as this completed-state refresh.

One no-callback preparation phase, in an out-of-line runtime helper, would:

1. Acquire a canonical result while selected source roots are live. Finish
   provider/observer/epoch checks and every fallible admission before commit.
2. Move every consumed window owner to live C-owned storage and NULL every
   original home. Detach any displaced LOCAL separately; do not let publication
   close it midway. Overlapping destinations count once.
3. Install the result in its final home, or retain its discard owner in C.
   Publish the real caller PRE-next PC/continuation and retirement capability.
   Caller locals and temporaries now describe that completed state.
4. Retire detached owners, old LOCAL and any discard result exactly once.
   Clear each C owner before its close so reentry cannot retire it again.
   Aliased C owners represent distinct counts and need no multiplicity ledger.
5. After callbacks, reload the actual caller, slots, execution and continuation
   through the common completed-state refresh. Honor the latest redirect and
   instrumentation; dispatch the actual landing, never a saved CALL+1 or stale
   borrowed instruction pointer.

Once published, the result belongs to the caller. A finalizer's `f_locals`
mutation may replace it; cleanup must not restore it. Detached C references
keep objects alive across GC and nested Python. Never clear a source or result
home afterward through an old window pointer. No borrowed callee function,
code, region, proof, counter pointer or probe result may be read after the first
release: that release can destroy them. Update admission statistics beforehand.

There is no MISS, rollback, CALL replay, fictitious frame pop or callee recursion
decrement after commit. Cleanup drains its remaining owners even if an error
must propagate; errors and unraisable finalizer behavior follow the shared
completion/error protocol. Callbacks may enable tracing, profiling or a custom
provider: the operation remains completed and subsequent execution honors the
changed state. Entry exclusions do not preserve observation facts after callbacks.

## Required witnesses before removing the ledger

Prove the scene with real final decrements: argument/self/function weakref
callbacks, cyclic GC, result aliases, old LOCAL displacement and discard.
Callbacks should inspect/mutate caller `f_locals`, install tracing/monitoring,
and retain the caller. Exercise next-nonCALL and next-discard-CALL landings,
with a finalizer trace callback requesting an ancestor `f_lineno` redirect.
Verify the next CALL is neither skipped nor replayed and a redirected result
is not overwritten. Include nested cleanup/redirects that invalidate an earlier
captured generation.

Strict value-API and positive source-contract checks must establish C-owner
lifetimes and the terminal boundary without addressed handler locals or effect
waivers. Ordinary-return/shared-publication witnesses must cover overlap and
callbacks from displaced LOCAL retirement. Exact finalizer schedules or debug
transcripts are not gates; ownership, observable effects exactly once, valid
continuation and supported redirect behavior are.

Until these contracts and witnesses exist, retain conservative nonfinal
completion. Selected leaf CALL handlers and immutable feedback-index reuse
remain independent C2 directions; neither proves callback-safe retirement.
Peer call_binding_lane independently identified the same descendant-classifier
obstruction and approves this conditional design lane, not an implementation
or speedup claim. Actual implementation remains unapproved and untested.
