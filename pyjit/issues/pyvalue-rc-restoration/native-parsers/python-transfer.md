# Bounded Python-frame transfer amendment

Accepted bounded design addition; the caller lane may implement these seams.
The existing `_PyEvalFramePushAndInit` / `_Ex` consume their supplied owners and
may close them on an initialization error before the caller clears its homes.
Passing the caller's actual owners therefore makes failure callbacks observe
stale parent slots. Preserve inlining, using independent consumed copies.

## Caller adapter and exact boundaries

Add bounded `call_transaction_push_python` / `call_transaction_push_python_ex`
helpers in ceval.c, using the same instruction-local entry record and captured
indices as native invocation. Use them from `_DO_CALL`, `_PY_FRAME_GENERAL`,
`_DO_CALL_KW`, `_PY_FRAME_KW`, `_DO_CALL_FUNCTION_EX` and `_PY_FRAME_EX`. Their success dispatch
uses a new producer transfer marker rather than flushing old input locals.
Include `_EXPAND_METHOD` and `_EXPAND_METHOD_KW` in the already-reviewed
method-preparation protocol because their specialized callers reach
`_PY_FRAME_GENERAL` / `_PY_FRAME_KW`.

Targeted provider-call classification (all seven bytecodes.c call expressions):

- `_DO_CALL`, `_PY_FRAME_GENERAL`, `_DO_CALL_KW`, `_PY_FRAME_KW`,
  `_DO_CALL_FUNCTION_EX`, `_PY_FRAME_EX`: this transaction amendment. This
  includes warmed CALL_PY_GENERAL and CALL_BOUND_METHOD_GENERAL, not just cold
  calls and keyword/EX specializations.
- `_CREATE_INIT_FRAME`: separate CALL_ALLOC_AND_ENTER_INIT shim route. It
  allocates an instance before changing callable/self and builds a second shim
  frame before the consuming provider. It requires its own preparation/shim
  transaction; do not claim this constructor specialization admitted by the
  ordinary constructor bridge cut. Keep it explicit in qualification blockers.
- `_INIT_CALL_PY_EXACT_ARGS` does not call either consuming provider: guarded
  no-allocation `_PyFrame_PushOverlapping` / `_PushUnchecked` transfers owners
  directly into actual child homes, with no callback between moves and handoff.
  Its separate bound-method preparation needs the same publish/refresh discipline
  wherever that uop is selected; no consuming-copy adapter is needed for the
  pure transfer itself.
- `_PyEval_Vector` is the other ceval.c public-provider caller. It already
  supplies independent owned canonical argument copies; its explicit locals
  parameter is part of that external entry contract. It is not an inlined
  parent-window transfer. Public provider ABI remains unchanged.

Before allocating copies, finish all obsolete preparation locals and monitoring
views while the parent IN_CALL homes remain owning/coherent. Reserve the copy/
retirement storage before any moves. No whole-handler replay is allowed once
allocation/preparation can have effects. Capture a new strong callable owner,
independent canonical vector owners, and the existing required owned globals/
locals reference. The EX adapter instead duplicates the actual tuple/kwargs
container owners; it does not box another tuple. Pin kwnames for the keyword
vector route. Keep **original parent homes unchanged** while the existing
providers consume these copies.

Mark each copied owner consumed before calling a stealing provider. Afterward,
free only the copied array's storage, never its consumed element bits. Every
partial-copy error closes only initialized owners under the first error, then
refreshes the parent before returning failure. On provider failure, original
parent homes still root the evaluated operands throughout provider frame/argument
cleanup. Publish the parent's actual error PC/state; refresh and return to its
normal error path. No child executes and no transfer is claimed.

## Actual success handoff and retirement

1. The provider returns a fully initialized child which independently owns its
   function, executable and bound arguments. Refresh/revalidate the still-current
   parent before reading its homes. If a preparation cleanup error occurs here,
   clear/pop the child under that primary error while parent originals remain
   valid, and return an ordinary parent failure.
2. Nonallocatingly move the parent temporary window owners to the reserved
   retirement slots and clear those homes. Set the parent's **actual return_slot**
   to the captured destination; retain the child's real `previous` link. Publish
   **tstate->current_frame = child** before releasing the parent duplicates.
   This is an actual frame handoff, not a test flag. The child's initialized
   executable/locals/initial PC now own the transferred invocation state.
3. Retire original parent temporary owners, kwnames/tuple/kwargs duplicate
   metadata and workspace under that real handoff, with no stale parent decoded
   data access. The parent remains suspended at its call-site PC with absent
   consumed temporaries and its callback-current locals intact. The child is
   current, initialized and not yet executing its first instruction; ordinary
   introspection may skip this incomplete child, but ownership is real and the
   suspended parent's homes contain no dangling claims.
