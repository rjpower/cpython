## Summary

The in-place slot ABI and protocol wrappers fit the architecture.  
Descriptor binding, native storage, and ABI failure guarantees need correction.  
The implementation plan also needs buildable rung boundaries and stronger GC proofs.

## Blocking issues

1. **`slot_tp_descr_get` cannot use the proposed binding helper.**  
   **Where:** §4; [slot_tp_descr_get](/home/power/code/cpython/.worktrees/boil-6d/Objects/typeobject.c:10853) and `vectorcall_method_value` in the same file.  
   **Problem:** The current slot retrieves the raw `__get__` class attribute and calls it with three arguments. `vectorcall_method_value` binds that attribute first. For `__get__ = staticmethod(lambda *args: args)`, ordinary descriptor access currently supplies `(descriptor, instance, owner)`; the proposed helper supplies only `(instance, owner)`. I verified this distinction with a host-Python probe.  
   **Fix:** Keep the raw `_PyType_LookupValueAndVersion` lookup and call its result through `PyObject_VectorcallValue` with three values. Preserve the missing-method fallback. Add tests for staticmethod and custom callable implementations of `__get__`.

2. **Native object-member stores have no specified implementation.**  
   **Where:** §§4, 8–9; [PyMember_SetOne](/home/power/code/cpython/.worktrees/boil-6d/Python/structmember.c:155), [property_members](/home/power/code/cpython/.worktrees/boil-6d/Objects/descrobject.c:1608), and [Pickler_setattr](/home/power/code/cpython/.worktrees/boil-6d/Modules/_pickle.c:5697).  
   **Problem:** `_Py_T_OBJECT` and `Py_T_OBJECT_EX` fields still physically hold `PyObject *`. Stores such as `p.__doc__ = 1.5` therefore require a box unless the field and its consumers change representation. Custom setters such as `Pickler.persistent_id` have the same issue. These boundaries are absent from §8’s purported complete inventory.  
   **Fix:** Specify boxing at native pointer storage, retain the raw box until publication, and record that boundary explicitly. A canonical value root alone cannot retain a numeric box. Also specify member semantics: NULL `_Py_T_VALUE` reads as `None`, NULL `_EX` raises, booleans remain booleans, and unsigned members use unsigned constructors. Cover these through both APIs.

3. **The closed-set argument for descriptor receivers excludes explicit descriptor calls.**  
   **Where:** §§2.4, 3, 8; [wrap_descr_get](/home/power/code/cpython/.worktrees/boil-6d/Objects/typeobject.c:10011).  
   **Problem:** `property(lambda x: x).__get__(3, int)` passes an inline instance to a property descriptor without finding that descriptor on `int`, `float`, or `object`. The retained `PyObject *obj` signature requires boxing here. This is outside the “int and float C methods and getsets” residue, and §12.1’s proposed receiver flag would not remove it.  
   **Fix:** Restrict the closed-set proof to implicit attribute lookup. Specify explicit `__get__`, `__set__`, and `__delete__` argument conversion and rooting, then give their remaining boxes an owner and tests.

4. **The claimed ABI failure behavior is false.**  
   **Where:** §2.1 and §9.  
   **Problem:** An old setter can ignore its argument or save it into a pointer field without dereferencing it. It need not fault immediately; corruption can surface later. Result checks cannot detect this. Likewise, removing casts does not make the compiler check `PyType_Slot` entries, which remain `void *`. Changing the SDK’s `abi_id` does not reject an old binary at import.  
   **Fix:** Remove the “loud rather than corrupt in every case” guarantee. Require an explicit audit of every untyped in-tree slot entry. Either reject value-v1 binaries through an enforced ABI discriminator or record the unsafe import gap and its retirement step. Test a legacy setter that merely saves its argument, alongside the boxed-result diagnostic.

