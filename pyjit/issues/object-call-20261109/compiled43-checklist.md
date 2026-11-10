# Complete inline-commit experiment: compiled checks

Root preserved42's source/ELFs/disassembly/identities in `E/structural42` after
its incomplete inline result. Root reports273 focused checks pass on each42
build. The independent [extension review](frame-commit-inline-extension-review42.md)
approved adding only `Py_ALWAYS_INLINE` to `_PyFrame_Initialize`, retaining the
outer annotation. That one declaration edit is now prepared. No additional
runtime body, proof, ABI, generated product or test change was made; this worker
performed no build, native run or commit. Successor identity and evidence belong
to root. Compilation and all43 outcomes are pending.

The second annotation reaches every initializer consumer, so inspect a wider
set than the three original overlap sites. Compare43 with retained41, archived42
and golden39 using existing ELFs and normalized disassembly, keeping source
and build identities explicit.

| Consumer | Required emitted-code check |
| --- | --- |
| Positional general, keyword Python and generic overlap | The initialized-storage/frontier/header commit is inline, with no substantial frame-construction or initializer helper substituted. Existing guards, defaults, allocation-before-detach and final publication remain. |
| Generic overlap early owner miss | Owner compare/branch still precedes rbp/register saves and stack reservation; record exact early-path instructions and native return island. |
| Exact-argument ordinary/bound-method calls | Inspect thread-owned overlap and generator-owned unchecked arms separately. Compare bytes, instruction/register/stack-home assignments, stack reserve, canaries and native call sets; the actual coroutine profile makes the unchecked arm important. |
| Other unchecked consumers | Inspect `BINARY_OP_SUBSCR_GETITEM`, property getter and overridden-getattribute entry handlers: each calls `_PyFrame_PushUnchecked` in its producer, which delegates to the annotated initializer. |
| Generic `eval_frame_push_and_init_code` | Confirm ordinary/vector/native adapters still prepare storage, initialize owners/constants, invoke consuming binding and preserve partial-child cleanup; compare its compiled footprint and calls. EX delegates through the same provider. |
| Direct heap frame initialization | Inspect `Objects/frameobject.c:init_frame` and its inlined caller `PyFrame_New` if no standalone symbol remains. Keep the complete owned-slot NULL fill before initialization, frame/code/function owners and constant homes. Escape/generator moves do not gain an extra initializer. |
| Generator creation/return/await | Compare the profiled RETURN_GENERATOR, make_gen, GET_AWAITABLE, CompleteGeneratorReturn and SEND_GEN paths for collateral codegen/layout changes. Distinguish unchanged instructions from unchanged cycle cost. |

The source initializer's direct callers are `_PyFrame_PushUnchecked`,
`_PyFrame_PushOverlapping`, `eval_frame_push_and_init_code`, and direct heap
`init_frame`. The unchecked producer sites are in `Python/bytecodes.c` at
getitem, property, overridden getattribute, and exact-argument entry. Use the
actual current symbol mapping after compilation rather than assuming every
static function remains separately named.

Record total text/data/bss and each hot caller's stack reserve, static rbp
references and canary count. Those static references are not dynamic spill
counts. Initializer/constant-fill and frontier loops must retain mortal-owner
duplication and initialized storage; no guard may move after an owner detach.
The `Py_DEBUG` macro expansion remains empty; root still compiles and executes
the debug ownership/default/keyword tests.

If a third substantial helper becomes outlined, stop and report its callers,
prologue/epilogue and estimated executed exposure. Do not apply another
annotation automatically. Existing leaf/runtime operations on rare paths must
be assessed by work and frequency, not by demanding that no call instruction
exist. Only after the structural review and root's focused checks pass should
balanced logging/keyword timings proceed, with coroutine/DeltaBlue controls.
This checklist grants no performance result or promotion.
