# One successor: an exec-only arithmetic block head

Read-only proposal; no implementation or execution authority. Close the frame-entry experiments. Source11aef's valid full29 GM ratios1.005209/1.004548 and every aggregate sensitivity ratio exceed1 despite proved cached-negative call skipping. Logging_silent still loses11.85%/9.99%. This does not prove remaining frame-guard cost caused those losses.

**Hypothesis:** remove BOTH canonical frame-entry hooks completely, restoring ordinary entry code to the qualified RC source, and enter native code only through a quickened BINARY_OP-family instruction in an executing IR copy. Unrelated code executes no extra frame-entry load, branch or call. Additional handler/text/layout and exec-owner storage can still affect it; zero total overhead is not promised.

## Why dispatch integration before a larger compiler

The existing full integer payload already improves spectral locally; a one-op native stub would discard that evidence. Reuse the full contiguous six-native-arithmetic body (seven source sites with CSE), not a shortened rescue region. The captured eval_A has a natural BINARY_OP_ADD head at word2, ten-word region2..11 and ordinary continuation12. Word1's float literal remains materialized and untouched. The selection rule must discover this arithmetic shape from pristine words, never names, filenames or benchmark identity.

Making a longer region without changing entry still cannot remove rejected-entry work from logging/copy/generator calls. A native loop chunk could amortize a transition over many iterations, but needs loop-carried owners, float/memory operations, branches and bounded breaker service absent from today's scalar leaf. No captured all29 loop establishes that contract. Defer that larger compiler change. Test the full known arithmetic block with isolated dispatch entry first; fixed all29 decides whether its local work amortizes this new transition sufficiently.

## Cold selection and patching

`Python/bytecodes.c:5611` invokes `_Py_Specialize_BinaryOp` only when the existing adaptive counter triggers; its fast specialized handlers carry no generic prefix check. Add optional structural discovery ONLY to that existing cold specialization path (`Python/specialize.c:2669`), after temporary boxes/DECREF work and with fresh code/exec/monitoring validation. Failed discovery caches a structural negative; it adds nothing to normal frame entry or established arithmetic fast paths.

Initially admit only private nonzero FB_BINARY/NB_ADD heads, outside exception tables and generator/coroutine code. Select at most ONE site per code: the lowest structurally eligible pristine offset. Key scalar Plan/eligible64/owner to its first_word; another cold site cannot steal or rebind it. Decode the longest supported acyclic straight-line region, with overflow-safe bounds RELATIVE to its start (at most16 words), not the old absolute word16 limit, and at least three arithmetic sites. Stop before calls, attributes/memory, branches, monitoring boundaries or any backedge. Reject external jump targets entering its interior. Interior words stay unchanged.

On success patch only the head handler bits to one BINARY_OP_NATIVE specialization, retaining original operands/feedback index. Count fully guarded eligibility toward64 ONLY at this selected head; compile/publish the existing native owner there when ready. Before activation it executes the original head operation once. No new global counter or runtime selector. Scalar native-only execution, optional allocation failure and GIL/Linux/x86 scope remain.

## Head semantics and materialization

Use a BINARY_OP-family head, not a universal arbitrary-op NATIVE_BLOCK. Give it the family's operand/FB format and semantic base BINARY_OP. `ir.c:60` BaseOpcode, HandlerFor:246, RealHandler:264, `_opcode.get_ir_formats`, dis and validators must recognize it as an exec-only specialization. The pristine offset retains NB_ADD; no handler-ID escape or hidden variable semantic base.

Reject the new head in pristine code/persisted caches. Preserve existing pristine handler IDs; if generation cannot, stop for an explicit private cache/marshal version decision before source acceptance. Do not silently reuse incompatible pycs.

This is a NEW block-entry contract, not old TryEntry's word1/RESUME assumption. Inputs are actual materialized head operands/live-ins. Require exact compact ints and NULL writable temporary homes; preserve untouched owners, including materialized float home2. Locals stay owned/unchanged. Derive exit homes from block liveness/versions; commit only live scalars, with dead overwritten homes proven NULL. Decline an owned destination; never DECREF inside commit. The known new Plan has only int home3 at outputindex0/scratch18; old float+int Plan used index1/scratch19. Reuse compiler/ABI, but verify regenerated full leaf/output mapping rather than require old94-byte identity.

Publish the original head instr_ptr and fully materialized stack pointer through the normal generated boundary BEFORE cold work. Guard code/exec/word/head identity, observers, tracing/profile/eval-frame, pending exception/breaker, inputs and writable homes; repeat after promotion. Success commits required homes and last-completed instruction/continuation PC consistently. Decline commits nothing, preserves original-head materialization/owners/error and executes ADD exactly once through a shared generated operation body. No repatch/redispatch loop. Overflow/custom-dunder/error behavior remains reachable.

`JUMP_BACKWARD` retains `_CHECK_PERIODIC` (`bytecodes.c:3349`); NO_INTERRUPT await/yield paths remain untouched. The first block has bounded straight-line work and no skipped safepoint/backedge. It does not claim native loops or asynchronous observation inside native code.

## Restore and lifetime closure

Instrumentation uses BaseOpcode(word) and restores with HandlerFor(pristine offset) (`instrumentation.c:82,89`). Before instrumentation rewrites/disable, restore the ordinary head and prohibit native entry; keep conservative permanent disable after actual observation. Retain the owner until clear/deregistration permits freeing. Restore before munmap on failure/clear and recheck after cold reentry. No patched word may reach a retired owner. Interior words remain available for tracing/deoptimization; preserve source line/exception identity.

## Exact scope and deciding sequence

Touch only the existing scalar/block owner+entry code, canonical/generated arithmetic handler, cold binary specialization, shared IR/opcode metadata/readers, instrumentation restoration and existing CAPI/test_ir fixtures. Full metadata generation is required; this is broader than moving a frame guard, but not a new IR optimizer, allocator, encoder or interpreter backend.

1. Peer review the explicit head/fallback/materialization/restoration contract, then a separate finite source grant. First prove whole ordinary frame-entry source equals qualified RC with both old hooks absent.
2. Reuse release/debug54 with focused extensions: actual exec-only patch/restore, successful varying inputs, late overflow/dunder fallback once, owned-destination decline, empty/invalid FB, interior-target rejection, monitoring/tracing callbacks, cold failure and lifetime. Verify complete generated metadata closure and native bytes/ABI; no extra census project.
3. Inspect actual ordinary entry and selected head CFGs. Prove no admission code on unrelated frame entry, native success at the actual registered arithmetic site, fallback executes once, and complete owner/RX restoration ordering. No dynamic-frequency or loss-recovery claim follows.
4. Go directly to one fresh predeclared full29 against the SAME qualified RC image, existing116/580+58/174 controller and unchanged GM29/290/spectral10 gates/material-loss disposition. No four-workload pilot, old-score replacement or adaptive region rescue. Publication/fullLib remains held unless that broad result is useful.
