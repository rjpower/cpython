# Candidate43 timing and owning-generator proposal review

## Candidate43: approve bounded timing

Verified clean runtime HEAD `47dc23f48cf1470d4a98137beb5d313b5093033d`. Its header diff from candidate41 contains exactly the two approved `Py_ALWAYS_INLINE` declarations on `_PyFrame_Initialize` and `_PyFrame_PushOverlapping`; their bodies and ownership contracts are unchanged.

Approve bounded timing after the active focused checks and broader compiled-code review succeed. The coordinator reports both release/debug module gates passed and the outer/header native boundaries disappeared. The conditional `_PyFrame_FillConstSlots` helper remains out of line at size `0x70`. This is an explicit retained boundary when `co_nconstslots != 0`, not an entirely inlined commit. Do not add a third annotation.

The conditional fill retains canonical owners: its loop duplicates constants into initialized homes, including mortal replacement constants. Moving that unchanged loop behind a native call does not create a callback or ownership release. Its call/spill cost still counts, and may affect workloads with constant slots. Inspect its callers and include full-call elapsed/instruction effects rather than counting only the removed outer calls. The reported +744 text bytes versus41 is a cost, not proof of benefit. Existing exact-argument, generic fallback, and early-miss code-shape checks remain required. No automatic promotion follows from this approval.

## Owning-generator entry: approve investigation, not implementation yet

`owning-generator-entry-proposal.md` identifies a structural opportunity supported by current dynamic evidence: constructing the heap-owned resumable frame directly could remove a temporary thread-frame reservation, initialization/copy, and RETURN_GENERATOR dispatch. It keeps the ordinary frame/home ABI and existing result transaction. This is more directly relevant to the non-yielding recursive coroutine workload than a SEND recipient tweak.

The proposal correctly refuses to treat construction/allocation self samples as removable cost. Allocation, argument ownership, constant homes, first resume, and final clearing remain. Its narrow initial admission—validated RETURN_GENERATOR at word zero, exact positional shape, no closure/cell prefix or observers, no async generator or origin tracking—is appropriate. Census and a concrete provider/shell proof should precede runtime implementation.

Source checks covered RETURN_GENERATOR in `Python/bytecodes.c`, `_PyFrame_Initialize` and `_PyFrame_Copy`, `make_gen`, `_Py_MakeCoro`, `gen_traverse`, `gen_finalize`, and `gen_dealloc` in `Objects/genobject.c`, and the generic code-specific provider in `Python/ceval.c`.

### Required details for the implementation proposal

1. **Select the actual hot provider.** Profiles show exact-argument calls prominently. `_PyCallResult` supporting an immediate result does not mean the exact-argument bytecode currently accepts one: its existing path initializes a child frame. Identify precisely where eligible exact calls enter the shared result/retirement transaction, what owner homes it consumes, and what extra branch/cost applies to ineligible ordinary calls. A route only reachable through generic fallback could miss the target workload. Do not duplicate binding semantics to reach it.

2. **Allocate from pinned code, not mutable function metadata.** Existing `make_gen(func)` rereads `func_code` for slot capacity, and `_Py_MakeCoro(func)` rereads it for kind. The new provider must derive type, physical slot capacity, const layout, and continuation from its pinned/validated code. After any fallible preparation, refresh the current call window and function/code/provider/observation proof before consuming it. Reusing `_Py_MakeCoro` unchanged is not a sufficient shell design even when origin depth is zero.

3. **Specify every shell field and cleanup state.** `gen_traverse` skips frame homes when FRAME_CLEARED but still visits executable, names, exception value, and coroutine origin. `gen_dealloc` temporarily retracks the object, calls finalization, and clears subtype fields. Merely calling a shell untracked or FRAME_CLEARED is insufficient if it owns hidden frame values or contains uninitialized destructor fields. Give a field/state table for allocation failure, precommit miss, commit, and publication. Before commit, no frame owners may be hidden across a collection safepoint. A failed shell must be safely destructible without warnings or user generator execution.

4. **Initialize heap-specific links explicitly.** `_PyFrame_Copy` currently clears `previous` and moves to `owned_slots`; `_PyFrame_Initialize` instead installs the supplied previous pointer and initially marks thread ownership. Direct heap construction must end with `previous == NULL`, base at the embedded owned slots, generator ownership, correct exception links, and the real suspended instruction/initial recipient before publication. Never use a stack pop/restore path on that heap frame. State whether saved-top and other nonapplicable fields are initialized defensively for all traversers/introspection consumers.

5. **Prove entry observation and recursion ordering.** The final-IR descriptor must remain valid for the executing record selected at commit, including reinstrumentation and code replacement. Empty-prefix eligibility must reject any observable pre-creation work, not merely absent source-level statements. Preserve call-time recursion admission and current exception behavior before returning a created object, while first-resume admission remains separate. Identify how checks and allocations are ordered; skipping the construction frame must not bypass recursion limits. Deferred MAKE_CELL/COPY_FREE_VARS/genexpr support is correctly excluded.

6. **Keep all precommit effects and postcommit failures explicit.** An allocation or validation failure retains the existing caller window for one error transaction. A successful commit moves each source owner once and publishes the complete object before retirement can invoke finalizers or redirect. A post-transfer miss cannot fall back to the existing binder. The active exception and a newer redirect must survive cleanup. This is a Python behavior and lifetime-safety requirement, not an insistence on identical incidental finalization timing.

### Admission evidence

First count validated eligible creations by caller owner, exact argument shape, and observation state. The recurrence estimate and profile percentages do not establish actual new-provider hits. Show the complete old/new weighted creation sequence, including moved work, shell setup, proof checks, and extra admission branches on non-generator calls.

The proposal's behavioral matrix is appropriately broad: creation/first send/throw/close, warnings, recursion, allocation failures, code mutation, introspection, observation fallback, ownership destinations, and unsupported prologues. Add explicit evidence that exact-argument recursive calls hit the provider and that allocation-triggered mutation either refreshes safely or falls back once. Actual constant-slot and mortal-constant cases are also relevant to direct initialization.

This lane can justify a bounded implementation after those proofs and census are reviewed. It does not presently authorize a second binder, a universal resume dispatcher, or a wholesale generator redesign. Candidate43 qualification remains independent.

This review changed documentation only. No runtime edits, builds, or native workloads were executed.
