## Summary

The in-place slot conversion fits the architecture and gives the main compatibility bridges retirement steps.  
Dispatch coverage, remaining forwarding paths, and the root-test proof still need correction.  
Reviewed revision `ea58963eed4`; concurrent implementation edits were excluded.

## Blocking issues

1. **The dispatch inventory skips a real call, and §F omits its check.**

   **Where:** §§A/B/F. [slot_inventory.py:62](/home/power/code/cpython/.worktrees/boil-6d/boil/tools/measure/slot_inventory.py:62) treats every line beginning with `*` as a comment. This excludes [the optional-attribute dispatch](/home/power/code/cpython/.worktrees/boil-6d/Objects/object.c:1137):
   ```c
   *result = (*tp->tp_getattro)(v, name);
   ```
   §B carries that dispatch into `PyObject_GetOptionalAttrValue`, but §F lists only `getattr_object_value`. A malformed slot can therefore escape the promised immediate check through optional lookup, including `getattr` with a default and `hasattr`.

   **Fix:** distinguish comment continuations from dereference assignments, regenerate the inventory, and explicitly check this dispatch before conversion or suppression. Extend the malformed-getattro subprocess cases to both optional APIs and both builtins.

2. **§C’s blanket conversion rule preserves boxes in remaining forwarding slots.**

   **Where:** §C’s final bullet and Tests 9/11. [Example_getattro](/home/power/code/cpython/.worktrees/boil-6d/Modules/_testmultiphase.c:79) and the `Xxo_getattro` implementations read through `PyDict_GetItemWithError`; they do not merely return objects they build. Wrapping those results in `PyValue_FromResult` preserves the temporary numeric boxes.

   Similarly, boxing at the top of [Pickler_setattr](/home/power/code/cpython/.worktrees/boil-6d/Modules/_pickle.c:5697) also boxes ordinary attributes forwarded to the generic setter. The native field requires boxing only in the `persistent_id` branch.

   **Fix:** specify value dictionary reads and writes for these implementations. Box inside branches that actually retain a `PyObject *`. Explicitly convert [proxy_setattr](/home/power/code/cpython/.worktrees/boil-6d/Objects/weakrefobject.c:734) to value forwarding too. Add enforcing cases for these dictionary and generic-fallback paths; the current forwarding test list misses them.

3. **The lifetime fixture’s stated fallback assertion does not observe test-range fallbacks.**

   **Where:** Test 5. [gc_walk.c’s fallback handler](/home/power/code/cpython/.worktrees/boil-6d/Python/gc_walk.c:845) increments `w->test_fallbacks` for `_PyGC_RANGE_TEST`, rather than the normal statistics counter. [scan_from](/home/power/code/cpython/.worktrees/boil-6d/Python/gc_trace.c:1179) discards both the walk’s return status and this separate counter.

   Thus “zero fallbacks from the walk’s statistics” does not establish that the registered fixture escaped conservative scanning. Separate positive and negative functions can have different unwind behaviour.

   **Fix:** expose and assert test-range fallbacks, or otherwise observe the actual collection walk through each fixture. Require precise classification and no fallback for both controls, alongside raw-box survival and canonical-control death.

## Non-blocking issues

- **Two proposed object bridges already have value outputs.** §F groups `_PyObject_LookupSpecialMethod` and `lookup_method_ex` with `_PyObject_LookupSpecial`. The first two already write `PyValue` outputs in [typeobject.c](/home/power/code/cpython/.worktrees/boil-6d/Objects/typeobject.c:2744). Assign checked descriptor results directly into those outputs. Keep boxing at consumers that actually require objects.

- **The fatal-error classification is inaccurate.** §F calls the fatal reporter non-collecting. [fatal_error](/home/power/code/cpython/.worktrees/boil-6d/Python/pylifecycle.c:3588) can display exceptions and flush Python streams, invoking Python and collection. Use a reporter that cannot invoke Python, or a checked opaque reporting path; a reviewed comment does not establish this effect.

- **Specify allocation assertions.** Tests 9/11 promise enforced allocation counts, but [box_audit.py](/home/power/code/cpython/.worktrees/boil-6d/boil/tools/measure/box_audit.py:198) currently gates only boxing counters. Define expected allocations per case, including intentional bound-method allocations and rung-4 residue.

- **Isolate audit statistics.** Passing an interpreter path does not fix `box_audit.py`’s shared `/tmp/py_stats` directory and deletion of newly discovered files. Use a task-owned `PYTHON_STATS_DIR`, as `slot_boxes.py` already does.

## Questions

- At the named base, `method_vectorcall_*` and `classmethoddescr_call` are still unmarked, despite note 12 §7’s precision claims. Who owns their promotion, and will `method_check_args` append its receiver box to a caller-owned span or rely on conservative scanning until that promotion?
- Which step retires the inline `super` receiver box? §C assigns it to open question 1, whose proposed method-receiver conversion does not change `superobject.obj`’s `PyObject *` storage.

## Verdict

revise
---
Reviewer: codex, 2026-10-28 02:15 PDT
