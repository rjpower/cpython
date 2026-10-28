## Summary

The guard consolidation, class-version mirror, and polymorphic stores fit the architecture.  
Two boundary cases remain blocking: large-object sizing and the small-dictionary assertion.  
Static review against source at `ecd7485b9e2`; no tests run.

## Blocking issues

1. **The proposed size-class helper returns a sentinel for large objects.**

   **Where:** [§D](/home/power/code/cpython/.worktrees/boil-6d/boil/design/work/6d-1.md:140), [mi_bin()](/home/power/code/cpython/.worktrees/boil-6d/Objects/mimalloc/page-queue.c:60), and [the huge queue initializer](/home/power/code/cpython/.worktrees/boil-6d/Objects/mimalloc/init.c:45).

   **Problem:** Above `MI_MEDIUM_OBJ_SIZE_MAX`—65,536 bytes on this target—`mi_bin()` returns `MI_BIN_HUGE`. Its queue’s `block_size` is 65,544, not the allocation size. For `B=65544, S=0`, the proposed helper therefore turns a 65,592-byte request into **capacity zero**. A class with 8,192 declared slots plus `__dict__` can reach this case. It violates `_PyShape_InitTypeShapes()`’s positive-capacity assertion and dictionary mode’s requirement for slot 0.

   **Fix:** Implement the unpadded equivalent of the complete `mi_good_size()` calculation, including its large-request branch and overflow limits. Cap `w` at 30 before constructing the request. Update `capacity_rule.py` accordingly, and test requests around 65,536 plus a large-slotted class that supports an instance dictionary.

2. **The no-resize assertion fails on ordinary small conversions.**

   **Where:** [§G](/home/power/code/cpython/.worktrees/boil-6d/boil/design/work/6d-1.md:291), [dict_new_presized()](/home/power/code/cpython/.worktrees/boil-6d/Objects/dictobject.c:1391), and `dict_from_slots()` in [Objects/shapeobject.c](/home/power/code/cpython/.worktrees/boil-6d/Objects/shapeobject.c).

   **Problem:** `dict_new_presized(minused, true)` returns `PyDict_New()` when `minused <= 5`. For conversions with one through four slots, the first insertion replaces `Py_EMPTY_KEYS`. Consequently, the proposed assertion that the keys pointer remains unchanged fails, although no `dictresize()` occurred. The seven-attribute regression misses this branch.

   **Fix:** Distinguish initial table allocation from subsequent growth. Keep pointer equality for eagerly presized tables; for initially empty dictionaries, verify that filling uses only the minimum table and leaves room for the triggering store. Cover zero, one, four, five, and seven slots, including holes and an actual conversion-triggering store.

## Non-blocking issues

- **Sampling does not count conversion events.** §D says dictionary-mode samples reveal types pushed into dictionary mode, but [occupancy_types.py](/home/power/code/cpython/.worktrees/boil-6d/boil/tools/measure/occupancy_types.py) can miss every short-lived converted instance. Note 12 §6.4 explicitly requires kernel conversion counts because sampling misses transient peaks. Record start/end dictionary-mode counter deltas per kernel; retain sampled rows for type attribution.

- **The committed baseline omits allocation counts.** Note 12 §10 requires `Object allocations`, but [measure_boxes()](/home/power/code/cpython/.worktrees/boil-6d/boil/bench/run.py:373) does not serialize that counter, and `6d-before-pystats.json` contains no allocation measurements. This matters for getters such as `complex.real`, whose temporary numeric allocation already reports zero inline boxes. Add allocation counts and capture the before values before converting those getters.

- **Update the governing specification.** [Note 12 §6.3–6.4](/home/power/code/cpython/.worktrees/boil-6d/boil/design/12-object-slots-on-values.md:381) still specifies `_PyDict_NewPresized()` and padded `mi_good_size()`, contradicting this revision’s Unicode helper and unpadded rule. `boil/PROCESS.md` makes the step-level note the specification; reconcile both documents.

## Questions

- Which comparison commits and after-result files will establish rung 1’s instruction attribution? §D names the before records, while “Risks” requires merging 10a at every commit boundary. Specify how those concurrent changes will be separated in the measurements.

## Verdict

revise
---
Reviewer: codex, 2026-10-28 00:48 PDT
