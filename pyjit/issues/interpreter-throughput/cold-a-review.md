# Integration A: independent cold source review

2026-11-06. **Accept the integrated increment; no must-fix regression found.**
Reviewed runtime `8a8fdc91368` against predecessor `6a0e4c12c17` at
`python-boil-the-ocean/build/worktrees/pyvalue-rc-universal`. Source line
numbers below refer to that runtime commit, not the documentation checkout.

The first pass read the actual diff, transaction helpers, generated CALL and
attribute handlers, frame primitives, and compaction/verification before
reading this packet's proposals or previous reviews. The subsequent contract
comparison agrees with the selected prepare/guard/commit and all-slots-valid
rules. No source, generated product, build or benchmark was changed. No
runtime, tests, generators or expensive analysis ran during quiet timing.

## Should fix

1. **Make the slow provider's assertion cover its declined-adapter result.**
   `Python/ceval.c:3157` defines status 1 as a permitted decline, with
   `child == NULL` initialized at 3162. The final adapter guard at 3178–3180
   explicitly jumps to cleanup without changing either value. Nevertheless,
   the assertion at 3265–3266 accepts only negative status before dereferencing
   `child`. A valid decline therefore faults in assertion-enabled builds before
   reaching the intended native fallback at 3284–3285. This defect is already
   present in the predecessor; it is not introduced by Integration A.

   Restrict the executable-identity assertion to a successful push, while
   retaining the existing checks for failed and declined calls. The source
   establishes the inconsistent branch; this review does not establish an
   ordinary Python reproducer. In particular, `gc.c:2018–2036` schedules GC
   rather than synchronously running `gc.callbacks` inside `PyDict_New`.
   Testing the branch needs a controlled native preparation/allocator hook or
   an equivalent existing facility that really changes adapter selection at
   that boundary. Do not describe a normal GC callback test as coverage unless
   its execution at the intended boundary is demonstrated.

2. **Keep remaining publication work bounded to the proven duplicate.**
   The five specialized loads now have one pre-retirement publication, but
   retain a second temporary publication at handler exit. For example,
   `generated_ir_cases.c.h:13125` emits `IR_STORE_TEMP_DST`, and 13147 emits
   `IR_STORE_DST`; the latter's non-local arm writes the same value again
   (`ceval_macros.h:575–590`). The receiver retirement/refresh between them
   prevents assuming GCC can eliminate the second store. Generic attribute
   completion already has a narrower postcondition:
   `ir_generator.py:2434` sets `published_temporary_result`, and 2083–2089
   emits the final store only for a local destination.

   This is a remaining source opportunity, not a correctness defect or a
   measured cost. First inspect the relevant GCC path. If material, give the
   selected specialized retirement boundary an explicit unchanged-output
   publication postcondition, reusing the existing mechanism only after its
   branch/copy/invalidation behavior is reviewed. Preserve publication before
   receiver destruction, the folded-local store afterward, and error cleanup
   of a C-held local result. Avoid a general distributed publication cache for
   this one sequence. Require generated-output and callback/local-destination
   regressions before admitting it.

3. **State the overlap ownership range precisely when changing initialization.**
   `_PyFrame_PushOverlapping`'s comment at `pycore_interpframe.h:520–525`
   says slots above `base` are NULL, although its argument prefix deliberately
   contains owners. The intended range begins at `base + nargs`; during child
   execution those physical cells are child homes, even though they also lie
   in the parent's full array extent. The source already implements transfer
   correctly. Tighten the comment alongside the pending initialization packet,
   so future GC/validity work cannot mistake aliases for additional owners or
   treat the supplied arguments as known-NULL storage.

## Ownership, observation and structure

The new route has a coherent commit boundary (`ceval.c:3112–3131`). Its entry
precondition is established by `call_transaction_invoke:3305–3315`. It pins
selected code across kwargs preparation, refreshes and rereads mutable
function/default state, and releases the pin safely while the revalidated
function still owns code. Default duplicates, immortal empty varargs, frame
initialization and kwargs owner transfer cannot escape on this guarded GIL
route. The original function owner moves into the child; argument owners keep
their physical cells. Unsupported shapes leave the original window owning.
The precommit cleanup refreshes after releasing the code pin. No fallback
follows detachment.

Instrumented calls, non-thread callers, keyword/EX calls and free-threaded
execution retain the existing provider. Generator callees can use overlap:
`RETURN_GENERATOR` moves every slot through `_PyFrame_Copy` and NULLs the
source before restoring the caller. Retained frames use the corresponding
copy-on-retirement protocol. Child entry still uses `start_frame` recursion
and RESUME handling. The source provides no reason to merge these paths into
a more elaborate general binder or add another frame representation.

The publication change is focused: retirement validates retired values before
discarding input metadata, consumes inputs, flushes the result, then retires.
Its analyzer markers retain synchronization properties. The generated diff
removes precisely five redundant pre-retirement stores, preserving final
local stores and existing callback refresh/error handling.

The next frame-entry increment should use the shared overlap primitive.
Compaction tracks held owners through consumption (`regcfg.c:899–914`),
including dead-but-owned values; its future-read verifier alone is insufficient
to justify skipping initialization. All physical homes must remain valid for
copying, exposed frames, generators, cleanup and roots. Reuse only the proven
NULL non-argument overlap prefix, initialize any new tail and fill constants
normally. Ordinary thread retirement already clears locals only after asserting
empty temporaries (`frame.c:185–190`); there is no release-mode blanket
temporary clear to remove there.

## Admission and next action

The coordinator reports release/debug/stats builds with 117 modules each,
939 focused tests per release/debug profile and zero ownership-lint failures.
This reviewer inspected source and the integration receipt, not raw build
logs or independent execution; those results remain coordinator evidence.
Broad suites and matched throughput results were pending during this review.

Correct the small assertion invariant, then integrate and measure the reviewed
overlap-initialization packet separately. Reserve the remaining publication
experiment for evidence that GCC retains meaningful work. Keep the full-suite
run at the coherent batch boundary. No zero-box admission gate, second compiler,
ctypes/LRU change, or general ownership-bitmap design is needed for this work.
