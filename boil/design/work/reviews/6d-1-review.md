## Summary

The handler consolidation and separate class and shape versions fit the architecture.  
Three issues block implementation: ABI ordering, allocator padding, and dictionary key layout.  
Reviewed the design, required context, and relevant source paths; no tests run.

## Blocking issues

1. **The ABI mark lands after the ABI break.**

   **Where:** §E, §F, and “Order of commits”: E is first, F is last, and every commit is pushed.

   **Problem:** Adding `class_version` changes `PyTypeObject` and subsequent `PyHeapTypeObject` offsets while intermediate commits still accept conventionally named value-v1 extensions. Note 12 §2.1 requires isolation from the first layout change.

   **Fix:** Commit E and F together, including suffix tests and SDK metadata. Include E’s regeneration and magic bump there: E changes the member macros and specialiser output, which also falls under `boil/PROCESS.md` §3’s rule.

2. **The capacity formula counts allocator padding as available object storage.**

   **Where:** [§D]( /home/power/code/cpython/.worktrees/boil-6d/boil/design/work/6d-1.md:133), [mi_good_size()](/home/power/code/cpython/.worktrees/boil-6d/Objects/mimalloc/page-queue.c:109), and [pycore_mimalloc.h](/home/power/code/cpython/.worktrees/boil-6d/Include/internal/pycore_mimalloc.h:34).

   **Problem:** `mi_good_size(n)` rounds **`n + MI_PADDING_SIZE`**. Debug builds enable eight bytes of mimalloc padding. For `B=8, S=1`, the proposed capacity is therefore five on debug and three on release. Allocating the enlarged payload applies padding again, invalidating the claim that blocks never grow. CPython’s debug allocation hooks add further overhead. The measurement script models unpadded classes, so it also disagrees with this implementation.

   **Fix:** Define the policy using unpadded size classes through one allocator helper, and validate `capacity_rule.py` against it. Alternatively, explicitly adopt a build-dependent rule and revise its sizing proof and measurements. Test the chosen behavior on release, debug, and ASan, including size-class boundaries.

3. **Presizing changes converted dictionaries to the representation rejected by the attribute fast paths.**

   **Where:** §G; [dict_from_slots()](/home/power/code/cpython/.worktrees/boil-6d/Objects/shapeobject.c:509), [_PyDict_NewPresized()](/home/power/code/cpython/.worktrees/boil-6d/Objects/dictobject.c:1390), and [_LOAD_ATTR_WITH_HINT](/home/power/code/cpython/.worktrees/boil-6d/Python/bytecodes.c:2722).

   **Problem:** Above five requested entries, `_PyDict_NewPresized()` allocates a `DICT_KEYS_GENERAL` table. Existing conversion starts with an empty dictionary and produces a Unicode-key table for exact attribute names. Both `WITH_HINT` handlers reject general-key tables. The proposed optimization would therefore disable subsequent load/store specialization for these converted objects and enlarge their entries.

   **Fix:** Expose the existing Unicode-aware presizing mechanism for conversion. Add a regression with at least six exact-string attributes that checks the converted key kind and warms both `WITH_HINT` handlers. Make the no-resize proof mandatory; logging’s aggregate instruction count cannot establish it.

## Non-blocking issues

- **Correct the layout accounting in §E.** [_PyShape](/home/power/code/cpython/.worktrees/boil-6d/Include/object.h:162) is currently **64 bytes** on this target and becomes **72**, rather than 24→32. `PyTypeObject` grows by eight bytes. Update the static-type, heap-type, shape-table, and populated-shape expectations in [test_sys.py](/home/power/code/cpython/.worktrees/boil-6d/Lib/test/test_sys.py:1909), beyond the instance-size cases named in §D.

- **Extend §B’s invalidation tests.** Renewing one row’s version does not cover installing a data descriptor over the stored name, changing `__setattr__`, or collecting a row’s type while the code object survives. Adapt the existing load-side dead-type and mutation tests for stores. Also test target-only generalisation: an ADD target can generalise without renewing its parent’s version. A stale narrow kind remains safe because kinds only widen; state that invariant explicitly.

- **Test collector visibility after polymorphic stores.** Store fresh heap values through ADD and REPLACE rows, drop the other references, collect, and read them back. Verify the replaced value can die. The allocation-free handler needs no new root span, but correctness still depends on publishing the slot before the shape and on traversal seeing the resulting layout.

- **Strengthen §C’s shadowing tests.** “Then with the dict made” should include an empty dict, an unrelated attribute, and an actual method-name shadow. Cover dictionary creation through the C API and negative offsets on differently sized int/tuple subclasses.

- **Complete the measurement plan.** Note 12 §11 assigns rung 1 the new micro rows and their before values; this document omits them. After the capacity change, sampling only shape-mode objects cannot reveal objects already forced into dictionary mode. Preserve the before histograms and record per-kernel conversion counts alongside RSS and instruction attribution.

- **Fix §F’s audit arithmetic.** The list contains four `descr_get` entries and four function names. With that correction, the category totals equal the claimed 38.

## Questions

- If the suite exceeds 53,084 capacity conversions, does landing wait for the coordinator’s fallback decision? Name the decision point and the fallback commit’s required remeasurement.
- Should member sites specialise when the shape version is zero but the class version remains valid? §E describes class-only guards, while both current specialisers reject `plan.type_version == 0` before reaching the member branch. Specify whether those early checks move.

## Verdict

revise
---
Reviewer: codex, 2026-10-28 00:32 PDT