5. **Rung 2’s `wrapperfunc` change cannot land with its listed scope.**  
   **Where:** §§7, 11; [wrapperdescr_raw_call and wrapperdescr_call](/home/power/code/cpython/.worktrees/boil-6d/Objects/descrobject.c:541).  
   **Problem:** Rung 2 changes `wrapperfunc` to return `PyValue`, but rung 3 owns these consumers. Their current bodies return the wrapper result as `PyObject *`. The intermediate tree will not compile. Converting the entire consumer chain immediately also reaches the still-object-form `tp_call` ABI.  
   **Fix:** Define an atomic dependency boundary. Convert the raw wrapper dispatcher in rung 2 and put an explicit temporary boxing adapter at the existing call entry, retired in rung 3; alternatively move the connected call-slot conversion into rung 2. State which files each rung owns and how its intermediate ABI passes level 3.

6. **The validation does not prove the changed lifetime contracts.**  
   **Where:** §§7, 9; [value-root helpers](/home/power/code/cpython/.worktrees/boil-6d/Include/internal/pycore_value.h:337), `Objects/call.c`, and `Python/bytecodes.c`.  
   **Problem:** Numeric identity tests prove canonical results, not retention of temporary boxes, fresh heap results, or argument arrays. The C checker covers marked functions; the rewritten opaque descriptor and executor paths still need runtime evidence. Instrumented `CALL_FUNCTION_EX` also holds a result across monitoring callbacks, which may collect.  
   **Fix:** Add targeted collection-stress and applicable kill-mode tests for:
   
   - Inline receiver boxes read by a C getter after collection.
   - Fresh heap results surviving monitoring callbacks that collect.
   - Descriptor callbacks that remove themselves or replace their getter before collecting.
   - Prepend calls exceeding the small-stack buffer, with keyword unpacking.
   - Allocation failure during boxing and conversion, including root cleanup.

   Check raw object roots separately from canonical values: `_PyValueSpan_AppendObject` retains a box; appending `PyValue_FromObject(box)` may not.

## Non-blocking issues

- **Specify all `class_version` writers.** §6.5 should explicitly include `_PyShape_InitRoot`, `_PyShape_InitTypeShapes`, and `new_shape` in [shapeobject.c](/home/power/code/cpython/.worktrees/boil-6d/Objects/shapeobject.c:36). State the invariant `shape->class_version == shape->type->tp_version_tag`, including zero, independently of the shape version. Add invalidation tests after descriptor replacement, base changes, dictionary conversion, and version exhaustion; zero deoptimizations across generalisation proves only the performance case.

- **Complete the conversion inventory.** §2.3 misses direct descriptor calls in [method_getattro and instancemethod_getattro](/home/power/code/cpython/.worktrees/boil-6d/Objects/classobject.c:151). §5’s trampoline work also reaches getter/setter trampolines and [wasm_trampoline.c](/home/power/code/cpython/.worktrees/boil-6d/Python/wasm_trampoline.c:1). List these explicitly so return-type conversions do not leave hidden object round trips.

- **Make the API contracts exact.** §2.3 needs complete signatures for the new public entries, especially the callable argument type and optional-attribute out-parameter. Specify found/missing/error behavior and boxing failure. Public compatibility functions must retain external linkage; “static three-line wrapper” is inaccurate.

- **Make the measurements reproducible.** Commit the scratch slot and capacity probes used in §§6.4 and 10. Report capacity conversions by kernel and static-attribute count. Also correct §10’s “24% store cost”: note 06 §20.9 attributes about 10% of go’s instructions to the generic store path; 24% is the broader regression.

## Questions

- Which integration commit establishes the required 10a state, and who owns marking the rewritten typeobject bridges? §12.3 still offers competing schedules.
- If rung 4 is cut, which named step retires object-form getsets, and will that later conversion require another ABI revision?
- Is §12.5’s capacity threshold an automatic fallback decision? Define the comparison population before implementation, consistently with PROCESS’s rule that measurements are recorded rather than tuned for.

## Verdict

revise
---
Reviewer: codex, 2026-10-27 23:46 PDT
