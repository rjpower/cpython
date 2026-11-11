# Option 5: normal-completion frame cleanup

Status: proposed; `BASE/worktrees/round51-option5` unchanged at bf810. This
implements Astra option 5, not direct generator construction. Proposal review
precedes source edits; implementation deadline is 23:30 UTC, 2026-11-11.

RETURN_VALUE already detaches its canonical return owner, asserts scratch empty
and unlinks the dying frame before retirement. Generator/coroutine cleanup then
scans every slot through `_PyFrame_ClearLocals`, although scratch is proved NULL.
Thread cleanup already clears locals only. Carry that established normal-return
fact through the existing shared cleanup, with no new frame state or owner map.

Add `_PyEval_FrameClearAndPopCompleted(tstate, frame)` as the explicit capability
entry used only by `_RETURN_VALUE`. It asserts scratch empty in debug and enters
the same profile-update, owner dispatch, generator state/exception unlink and
retirement path as `_PyEval_FrameClearAndPop`. A shared internal implementation
takes a boolean capability; the existing generic entry passes false. The
generator helper passes this fact to a narrow completed-frame clear entry.

In `frame.c`, share the entire frame-object detach/takeover and owner retirement
body between `_PyFrame_ClearExceptCode` and a new
`_PyFrame_ClearCompletedExceptCode`. The completed entry supplies the scratch
proof, so ordinary nonescaped cleanup calls `clear_slots(co_nlocalsplus)`.
Generic non-thread cleanup still calls `_PyFrame_ClearLocals` over every slot.
Both paths retain the existing reverse order and NULL-before-close behavior,
then clear f_locals and f_funcobj as before. Thread cleanup keeps its existing
locals-only rule. Code ownership and release order are unchanged.

Frame-object takeover remains before slot retirement and copies the complete
frame using `_PyFrame_Copy`, including all slots, regardless of capability.
Do not optimize that copy. Generator FRAME_CLEARED publication, exc_info unlink,
critical section and exception-state clearing remain in their current order.
Set `generator_return_kind` only after the last escaping operation as today.
Return owner remains C-owned across all retirement callbacks and is published
by the existing IR_RETURN_TO_CALLER continuation/recovery machinery.

Generic unwind, entry/binding failure, suspended generator deallocation,
explicit close/throw and coroutine cancellation retain the generic entry and
full non-thread clearing. A close/throw that executes an actual RETURN_VALUE
may use its own proven normal-return capability; mere generator state or
ownership never grants it. No direct generator-construction, entry copy,
source/observer, return-recipient or calling-convention change.

Files: `Python/bytecodes.c` normal return call; declarations in
`Include/internal/{pycore_ceval,pycore_interpframe}.h`; `Python/ceval.c` shared
dispatch; `Python/frame.c` shared clear implementation;
`Tools/cases_generator/ir_generator.py` exact return-body proof and existing
known clearing-call classification; focused generator-completion regressions.
Coordinator owns regeneration and native builds. Positive canonical-return proof
still matches the whole body; changing the expected helper name does not relax
any input, observation or ownership requirement.

Focused evidence: normal generator and coroutine results aliasing locals;
mortal locals/closure cells and finalizer reentry; escaped frames retaining
locals; monitoring return/retirement; large scratch requirements. Also exercise
suspended close/throw, exception unwind and cancellation with owners live in
expression scratch, proving those owners retire. Existing generator/frame/
coroutine/monitoring and canonical-codegen tests remain applicable. Native
completed generator clearing must use co_nlocalsplus, with generic clear
retaining total-slot bound. Root owns release/debug checks, independent post-
review and matched generators/coroutines/comprehensions plus logging guards.
No speedup or qualification is claimed before that evidence.
