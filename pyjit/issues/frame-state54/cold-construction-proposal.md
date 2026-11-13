# Cold public generator construction on golden51

Proposal only. Base is immutable golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`; reviewed extraction source is R2
`fcc82220ddd42fc49ffd1885dcd79e6964889703`. No runtime worktree was changed,
and this worker ran no native code, build, generator or test for this proposal.
Root must create an isolated candidate after independent peer approval.

Extract only the public `PyFrame_New -> Py{Gen,Coro,AsyncGen}_New` correctness
fixes and their direct tests. Golden51 does **not** need NONE, recipient bits,
different activation initialization, or a different publication protocol to
make these fixes coherent. There is no performance claim or admission here.

## Why the existing hot contract suffices

Golden51 `_PyFrame_Initialize` sets `return_slot = 0`. Leave it unchanged.
Its `_PyFrame_SetReturnValue` already checks raw `return_slot == IR_NO_DST`
before indexing a home and closes a discarded input. Its ordinary
RETURN_GENERATOR records the instruction's destination before publishing a
CREATED generator; `Objects/codeobject.c:1217` requires that destination to be
IR_NO_DST. Thus the existing baseline already has the exact first-input discard
mechanism required by cold public construction.

The public helper bypasses RETURN_GENERATOR. Set its source frame's raw
`return_slot = IR_NO_DST` before the existing two-argument `_PyFrame_Copy`.
Copy transfers that value unchanged. The first permitted None input is then
discarded by the original baseline helper; no slot-zero owner is created and
no initialized temporary invariant is violated. Non-None first-send rejection
remains unchanged. Subsequent YIELD records the ordinary resume destination.

This is an explicit constructor action, not a fresh-activation default. There
is no change to ordinary calls, SEND, return/yield publication, interpreter
entry/trampolines, debugger admission, remote decoding, or initialized-home
rules. Keep golden51 `_PyGen_YieldFromRef` raw numeric accesses unchanged.

## Exact cold changes

Only three runtime/test files are needed:

* `Objects/genobject.c`: `gen_new_with_qualname`, `PyCoro_New` and
  `PyAsyncGen_New`. Extract the R2 hunks for these functions only, excluding
  R2's `_PyGen_YieldFromRef` descriptor port.
* `Modules/_testcapi/frame.c`: new `frame_new_generator` helper, calling the
  existing frame constructor then selecting PyGen_New, PyCoro_New or
  PyAsyncGen_New from code flags.
* `Lib/test/test_capi/test_frame.py`: direct public-construction regression
  methods and their support/weakref imports.

No `Objects/frameobject.c`, header, pystate, interpreter, DSL, generator,
opcode, source-map, magic or generated-product change is necessary. Do not
extract R2's first-native-callback debugger regression: it qualified NONE,
which this candidate does not contain. The fresh ordinary-frame clear test
also need not be extracted; the three constructor regression methods are the
direct qualification for this fix.

### Commit and owner ordering

The current helper copies a NULL `frame_obj`, asserts it equals the supplied
frame object, and lacks the new association's owning reference. Root reproduced
that impossible assertion on golden51. Correct it without changing the common
frame-copy API or the direct `f_locals` layout:

1. Allocate an untracked subtype object. Allocation failure consumes the
   constructor's stolen input frame reference, as today.
2. For a coroutine with origin tracking enabled, compute the origin before
   moving any source owners. On failure directly GC-free the still-untracked,
   uninitialized generator allocation and consume the stolen frame reference;
   do not invoke generator destruction over uninitialized fields. There is no
   source mutation on this path.
3. Under the input frame's critical section, install the raw discard input
   contract, move with the existing two-argument Copy, set CREATED and
   GENERATOR ownership, and acquire an owning frame-object reference with
   `frame->frame_obj = Py_NewRef(f)`.
4. Initialize weakrefs, exception linkage, origin/finalizer, async flags, name
   and qualname before publishing `f->f_frame` or GC-tracking the generator.
   None of these post-move operations allocates or invokes Python callbacks.
   Track only after the complete association and subtype state are coherent.
5. Leave the critical section, then release the stolen input frame reference.
   Direct `f_locals` ownership moves with Copy; escaped-frame takeover and
   eventual frame.clear keep their original baseline ownership protocol.

PyCoro_New and PyAsyncGen_New must return the common helper directly. Leaving
their old initialization in place would recompute and leak the origin, restore
a fallible operation after publication, and mutate subtype fields after the
coherent commit. These wrapper simplifications are part of the cold fix.

## Qualification and limits

Extract the three existing R2 constructor tests. They exercise:

* Generator frame association, wrong first input then send(None), first yield,
  returned value/close, namespace retention across copy and escaped-frame
  takeover, and release after frame.clear.
* Coroutine frame association, first send(None) completion, and origin tracking
  disabled/enabled at depth 2, including the creating Python frame's name.
* Async-generator frame association, first input, first-iteration hook, asend
  after suspension, close, and absence of finalizer-hook invocation after close.

Root should additionally qualify allocation/origin-failure ownership using the
repository's existing failure-injection facilities if a bounded reliable probe
is available; the extracted tests cover successful origin creation, not this
failure path. Failure evidence must distinguish failure before frame creation,
subtype allocation and origin construction. Source ordering alone is not a
claim of executed failure-path coverage.
Golden51 already provides `_testcapi.call_with_nomemory(start, stop, func,
*args)` in `Modules/_testcapi/mem.c`; it arms allocation failure immediately
before a prebuilt C call and always removes the hooks afterward. Prefer that
existing facility with the new construction helper over a new production
failure-injection branch. A bounded allocation sweep still needs stage evidence
before describing a failure as specifically occurring in origin construction.

Run focused release/debug C API, generator/coroutine/async-generator and escaped
frame/GC checks under root's execution window. Inspect the final three-file
diff against golden51 and confirm hot headers/handlers/publication remain
identical. Retain original R/R2 and all rejected performance evidence. This
separate correction addresses a reproduced public-construction bug and makes
no promise of throughput improvement.
