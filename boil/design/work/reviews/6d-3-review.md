## Summary

The in-place slot conversion fits the architecture.
The plan has a collector lifetime hole, conflicting API signatures, and incomplete conversions.
Revise before implementation; this review is based on source inspection.

## Blocking issues

1. **The monitored call result needs an explicit root.**  
   **Where:** §E and Tests 6; [Python/bytecodes.c](/home/power/code/cpython/.worktrees/boil-6d/Python/bytecodes.c:5191).  
   **Problem:** `_DO_CALL_FUNCTION_EX` keeps the result across the `C_RETURN` callback. That callback can collect before the result reaches its output slot. Interpreter frames are precise, so conservative C-stack scanning does not protect the local; reference counting provides no protection either. Changing the call and NULL tests does not establish a root.  
   **Fix:** Hold the returned `PyValue` in a pushed value root through instrumentation, then publish it to the output. Pop on success and callback failure. Add a deterministic test returning a fresh, otherwise unreachable heap object while `C_RETURN` collects, plus a callback that raises. Run both under kill mode.

2. **The public value-call signature contradicts the specification.**  
   **Where:** §B versus note 12 §2.3.  
   **Problem:** This document declares `PyObject_CallValue(PyObject *callable, ...)`; note 12 requires `PyValue callable`, matching `PyObject_VectorcallValue`. The prepend form likewise specifies only `obj` becoming a value, leaving its callable contract unresolved. This makes the new API require an object conversion before it can reject an inline non-callable.  
   **Fix:** Use `PyValue callable` consistently in the public and internal value forms and prepend helper. Canonicalise at object compatibility entries. Specify the inline rejection path before pointer access, and test inline ints and floats through both APIs.

3. **The conversion inventory omits affected callers and a public declaration.**  
   **Where:** §§B–C and Commit 1.  
   **Problem:** `call_method` has another caller: [`slot_tp_init`](/home/power/code/cpython/.worktrees/boil-6d/Objects/typeobject.c:10985). `_PyObject_Call` also serves [`PyObject_CallObject`](/home/power/code/cpython/.worktrees/boil-6d/Objects/call.c:636). Neither conversion is specified. `PyVectorcall_Call` is declared in [`Include/abstract.h`](/home/power/code/cpython/.worktrees/boil-6d/Include/abstract.h:283), which the change list omits. Following the listed edits leaves unresolved calls or incompatible declarations.  
   **Fix:** Add every affected usage. Let `slot_tp_init` inspect the value result directly, preserving the “must return None” check and naming an inline result’s type without boxing. Keep `PyObject_CallObject` as an object compatibility entry. Update the existing `PyVectorcall_Call` declaration in place.

4. **The module recipe retains boxes where value implementations already exist.**  
   **Where:** §D and Tests 8; [Modules/_operator.c](/home/power/code/cpython/.worktrees/boil-6d/Modules/_operator.c:1113).  
   **Problem:** The description of `itemgetter_call` is stale: it already calls the value-returning `itemgetter_call_impl`, then boxes its result. `attrgetter_call` has the same structure. Wrapping those object results in `PyValue_FromResult` preserves a needless box. Canonicalisation allocates nothing, but the preceding adapter does. Ordinary calls use vectorcall and can conceal this omission.  
   **Fix:** Return the existing value helpers directly. Add direct `tp_call` coverage and box budgets for both getters, using uncached int and float results. Audit the other module adapters by their actual return expressions.

5. **The commit boundaries leave incompatible intermediate interfaces.**  
   **Where:** Commits 1–3 and §§C–D.  
   **Problem:** Commit 1 converts `call_method`, while its object-returning `slot_tp_call` remains until Commit 2. Commit 2 converts `wrapper_call` and `wrapperdescr_call`, while `wrapperdescr_raw_call` and `wrapperfunc` remain object forms until Commit 3. Conversely, `wrap_call` must consume the new `callfunc` result while still satisfying the old wrapper signature. The stated final bodies cannot simply land in that order.  
   **Fix:** Combine the dependent conversions, or specify each temporary boundary conversion and its removal commit. Put focused regression tests in the commits that introduce the behavior; deferring all new tests to Commit 5 weakens the earlier gate claims.

## Non-blocking issues

- **Specify the fpcast conversion beyond changing casts.** In [Modules/_testcapi/fpcast.c](/home/power/code/cpython/.worktrees/boil-6d/Modules/_testcapi/fpcast.c:15), `zero` through `four` are shared by methods, getsets, and call-slot fixtures. The call fixtures need separate value-returning functions while the other consumers retain their current conventions. The wasm value trampoline must test value-returning signatures at every supported arity. A pointer-shaped result passing the canonicality check does not establish C calling-convention compatibility.

- **Clarify `classmethoddescr_call`.** §D says its check-then-borrow goes because “its result is returned as it is.” [`classmethod_get`](/home/power/code/cpython/.worktrees/boil-6d/Objects/descrobject.c:551) returns the bound callable, which must then be invoked. State which value helper invokes it with the remaining tuple items and keyword dict; remove `_PyTuple_Objects` if that helper accepts values.

- **Make the lifetime ladder concrete.** Tests 6 names suites and stress settings, not the promised cases. Carry forward note 12 §9’s large prepend array, keyword arguments, and allocation-failure cases. The receiver test should assert that the injection fired and cover failed boxing before any `Py_TYPE(self)` access.

- **Complete the documentation changes.** §G should also update `call.rst`’s `tp_call` prototype and `typeobj.rst`’s assertion that its signature matches `PyObject_Call`.

## Questions

- Which step now owns super’s receiver box, `_PyObject_LookupSpecial`’s object bridge, and the `__length_hint__` box? Work doc 6d-2 §§C/F and `box_audit.py` assign them to rung 3; this document leaves super as a possible residue and omits the others. Record the revised owners and retirement steps.

- Which 10a merge supplies the descriptor-call spans and non-collecting classification of `_PyValue_CheckSlotResult`? Record the final frame classification and checks for the changed functions; “no new error” in `call.c` alone does not cover that integration.

## Verdict

revise
---
Reviewer: codex, 2026-10-28 04:36 PDT
