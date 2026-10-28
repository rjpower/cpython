## Summary

The in-place slot conversion fits the architecture, and wrapper/getset bridges have named retirement steps.  
The revision addresses the first review’s deletion, error-context and compatibility concerns.  
Rooting validation and several dispatch conversions remain incomplete.

## Blocking issues

1. **The proposed precise-root fixture will still be scanned conservatively.**

   **Where:** Tests 5. In [Python/gc_walk.c](/home/power/code/cpython/.worktrees/boil-6d/Python/gc_walk.c:784), section-based precision explicitly excludes extension frames. `_testinternalcapi` is a shared extension in both configured builds, so adding `Py_GC_PRECISE` there does not establish the claimed “span is the only root” condition. Conservative roots can still conceal the bug. Moreover, the canonical-root negative control should fail the C checker when it retains the box across collection.

   **Fix:** place the checked positive fixture in the core, or explicitly register its range as precise. Use the existing test-range registration mechanism for the deliberately invalid negative fixture, as [missed_root does](/home/power/code/cpython/.worktrees/boil-6d/Modules/_testinternalcapi/gc_walk.c:391). Assert precise classification, no walk fallback, positive survival and negative failure.

2. **The shared injection helper does not test the receiver box consumed by a direct method call.**

   **Where:** §§B/F and Tests 5. [get_method_value](/home/power/code/cpython/.worktrees/boil-6d/Objects/object.c:1482) returns an unbound descriptor for METHOD; it does not invoke `tp_descr_get`. `_PyObject_GetMethodValue` leaves the original inline receiver in `*self`. The actual C-method receiver is boxed later by [method_check_args](/home/power/code/cpython/.worktrees/boil-6d/Objects/descrobject.c:281).

   An injection during lookup therefore either disappears when the unnecessary lookup box is removed, or protects a different box from the one the method dereferences.

   **Fix:** resolve METHOD on the value without boxing. Inject collection after the call layer anchors its actual receiver box. Keep descriptor-binding injection separately. Specify that `box_owner_for_descr` appends to a caller-owned span whose lifetime covers the call; it must not return with a root pointing into its own expired frame. Assert both site execution and box counts.

3. **The promised dispatch audit still omits calls that can hide malformed results.**

   **Where:** §§A/F. The committed inventory contains initialisers, not the “locals” and “calls” lists §F relies on. Its placement list omits descriptor calls in `type_get_doc`, `type_get_annotate`, `type_get_annotations`, `_PyObject_LookupSpecial`, `_PyObject_LookupSpecialMethod`, `lookup_method_ex` and `subtype_dict`.

   For example, [type_get_doc](/home/power/code/cpython/.worktrees/boil-6d/Objects/typeobject.c:1879) calls a descriptor inside an object-form getter. `getset_get` then canonicalises that getter’s result, hiding a noncanonical box before the outer check can inspect it.

   **Fix:** commit an inventory of actual dispatches and direct calls to converted implementations. Specify each result conversion and place the check immediately after dispatch. Include the direct `classmethod_get` call in the already-precise `classmethoddescr_call`. Name retirement steps for necessary object bridges, and add malformed-result subprocess cases through nested getsets and explicit wrappers.

4. **The super conversion covers only specialised outputs and leaves inline inputs undefined.**

   **Where:** §C and Tests 8. The generic [_LOAD_SUPER_ATTR](/home/power/code/cpython/.worktrees/boil-6d/Python/bytecodes.c:2435), also used by the instrumented form, still calls `PyObject_GetAttr` and canonicalises its boxed result. Changing the two specialised handlers leaves that boundary intact; warmed measurements can miss it.

   The handlers also use `PyValue_ObjectBorrow(self_st)` without establishing an object receiver. Valid expressions such as `super(float, 2.5).__repr__()` can receive this opcode: [codegen accepts arbitrary two-argument super calls](/home/power/code/cpython/.worktrees/boil-6d/Python/codegen.c:3944). A result-only rewrite leaves this assertion/crash path unresolved.

   **Fix:** cover generic, instrumented and specialised paths. Carry lookup results as values, and define where an inline `su_obj` is boxed and rooted for object-form consumers. Test cold and warmed execution, monitoring, inline receivers and boxing failure.

5. **Forwarding slots can retain the very boxes this rung claims to remove.**

   **Where:** §§C/D and Measurements. “Return `PyValue_FromResult` of what they build” is insufficient for forwarding reads. Existing examples include:

   - [proxy_getattr](/home/power/code/cpython/.worktrees/boil-6d/Objects/weakrefobject.c:709), generated around `PyObject_GetAttr`;
   - [ga_getattro](/home/power/code/cpython/.worktrees/boil-6d/Objects/genericaliasobject.c:725), forwarding to the origin;
   - [local_getattro](/home/power/code/cpython/.worktrees/boil-6d/Modules/_threadmodule.c:1847), reading through `PyDict_GetItemRef`;
   - [_Py_module_getattro_impl](/home/power/code/cpython/.worktrees/boil-6d/Objects/moduleobject.c:1321), calling module `__getattr__` through `PyObject_CallOneArg`.

   Wrapping these object results restores canonical form while preserving transient boxes and allocations. Identity tests will pass.

   **Fix:** specify value-to-value forwarding through attribute, dictionary and vectorcall APIs. Add enforcing box/allocation cases for weak proxies, generic aliases, thread locals and module `__getattr__`, including optional lookup and suppressed errors.

## Non-blocking issues

- **Error-context boxing is duplicated.** §B boxes the owner before `_PyObject_SetAttributeErrorContext`, but that helper calls `PyObject_SetAttr`, which canonicalises it before the native member setter boxes it again. Make the context helper value-aware and let the native field store perform the required box.
- **Retire the redundant type helper.** Once `_Py_type_getattro_impl` returns `PyValue`, its existing object-wrapper role around `_Py_type_getattro_value` disappears. Consolidate them and update callers.
- **Strengthen numeric tests.** Require `assertIs(result, original_nan)` for identity-preserving paths; inequality with another NaN cannot detect replacement. Include below-`INT32_MIN`, negative infinity, and collecting or raising `__index__`/`__float__` callbacks. Preserve truncation stores and warning-as-error behaviour.
- **Preserve the C API deletion guard.** Moving dispatch must retain [object_setattr’s rejection of NULL with an active exception](/home/power/code/cpython/.worktrees/boil-6d/Objects/object.c:1214), including exception chaining and absence of mutation.

## Questions

- Which integration SHA supplies the reviewed call layer and injection-site numbering?
- Do `_ctypes`’ object-form `getfunc`/`setfunc` callbacks remain? If so, where are their temporary boxes rooted, and which step retires that boundary?
- How will unsigned member conversions work? The available value accessors lack unsigned counterparts; specify the inline and object branches while preserving negative-value warnings and overflow rules.

## Verdict

revise
---
Reviewer: codex, 2026-10-28 01:52 PDT
