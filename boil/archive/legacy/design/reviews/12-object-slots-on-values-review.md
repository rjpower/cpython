## Summary

The in-place value slots and protocol compatibility layer fit the architecture.  
The revision resolves the earlier descriptor-binding and native-field issues.  
Four gaps remain: wrapper arguments, generic dict accessors, ABI sequencing, and GC validation.

## Blocking issues

1. **Explicit wrappers have more unrestricted pointer arguments than §2.4 accounts for.**

   **Where:** §§2.4, 3; [wrap_descr_get and adjacent wrappers](/home/power/code/cpython/.worktrees/boil-6d/Objects/typeobject.c:10011).

   **Problem:** `wrap_descr_get` must handle an inline second argument too: `d.__get__(None, 3)` currently passes a boxed `3` as `type`. A Python descriptor can observe that value; it need not be a type. Likewise, explicit attribute wrappers receive arbitrary names, including numbers. Replacing `PyArg_UnpackTuple` with tuple-value reads cannot assume these positions contain object pointers.

   **Fix:** Specify conversion and raw-object rooting for every unrestricted argument retained as `PyObject *`. Validate before borrowing where the existing contract requires rejection. Add tests for numeric `__get__` owner arguments and numeric attribute names, including allocation failure when boxing is required.

2. **The “generic functions are slots” decision omits the generic dict accessors.**

   **Where:** §§2.2–2.3, rung 4; [function_getset](/home/power/code/cpython/.worktrees/boil-6d/Objects/funcobject.c:1033), [PyObject_GenericGetDict](/home/power/code/cpython/.worktrees/boil-6d/Objects/dictobject.c:6046), [PyObject_GenericSetDict](/home/power/code/cpython/.worktrees/boil-6d/Objects/object.c:1877).

   **Problem:** §2.3 says these paths are unchanged, but both public functions are installed directly in `PyGetSetDef` tables. Their existing signatures become incompatible when `getter` and `setter` change. Keeping the initializers unchanged cannot compile; casts would conceal the mismatch.

   **Fix:** Apply §2.2’s decision explicitly to these functions in rung 4: change their signatures, declarations, documentation, and direct callers together. Preserve dict identity, conversion failure behavior, and rejection of invalid assignments and deletion. Remove the conflicting “unchanged” statement.

3. **ABI isolation starts after the first ABI break and does not cover later rung boundaries.**

   **Where:** §§2.1, 6.5, 11; [_PyShape layout](/home/power/code/cpython/.worktrees/boil-6d/Include/object.h:162), [embedded root shape](/home/power/code/cpython/.worktrees/boil-6d/Include/cpython/object.h:245).

   **Problem:** Rung 1 adds `class_version` to `_PyShape`, enlarging the structure embedded in `PyTypeObject` and shifting heap-type fields. The suffix changes only in rung 2. Later, rungs 3 and 4 change function ABIs again under that same suffix. Each rung is independently landed, so binaries from incompatible landed states remain discoverable.

   **Fix:** Change the suffix and `abi_id` with the first layout change, and distinguish subsequent independently landed ABI changes. Alternatively, make the ABI transition atomic. Test rejection against each supported predecessor state.

4. **The proposed inline-receiver tests do not force the claimed lifetime transition.**

   **Where:** §9, “Lifetimes”; [float_float](/home/power/code/cpython/.worktrees/boil-6d/Objects/floatobject.c:1115), [int_bit_length_impl](/home/power/code/cpython/.worktrees/boil-6d/Objects/longobject.c:6192), [allocation trigger](/home/power/code/cpython/.worktrees/boil-6d/Objects/obmalloc.c:1599).

   **Problem:** `float.real` returns its exact-float receiver without allocating. `(7).bit_length()` uses cached small integers. Stress mode therefore does not ensure collection between creating a receiver box and reading it. Conservative scanning of the opaque executor can also preserve a box despite an incorrect canonical root.

   **Fix:** Add named collection injection after boxing and before dereference, using an uncached int or float. Include a precise C fixture whose raw span is the box’s only root, with a negative control demonstrating that canonicalizing that root loses the box. Assert that the injection actually ran.

## Non-blocking issues

- **Name the compatibility callers that must migrate.** §§2.3 and 11 introduce `PyObject_GetOptionalAttrValue` without identifying its consumers. [builtin_getattr](/home/power/code/cpython/.worktrees/boil-6d/Python/bltinmodule.c:1312) still boxes through the optional-attribute API when given a default. Add `getattr(o, name, default)` and `hasattr` to the conversion inventory and box audit.

- **Bound the suffix guarantee.** §2.1 excludes conventionally tagged value-v1 binaries. Untagged `.so` files and explicit loaders can bypass filename selection. State that limitation rather than describing the suffix as universal binary rejection.

- **Measure capacity conversions per kernel.** §§6.4 and 10 rely partly on sampled occupancy. [occupancy_types.py](/home/power/code/cpython/.worktrees/boil-6d/boil/tools/measure/occupancy_types.py:35) misses transient occupancy peaks and excludes dictionary-mode objects from its histogram. Record actual capacity-triggered conversions by kernel and type alongside RSS and sampled predictions.

- **Separate wasm trampoline changes by rung.** §5 groups getset trampoline changes with call-slot work, although getsets change only in rung 4 and may be cut. Assign each signature change to its owning rung and identify who validates the unbuilt wasm path.

## Questions

- Which integration commit and owner settle §7’s remaining ordering choice with 10a?
- Is the capacity alternative in §12.5 an acceptance rule or a later coordinator decision, and what comparison population determines “double”?
- If getsets move to 6e, when is that step added to the architecture plan and assigned an owner?

## Verdict

revise
---
Reviewer: codex, 2026-10-28 00:11 PDT
