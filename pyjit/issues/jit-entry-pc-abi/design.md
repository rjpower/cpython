# Separate by-value continuation hypothesis: exact NULL-decline interface

Design only. This version supersedes the initial unchanged-pointer decline proposal; the coordinator has frozen NULL on every decline. Closed source67f5, its failed aggregate result and its original90-minute source grant stay frozen. This document grants no source change, compiler execution, target invocation or measurement. The accompanying finding supplies static addresses and preserves the distinction between changed machine code and an unproved performance cause.

Replace the current exported internal declaration in BOTH Include/internal/pycore_jit_prefix.h and Include/internal/pycore_ir.h with:

```c
PyAPI_FUNC(_PyIRWord *) _PyJitPrefix_TryEntry(
    PyThreadState *tstate, struct _PyInterpreterFrame *frame,
    _PyIRWord *next_instr);
```

In Python/jit_prefix_entry.c, every decline returns NULL; debug NoteDecline behavior may remain in a helper returning NULL. The unsupported-profile implementation also returns NULL. Success still commits the two existing NULL destination homes only after the same scalar/native result succeeds, records the same one-shot/debug success fact with NoteSuccess, and only then returns exec->words + active->continuation_word. It does not change frame->instr_ptr or any observer/error/ownership rule. The full initial/observer/input guards, cold publication and post-cold rechecks are retained. There is no out-pointer to the evaluator IP local.

The only bytecodes.c hook change at _PUSH_FRAME and start_frame is:

```c
_PyIRWord *prefix_next = _PyJitPrefix_TryEntry(tstate, frame, next_instr);
if (prefix_next != NULL) {
    next_instr = prefix_next;
}
```

Both remain after the existing frame-entry notifications and before ordinary DISPATCH. There is no second increment and no redispatch loop. Regenerate all seven products; expected generated_ir_cases.c.h change is confined to these14 expanded hook sites, with handler IDs, bodies outside those lines and conservative escape metadata preserved. Any additional generated change must be explained before freeze. The private Plan, 94-byte selected native leaf, mapping owner, threshold, decoder, opcodes, feedback and compile configuration are not redesigned. No Makefile or assembler change is intended.

The direct CAPI existing-frame fixture in Modules/_testinternalcapi.c keeps its incoming pointer, saves the returned pointer separately, derives entered = (returned != NULL), and updates its local next only on non-NULL. The same validated Plan supplies a continuation inside its executing word array, strictly after word1; NULL never names a successful continuation. The fixture keeps its owned public frame, current/previous restoration, pending-error marker, reserved breaker bit, argument-owner and full-home snapshots. It still reports the actual continuation difference and original IP. This is one entry implementation, not a status-producing fallback or runtime selector.

Keep the same twelve focused test methods. Existing positive fixture asserts continuation12, IP0, exact committed homes and unchanged argument owners. Existing occupied/NULL/noncompact/pending-error/breaker/noninitial/overflow cases must assert entered is zero, the local continuation remains exactly the incoming pointer, and every owned/frame/error snapshot is unchanged. Within an existing direct method add a structurally rejected short-arithmetic code case, requiring the same NULL-result/unchanged-local-continuation transaction. Preserve automatic fresh-code success-seen, interpreter continuation errors, dunder behavior, one-input plan, observers, pressure spills, all cold mapping failures and executable-owner lifetime. Existing42 integration controls remain the finite behavioral scope; this document adds no method roster or broad qualification.

A separately granted source worktree and fresh matched build would test whether eliminating IP address escape lets GCC retain ordinary-handler IP in a register. Inspect the same complete evaluator and branch-followed LOAD_FAST_BORROW/STORE_FAST/FOR_ITER_RANGE/UNPACK paths, while verifying all14 interception calls and the same leaf still exist. Register recovery is evidence about the proposed mechanism, never a size/instruction proxy for speed. If the address/traffic change is absent, the intended mechanism has not been isolated. If present and correct, only a separately predeclared fixed elapsed comparison can test the cost claim; the old result remains rejected and supplies no replacement samples. No source or execution authority follows from this design.
