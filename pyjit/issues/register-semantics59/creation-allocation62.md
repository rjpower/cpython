# Direct creation with callback-free allocation

Proposal for [parity62 section 0](../../design/proposals/interpreter-parity62.md).
Root and independent source-direction peer approve implementation after the
grouped design freeze. No native measurement is claimed. Start a new W branch from the complete
round61 creation source and fixture corrections (`7bf6812d5aac`), preserving
`pyjit/parity61-created-generator`. Do not add the rejected producer, TEMP or
ascent packets. Root owns generation, native qualification and selection.

## Established boundary

The admitted GIL path has exact generator/coroutine code, zero origin depth,
no creation hooks or observation, a current learned function version and
monitoring epoch, and a heap caller. Keep those existing gates and cold created
entry proof. The following actual chain cannot execute Python:

- `Objects/genobject.c:1178` `make_gen` calls `PyObject_GC_NewVar`, initializes
  the cleared object, takes name/qualname references and tracks it.
- `Python/gc.c:2075` `gc_alloc` calls `_PyObject_MallocWithType` and
  `_PyObject_GC_Link`; the latter only schedules GC through the eval breaker
  (`2034–2062`). `_PyObject_GC_NewVar` (`2114`) initializes the variable object.
  Tracking links the object; it does not collect.
- `pycore_object_alloc.h:41` reaches `PyObject_Malloc`. The allocator and
  reference-tracer release contract in `pycore_object.h:111–117`, enforced by
  the `obmalloc.c` release-check wrappers, forbids Python callbacks, exception
  changes and raising from those hooks. Allocation may return failure and
  schedule later GC. Those effects do not imply a Python callback or poll.
- `genobject.c:1201` `make_coro` with origin depth zero only completes the
  returned coroutine's NULL origin field. Neither a GIL release nor a poll
  occurs between the existing context check and that read. Plain generators
  use the same `make_gen` interval. Async generators remain excluded.

This is a reached-path proof, not a global nonescaping annotation for
`_Py_MakeCoro`: its other callers and nonzero-origin path retain their existing
contracts. Do not label the entire creation marker callback-free. Its result
publication, discard and error cleanup can still run Python.

## Exact provider changes

Replace only the staging interval in
`Python/ceval.c:2002–2115`, `call_create_generator_exact`:

1. Keep the current version/arity prefix and epoch/context hit checks. Take one
   independent `Py_NewRef(func->func_code)` snapshot. Retain `current`, the
   executable entry and its proved initial scratch/discard recipient. Do not
   snapshot the arguments into another buffer or increase their counts.
2. Delete `small[10]`, `count`, `owners`, `PyMem_Malloc/Free`,
   `_PyTransitionPrefix`, the pre-allocation EXTERNAL push and the loops that
   take the CALL homes into it. Leave actual function/self/argument owners in
   their materialized CALL homes across allocation. The caller remains its
   current frame and those homes remain its ordinary container edges.
3. Retain the balanced recursion decrement/restore around the shared
   constructor. Call `_Py_MakeCoro(func, code)` under the established zero-origin
   condition. Its ordinary incomplete-child origin derivation is unreachable
   here. Add a debug assertion of that reached condition rather than a second
   constructor or another hot gate. Remove now-unused `_Py_MakeCoroForFrame`
   and its `pycore_genobject.h` declaration; ordinary `_Py_MakeCoro` is unchanged.
4. On allocation failure, preserve the raised exception while dropping only
   the extra code pin, then return error with original input homes untouched.
   Keep this owner-release cleanup distinct from the allocator effect proof;
   do not need a new claim about arbitrary code-owner release. Do not drain
   input owners here or use the old consumed-window cleanup label. The existing
   handler error ingress retires its actual unconsumed prestate once.
5. On success, complete initialization without a callout: initialize every
   final home to NULL, then move each actual argument directly from its CALL
   home to the corresponding parameter and NULL its source. Take the function
   home once, NULL it, and pass that owner plus the independent code pin to
   `_PyFrame_InitializeOwnedCode`. Keep its existing constant-slot filling and
   final header/CREATED setup. Only after the full frame owns its graph is
   the caller's CALL input window consumed.
6. Install the caller's existing `return_slot`, `return_pc` and phase before
   the first operation that can release a user owner. Keep the current hit
   accounting and continuation identity. There is no success replay or guard
   miss after ownership moves.

The argument mapping stays `source = i + 2 - has_self`. With self, B1 is the
first argument; without self, B1 is a virtual hole and must not be read or
cleared. All explicit arguments and the callable are actual owning scratch
cells even when several carry the same Python object. Move each count once;
do not deduplicate aliases or infer uniqueness. Zero arguments form no B1
read. Preserve the admitted no-default/no-cell/freevar restrictions and
constant-home disjointness from parameters.

The object remains `FRAME_CLEARED`, with NULL `f_executable`, until its complete
frame is initialized. There is no traversal, observer or finalizer between
the first transfer and the final `FRAME_CREATED` store. The shared initializer
only does field stores, checked function-owner extraction and constant XDUPs;
it cannot fail or run Python. Do not introduce a new fallible operation there.

## Canonical owner constructors

The deleted staging code currently converts the pinned code with generic
`PyValue_FromObjectSteal`; removing that entry removes its numeric classifier
entirely. The code owner stays a typed `PyCodeObject *` and transfers directly
to `f_executable` through `_PyFrame_InitializeOwnedCode`.