4. On successful cleanup, leave **no C-only owner or workspace pointer** before
   the generated transfer marker resumes the existing DISPATCH_INLINED/start_frame
   protocol. The emitter still has the old parent C variable for the existing
   `child->previous == frame` assertion; all private views are already drained.
   It must not perform the old window/result publication or replay preparation.
   Cleanup can reinstrument either frame: refresh/revalidate the child's actual
   owned code, executable words/feedback and initial numeric PC **0**, the known
   initialization index established by `_PyFrame_Initialize` / reset, under
   the current child before dispatch; retain the suspended parent's numeric
   offsets for rollback. Do not read the child's old instr_ptr in start_frame
   or LOAD_IP after a callback, and never derive its index by subtracting a
   returned pointer after initialize_locals has already called out.
   Specialized `_PUSH_FRAME` recursion decrement
   and generic start_frame accounting occur only after successful refresh;
   neither was executed merely by publishing the earlier ownership handoff.
5. On cleanup failure **after handoff**, do not enter the child. Preserve the
   first error, restore the actual current frame to the parent and publish its
   captured error PC while its consumed temporary homes remain absent. Clear/
   pop the initialized child under that error and finish retiring remaining
   duplicates/storage. Refresh the parent again and return failure there. The
   child has not passed start_frame, so do not use its normal error-unwind path
   or adjust recursion counters as though its body had started. This avoids
   double consumption and leaves the parent locals/destination unchanged.

The actual return destination is not overwritten by a Python handoff: it receives
the child's eventual result through the existing frame return protocol. If that
destination overlaps an evaluated source local, preserve its callback-current
value until the return assignment; clear only the verified temporary call window.

## One selected code snapshot through preparation and binding

Independent consumed copies allow both stealing algorithms to remain unchanged
for argument ownership. There is an additional existing code-pointer lifetime:
`_PyEvalFramePushAndInit` reads borrowed `func->func_code`, then calls allocating
`_PyCode_GetExec` before `_PyFrame_Initialize` establishes the child's code owner.
An allocator callback can replace `func.__code__`. The EX provider performs
allocating kwargs/vector preparation before entering that helper, so pinning one
earlier code snapshot only in the outer adapter does not cover its later selection.

Pinning only that inner borrow is insufficient: `initialize_locals` currently
independently rereads `func->func_code` after exec/frame allocations. EX also
allocates before the inner provider selects code, after an outer caller already
selected its locals policy. Bind all three decisions to one actual code owner.

Add private static `eval_frame_push_and_init_code` and
`eval_frame_push_and_init_ex_code` helpers in ceval.c, sharing the existing
algorithms and accepting an explicitly pinned `PyCodeObject *code`. After
workspace allocation and refresh, the selected transaction adapter takes that
strong code snapshot and derives CO_OPTIMIZED / owned globals-locals policy
from it with no intervening callback. It passes the same snapshot through EX
tuple/kwargs boxing/preparation into the ordinary helper, `_PyCode_GetExec`,
frame-size allocation and `_PyFrame_Initialize`. Change the sole
`initialize_locals` signature/call to take this exact code parameter; it must
not reread mutable func_code. No copied code flags or earlier speculative code
borrow chooses a frame layout.

The private code parameter is borrowed from the adapter's real strong pin and
remains pinned through the complete provider return, including failure cleanup.
On success the initialized child independently owns that same code. The adapter
holds its pin through actual handoff, then retires it with the other duplicates;
on failure it closes under the first error while parent input homes still own.

The two public consuming providers keep their existing ABI. Their thin wrappers
pin func_code before any provider allocation, call the corresponding shared
code-specific implementation, and close the pin under first-error cleanup.
Their explicit supplied locals argument remains an input, rather than silently
being replaced by an inferred policy. Selected inlined callers instead use the
private adapter path above so their inferred policy and frame code are one
snapshot. In EX, the shared implementation must call the code-specific ordinary
helper, never the public wrapper that would select func_code again.

This is a bounded code-selection seam and binder signature change, not a second
binding algorithm. Mutable defaults/qualnames retain their existing semantics;
broader binder cleanup is outside this cut. Coordinator and peer accepted these exact dependencies before provider edits.

## Evidence boundaries

Require wrong-arity/keyword/exec-allocation failure witnesses with parent homes
still owning; success handoff and post-handoff cleanup failure observations;
monitor changes during initialization/retirement; exact first errors and one
invocation. Observer results come from actual parent/child/frame homes, not a
synthetic publication flag. Existing frame-setup diagnostics outside this bounded
change stay explicit. Candidate execution remains pending the merged build.
