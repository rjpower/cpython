# Recovery interface design review

**Decision: revise the new interface section before treating it as an
executable contract.** The proposed direction is compatible with the real
register IR, and it correctly labels the whole-method JIT, `JitContext`,
`JitAction` and native recovery as planned. The added `FrameState` distinguishes
ownership from future-read liveness and rejects replay after effects. Four
handoffs still need exact rules. This review is of the uncommitted addition to
`pyjit/design/jit-e2e-system.md` on 2026-11-07, against runtime
`25ce1654801`; it does not approve a backend implementation.

1. **Give actions an exact transfer contract.** `EnterFrame(child,
   parent_return_slot)` does not say whether `child` is fully initialized,
   who owns its frame storage and argument homes before the trampoline takes
   it, or when the parent call window is detached. Existing
   `call_transaction_overlap_python()` and the consuming fallback in
   `Python/ceval.c` set `tstate->current_frame` before they return, and the
   latter can release retired owners immediately after publication. They
   cannot be called unchanged by a helper that promises the trampoline alone
   changes the current frame. Specify a new adapter boundary: preparation and
   binding may fail with the parent still owning its homes; a committed child
   and transferred owners must be published before any release that can run
   Python; the trampoline performs each frame switch, recursion/monitor event
   and parent return-slot write once. Define `Return(value)` and `Yield(value,
   resume_word)` as transfers of **one owned PyValue**, including the recipient
   home and failure cleanup. The present `_RETURN_VALUE` unlinks the dying
   frame before clearing it, then writes the held result to the parent;
   `_YIELD_VALUE` also updates the generator's `return_slot`, exception stack,
   suspension state and parent PC. These are action-specific obligations, not
   implied by the two payload words. `JitContext` should *carry* a non-owning
   current-frame handle; the frame has its own `owner` and storage lifetime.

2. **Separate fault PC, resume PC and handler entry.** `Raise(error_word)` is
   ambiguous: the current `error` path attributes the fault to the executing
   word, emits traceback/raise monitoring, and then finds an exception-table
   handler; `RERAISE` can replace `instr_ptr` with a saved last instruction
   before unwind. The handler entry is not the same as the fault or next word.
   Define `Raise` with a pending raised exception and its fault attribution,
   and specify whether the shared unwinder performs monitoring and handler
   selection or receives an already-handled state. A normal `Deopt` after a
   completed effect resumes at the next semantic word with its result and
   owner state; an error exit from that effect unwinds from the fault word
   without retry. A before-effect exit is legal only if no irreversible part of
   the operation has run. Make the phase and completed-effect prefix explicit
   at every exit, including a guard failure after a callback or release.
   `_PyIRFormat.effects` is a whole-handler summary and cannot supply this
   ordering by itself.

3. **Define failure during recovery without losing the old state.** Phase one
   says to reserve/materialize anything fallible while the native activation
   owns its values, but allocation or reference release can itself run Python
   and change locals, executable identity or exception state. Require the
   recoverer to keep a valid published frame/roots before such a callback,
   revalidate afterward, and commit homes only after all fallible work has
   succeeded. For the first implementation, canonical homes at every helper
   and effect boundary make this tractable. For later unboxed/inlined values,
   require a specific failure route for each allocation after an effect: the
   completed effect stays completed, the new error has a defined fault PC and
   precedence over any pending error, and every original/temporary owner has
   exactly one cleanup path. Otherwise prohibit that materialization after
   commit and reserve it before the effect. The design's phrase “pre-effect or
   already-declared error state” does not settle this case.

4. **Make frame snapshots and invalidation checkable.** Every physical frame
   home must contain a valid PyValue or explicit NULL, including temporaries;
   observable locals, exception-edge owners and future-read values are
   different obligations. `FrameState` should state which homes are real and
   which are virtual, who owns each non-immortal value, and how duplicate
   locations share one owner. Track the raised exception separately from the
   handled-exception stack (`tstate->exc_info`), and do not freeze a handler
   target when `RERAISE` or a callback may change the fault position. After an
   opaque callback, release, monitor event or allocation, recheck the actual
   frame, executable words/feedback and invalidated dependency versions
   before using a decoded pointer or optimized fact. An OSR entry needs the
   same owner/NULL map at the actual loop header, after its required periodic
   work; a yield needs canonical generator homes and a logical resume word,
   with no native-stack dependency. These statements should be explicit in
   the interface's validation obligations.

**Small admission slice.** The proposed “baseline statepoint checker” currently
depends on generated callable helpers and a baseline JIT that do not exist.
Start with a bounded, reviewable contract fixture over current pristine IR:
one ordinary call with success/raise/decline outcomes and one result-retirement
case with a callback. Decode their actual word/role metadata, and write the
expected before-effect, after-effect and error home maps from the source
handlers, including NULLs, an owned result, a borrowed view's named owner,
fault/resume words and parent return slot. A small offline checker can reject
duplicate owners, an unowned borrow, missing NULL/owner homes, a before-effect
resume after the recorded callback, and a success action with an exception.
Keep this fixture local to the experiment, without a new runtime framework or
a second opcode-semantics table. Its pass would validate the proposed contract
shape only. Admission of *native recovery* must later force exits from an
actual compiled slice and compare visible frame/exception/finalizer outcomes
against the interpreter, including call error, post-effect guard failure,
generator yield/resume and OSR entry. This distinction prevents static metadata
from being mistaken for working deoptimization.

No runtime source, build, test or benchmark was changed or run for this review.