For the result use one internal known-object stealing constructor, beside
`_PyValue_FromObjectKnown` in `pycore_value_api.h`. Proposed spelling:
`_PyValue_FromObjectKnownSteal(PyObject *owner)`. It requires a nonNULL owned
object that cannot be an exact int/float, checks that precondition in debug,
and uses the existing pointer/immortality encoding without DUP, DECREF or
numeric normalization. Here the actual `make_gen` result is an exact generator
or coroutine, so the source type and constructor supply the precondition.
Do not cast a borrow into an owner or use object-only immortal duplication.

This helper is not an unrestricted `OBJECT -> canonical VALUE` proof for the
DSL. Only introduce a typed-walker receipt when an actual source consumer also
proves its nonnumeric precondition; this C provider remains opaque to ordinary
normal/error replay. No helper-name exception, new owner lattice, borrowed
callee ABI or result-normalization variant is needed for this packet.

## Result root and error states

The argument prefix disappears; the result root does not. After full
initialization and before the return writer, push an existing 40-byte
`_PyTransitionRoots` record with `kind=EXTERNAL`, `prefix=NULL`, no header/home
authority and the single known-canonical result owner. This existing empty
prefix representation is accepted by push/pop and the visitor; it allocates
nothing and adds no fields. The heap caller and completed child provide their
own ordinary container traversal. The record protects only the result while
redirect cleanup may run Python before the destination owns it.

Keep `_PyFrame_SetReturnValueFrom(frame, &roots.result)` unchanged. It retains
the root during `CompleteRedirect`, reloads the current recipient, NULLs the
source at the consuming commit and then publishes or discards. A displaced
local finalizer must see the published child, with no stale external result
owner claiming the same count. Clear `result_present` before pop, as today.
Delete the prepublication `call_transaction_refresh`: allocation and final
initialization cannot run Python, poll or replace the active frame/exec, so
there is no state to revalidate before entering the shared writer. There is
also no remaining fallible operation between successful allocation and that
writer, so remove the old prepublication result-error cleanup branch. Keep the
postpublication refresh and load the callback-current continuation. Do not
introduce action tags, a new writer, or generic CALL's different local-result
schedule.

| Reached state | Owners and error route |
| --- | --- |
| Guard miss | Original homes untouched; existing role-specific fallback. |
| Allocation failure | Original CALL homes own all inputs; extra code pin released, recursion restored; shared error ingress. No result root exists. |
| Successful initialization | Input homes NULL; completed heap child owns arguments/function/code; result-only transition root owns child. |
| Redirect cleanup before result commit | Keep the result-only root linked until the shared writer consumes it; do not pop it or take a second result owner. |
| Publication/cleanup failure | Writer has consumed the result root. Destination or discard owns/retired the result; leave that reached state and report the final refresh error. Never reclaim the published result. |

The current source refresh is not a poll and grants no new map. Scheduled GC
continues to run at the existing supported boundary; no collection is inserted
inside allocation/initialization. The opaque provider's ordinary home summaries
remain UNKNOWN and its learned-only admission and monitoring invalidation stay
unchanged. In particular, input preservation on allocation failure is not a
claim that every creation error preserves input homes.

## Files and gates

Production scope: `Python/ceval.c`, `Objects/genobject.c`,
`Include/internal/pycore_genobject.h`, `Include/internal/pycore_value_api.h`.
Update affected source checks and `Lib/test/test_ir_created_generator.py`.
Keep marker effects and generic RETURN/SEND/YIELD bodies unchanged; no public
format, handler ID, feedback payload, header, allocator ABI or benchmark edit.
If source receipt tests need the known-object constructor, add only its actual
precondition/ownership test, not a global creation-helper effect assertion.

Minimum checks:

- Reuse zero/one/large arity, generator/coroutine, scratch/local/discard, all
  image routes and supplied-self coverage. Add mortal argument/callable aliases
  if not already covered, with post-close weakref release.
- Keep allocation failure injection, and verify that the ordinary error path
  releases the untouched inputs with the correct source traceback and clean
  reachability. Cover zero and heap-buffer-sized arities even though the
  provider no longer allocates that buffer.
- Keep explicit collection **after** completed creation, code/argument pinning,
  origin unchanged on the already-created child, and eventual release.
  `gc_verify` runs after collection returns. Do not force an unsupported
  in-constructor callback or claim GC verification counts owners.
- Keep real displaced-local finalizer reentry, local mutation/redirect,
  discard cleanup, hooks/origin/monitoring invalidation and learned-only public
  image rejection. Original-home error and consumed-home error paths require
  distinct source controls. No new test hooks are needed.
- Focused source checks require no prefix/buffer, one direct input-transfer
  loop, result-only root before the writer, NULL handoff before release, and no
  generic numeric constructor for known code/gen objects. Negative source
  tests must not turn an opaque provider into full normal/error authority.

Root native falsifier: no buffer reservation/fallback, prefix bookkeeping,
pre-allocation root push/pop or second argument move; no code/gen numeric
normalization; one window-to-final-frame transfer per input. Price the surviving
code pin, NULL fill, result-root record, context/epoch guards and one remaining
postpublication refresh.
Compare the whole helper plus both call handlers with round61's 2,475-byte,
328-byte-stack helper, not just a smaller callee. Ordinary RETURN_VALUE,
SEND and YIELD_DELEGATED must retain their selected native protocols, stack
accesses and inlined ordinary writer. Broad screening can still reject the
packet; fewer moves are not a performance result.
