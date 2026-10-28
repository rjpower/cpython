## Summary

The in-place slot conversion fits the architecture, and temporary wrapper/getset boxes have named retirement steps.  
Class deletion, inline-owner errors, and several internal callers need explicit treatment.  
The proposed validation does not yet prove the rooting and boxing claims.

## Blocking issues

1. **The proposed class-dictionary helper does not support deletion.**  
   §C specifies `_PyDict_SetItemValue`, but its implementation [asserts that the value is non-NULL]( /home/power/code/cpython/.worktrees/boil-6d/Objects/dictobject.c:1811). Passing the setter’s deletion sentinel would assert in debug builds and violate dictionary invariants in release builds. Merely changing the write also leaves `type_update_dict`’s old-value lookup boxing unnecessarily.

   **Fix:** retain the existing `type_update_dict` transaction and `_PyDict_SetItem_LockHeld`, which already accepts `PyValue` and implements deletion. Remove the unused old-value lookup, preserve type-version invalidation and slot updates, and test replacement/deletion after warming attribute caches, including subclass lookups.

2. **The inline-owner executor does not define its error path.**  
   §B says ABSENT does not read the owner and boxing occurs at descriptor calls. However, [set_no_attribute_error]( /home/power/code/cpython/.worktrees/boil-6d/Objects/object.c:1440) uses the owner’s type and populates `AttributeError.obj` through an object-form helper. Removing the receiver box without replacing this path either loses exception context or treats an immediate as a pointer. The setter path is also unspecified: note 12 §3 keeps its executor’s owner object-form, whereas §B groups it with value-owner dispatch.

   **Fix:** specify value-aware error handling, boxing and rooting an inline owner when storing it in native exception fields. Define the setter’s NO_STORAGE and DATA_DESCR cases explicitly. Test missing reads, writes and deletions on inline ints/floats, exception `.name`/`.obj`, suppressed misses, and boxing failure.

3. **The conversion boundary stops short of surviving object-form callers.**  
   §C’s statement that `super_getattro` returns its lookup does not resolve [do_super_lookup]( /home/power/code/cpython/.worktrees/boil-6d/Objects/typeobject.c:12353), `_PySuper_LookupDescr`, and `_PySuper_Lookup`, which currently return objects. `LOAD_SUPER_ATTR_ATTR` [consumes that object result]( /home/power/code/cpython/.worktrees/boil-6d/Python/bytecodes.c:2495). Boxing the new descriptor result to satisfy these interfaces preserves the boundary this rung intends to remove. Similarly, `_PyObject_GetMethodValue` still calls object-form `PyObject_GetAttr` on its custom-slot fallback.

   **Fix:** list these helpers and callers explicitly. Carry values through core lookup and handler paths; retain object wrappers only where an object interface is required. Add box-count cases for `super().x`, explicit `super` lookup, and custom-getattro method lookup. Any deferred boundary needs an owner and retirement step.

4. **The debug-check placement is incomplete.**  
   §F promises checks at every dispatch, but its placement list omits `classobject.c`, explicit wrappers, and the profiling/tracing calls listed in §A. For example, [method_getattro]( /home/power/code/cpython/.worktrees/boil-6d/Objects/classobject.c:166) invokes a local `descrgetfunc`. A later outer check can blame the wrong slot; wrapper boxing followed by canonicalisation can conceal the mismatch entirely.

   **Fix:** inventory actual invocations, including calls through local function pointers, and check each value-returning slot result before conversion. Add malformed-slot subprocess tests through explicit `__get__` and getattribute wrappers, alongside ordinary dispatch.

5. **The receiver-lifetime tests bypass the proposed injection site.**  
   §F places `_PyGC_SITE_SLOT_BOX` only in `generic_getattr_value`, but Tests 5 uses `(100000).bit_length()` and `(2.5).hex()`. Those expressions take the [method-call lookup path]( /home/power/code/cpython/.worktrees/boil-6d/Python/bytecodes.c:2583), through `_Py_LoadAttr_Values` and `_PyObject_GetMethodValue`, rather than the proposed executor site.

   **Fix:** use `getattr(100000, "bit_length")()` and `getattr(2.5, "hex")()` to exercise that site, asserting it fired. Test direct method calls separately. Specify that the raw-versus-canonical negative fixture executes in a precise core frame: an opaque extension frame’s conservative roots can otherwise keep the box alive and mask the missing raw root.

6. **The box audit cannot run the stated proof as written.**  
   Tests 7 names `box_audit.py` over `slot_boxes.py`’s cases, but these scripts have independent case tables. The audit also [hardcodes the integration worktree]( /home/power/code/cpython/.worktrees/boil-6d/boil/tools/measure/box_audit.py:16), so its default invocation would test another interpreter.

   **Fix:** include the slot cases in the enforcing audit, accept an explicit interpreter path, and record the tested SHA. Define rung-2 residue explicitly for wrapper results and getsets. Add `getattr` with a default and `hasattr` cases; neither appears in the current slot measurement table.

## Non-blocking issues

- **Member implementation location and duplication:** §D places `PyMember_*Value` beside object forms in `descrobject.c`; the existing implementations are in [Python/structmember.c]( /home/power/code/cpython/.worktrees/boil-6d/Python/structmember.c:35). Make that implementation value-based and reduce object forms to compatibility wrappers. Preserve truncation warnings, unsigned negative handling, conversion callbacks, offsets and readonly checks.
- **Inventory precision:** the inventory counts names, not distinct implementations: `Xxo_getattro` occurs in four files and `func_descr_get` in two. Print file/line locations, especially for the 24 unchecked spec entries; the referenced “2.1 list” does not enumerate them.
- **Measurement expectation:** `object_setattr` should lose its numeric box in rung 2 once `wrap_setattr` reads tuple values. Its result is `None`, so retaining object-form `wrapperfunc` does not require that box.
- **Builtin conversion:** changing `hasattr`’s call alone leaves Clinic’s `obj: object` converter boxing inline receivers. Change the converter to `value` and regenerate the wrapper.
- **Failure coverage:** explicitly test the optional API’s three outcomes and cleared output on error, plus allocation failure at compatibility-result boxes and native-field stores. Identity tests do not prove these contracts.
- **Documentation:** §G must also update `object.rst`’s existing generic getter/setter signatures and distinguish `PyValue_NULL` deletion from storing `None`.

## Questions

- Which exact integration SHA supplies 10a’s call layer, and where will the promised per-function C-mode results be recorded?
- Does `PyMember_SetOneValue` preserve the current rejection of deletion for `_Py_T_VALUE`, or intentionally permit it? The existing implementation permits deletion for `_Py_T_VALUE_EX` but rejects `_Py_T_VALUE`; state and test the decision.

## Verdict

revise
---
Reviewer: codex, 2026-10-28 01:23 PDT
