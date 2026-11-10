# Complete frame-commit inline experiment

**Approve the two-annotation experiment before timing, after preserving candidate42's identity and compiled evidence.** Add the existing release-policy `Py_ALWAYS_INLINE` to `_PyFrame_Initialize`, retaining it on `_PyFrame_PushOverlapping`. No body, ABI, ownership, guard, or binder changes are approved here. Use a new candidate revision and build identity. This remains an experiment, not a promotion recommendation.

Candidate42 did not satisfy the original structural prerequisite: its three overlap calls disappeared, but a native `_PyFrame_Initialize.isra.0` call replaced them. The coordinator reports text growth of 432 bytes and larger positional/generic/keyword stack reservations. Those facts do not establish an improvement. Timing42 can be retained as an optional diagnostic; it is not necessary before testing the intended complete inline commit. Record42 as a structurally unsuccessful first experiment rather than silently presenting the extension as its original scope.

## Why this is a coherent boundary

The intended optimized unit is the guarded frame-storage/header initialization commit: reserve initialized storage, update stack bounds, install the frame's owners and execution pointers, and populate constant homes. Argument/default/keyword binding and publishing the child remain the callers' responsibilities. The native frame-construction boundary is what the experiment aims to remove from successful hot overlap paths. It does not remove the Python frame or its ownership boundary.

`_PyFrame_PushOverlapping` delegates its header initialization to `_PyFrame_Initialize`. The latter writes the previous/base/function/code owners, globals/builtins/locals, execution pointers, return fields and state, then fills constant slots. Its nested helpers are:

- `_PyFrame_CodeHome`: duplicates the code owner.
- `_PyFrame_InitIRInstrPtr` and `_PyFrame_SetExec`: load the existing execution record and install its pointers.
- `_PyFrame_FillConstSlots`: duplicates constant values into initialized homes when the code has constant slots.

The outer helper also calls `_PyStackChunk_EnsureFrameStorage`, which initializes uncertain cells before header writes. These are parts of the same no-callback ownership commit, not separate binders or dispatch protocols. Constant count and storage-tail length remain variable; calling this merely “20 stores” understates its work. The annotation changes compile-time placement, not those operations.

Read-only `nm -S` inspection found no named `_PyFrame_Initialize`, `_PyFrame_FillConstSlots`, `_PyFrame_CodeHome`, `_PyFrame_InitIRInstrPtr`, `_PyStackChunk_EnsureFrameStorage`, or `_PyFrame_PushOverlapping` symbols in the frozen39 ELF. The current42 ELF exposes `_PyFrame_Initialize.isra.0` at size `0x115`. Symbol absence alone does not prove every body was inlined, but it is consistent with the worker's baseline disassembly and confirms the newly exposed initializer boundary. The first annotation's result supplies a concrete reason to extend the policy to its delegated header initializer.

## Broader effects must be checked

`_PyFrame_Initialize` is shared beyond the three changed overlap sites. Source consumers include `_PyFrame_PushUnchecked`, generic `eval_frame_push_and_init_code`, and `Objects/frameobject.c:init_frame` for direct heap frames. Exact-argument bytecodes use overlapping entry for thread-owned frames and unchecked entry for generator-owned frames. The second annotation therefore has a broader footprint than a local overlap-only patch.

This breadth is acceptable for the bounded experiment because it keeps one canonical initializer and uses the existing macro, whose `Py_DEBUG` expansion remains empty. It requires explicit scrutiny of all affected emitted paths. Do not characterize the result as touching only logging or only the three originally outlined sites.

The new dynamic coroutine evidence identifies exact-argument calls and generator creation/await/return machinery as major sampled costs. The earlier DIRECT-clear positional-general ledger is valid for that chosen branch but cannot stand in for the dominant application route. Inspect both arms of exact-argument frame entry and the actual generator paths. Do not predict coroutine recovery from an overlap-only explanation, and do not equate sampled self percentages with exact invocation counts.

## Stop conditions and validation

1. Preserve frozen41 performance controls and candidate42's compiled evidence; do not overwrite their identities. Annotate only the shared initializer declaration for the next candidate.
2. Inspect the complete nested commit in the release ELF. The three hot success sites must no longer call a native frame-construction/header-initialization helper. Compare spills, stack reservations, canaries, caller bytes, total text, and generic-overlap early-miss shrink-wrapping. Also compare exact-argument thread/generator branches, generic fallback initialization, and heap-frame initialization. Do not judge success solely by a disappearing symbol.
3. If another substantial nested helper is newly outlined, stop and report the call graph and cost. There is no automatic approval for a third annotation, broad forced-inlining policy, or copying initializer bodies. Small existing leaf/runtime calls or code emitted for rare paths must be assessed on executed cost rather than a blanket “no call instructions” rule.
4. Complete release/debug builds and the existing focused ownership/frame/default/keyword checks. Retain constant-slot behavior, including mortal constants, and the existing defaults proof. No new behavioral test is required solely for the declaration annotations.
5. Only after the structural gate passes, run balanced comparisons against frozen41 and golden39. Logging and keyword cases test the intended successful-overlap cost; coroutine and DeltaBlue remain required regression controls. Evaluate both executed instruction estimates and elapsed time, with raw/net semantics kept distinct. Broader qualification remains required for promotion.

A repeatable application benefit with acceptable text and stack effects justifies retention. Merely restoring the baseline inline shape does not. If the complete policy fails that test, revert the two annotations as an experiment and pursue measured attribution rather than expanding the inline policy again.

This review inspected source and existing ELF symbol tables only. No runtime files, builds, or native benchmark workloads were changed or executed.
