## Summary

The in-place signatures and whole-family cut fit the architecture.
The inventory and conversion rules still leave build failures, semantic regressions, and avoidable boxing.
This is a source review; rerunning the inventory was blocked by the read-only uv cache.

## Blocking issues

1. **The inventory misses entire getset families.** Where: **Inventory**, **Risks**, and [getset_inventory.py](/home/power/code/cpython/.worktrees/boil-6d/boil/tools/measure/getset_inventory.py:6). The script misses scalar definitions (`subtype_getset_dict`, `subtype_getset_weakref`), macro entries (`G(...)` in `posixmodule.c`), and `pyexpat.c`’s macro-generated accessors and dynamically assigned `HandlerInfo.getset`. `pyexpat.c` is consequently absent from the file list. Its Clinic matcher also requires the casts this rung removes. **Fix:** cover these forms, accept cast-free macros, reject unresolved entries, and regenerate the function manifest after conversion. Add the corresponding XML tests to the ladder.

2. **The mechanical rules preserve unnecessary value-to-object round trips.** Where: **E.1–2**, [cellobject.c](/home/power/code/cpython/.worktrees/boil-6d/Objects/cellobject.c:138). Applying the rules literally produces `PyValue_FromResult(PyValue_Object(value))` in `cell_get_contents`; the setter boxes before `PyCell_SET` canonicalises that box again. Neither operation needs an object. Dictionary-backed setters such as `module_set_annotations` have the same problem. **Fix:** add an explicit rule for existing value storage and protocols: return the value directly, use `PyCell_SET_VALUE`, and use dictionary value APIs. Add zero-box and zero-allocation budgets for cell reads and writes with uncached ints and floats.

3. **`descr_get_object` cannot be retired as described.** Where: **E.3**, [typeobject.c](/home/power/code/cpython/.worktrees/boil-6d/Objects/typeobject.c:1871). It has five callers, not two: `type_get_doc`, `type_get_annotate`, `type_get_annotations`, `subtype_dict`, and `_PyObject_LookupSpecial`. The last remains object-returning by rung 3 §H. **Fix:** enumerate all five conversions. The four getters should consume checked descriptor values without boxing; `_PyObject_LookupSpecial` must retain its checked object conversion. Preserve the `tp_descr_get` check and its diagnostic, including the existing `DebugCheckTests.test_descr_get` cases.

4. **The generic dict getter’s specified conversion loses its failure path.** Where: **B**, [dictobject.c](/home/power/code/cpython/.worktrees/boil-6d/Objects/dictobject.c:6101). Both `_PyObject_EnterDictMode` and `ensure_nonmanaged_dict` can return `NULL`. `PyValue_FromObject(dict)` requires a non-null pointer; using it on allocation failure asserts or dereferences null. **Fix:** use `PyValue_FromResult`, or explicitly return `PyValue_NULL` before conversion. Test failed dictionary creation for both storage layouts, preserving the exception and existing attributes.

5. **Numeric setters need protocol-specific conversion rules.** Where: **E.2**, **Tests 4**, [exceptions.c](/home/power/code/cpython/.worktrees/boil-6d/Objects/exceptions.c:2420). `OSError_written_set` currently uses `PyNumber_AsSsize_t(arg, PyExc_ValueError)`: it accepts `__index__` and maps overflow to `ValueError`. The listed `PyLong_AsSsize_tValue` would reject indexable objects and raise `OverflowError`. Conversely, decimal’s setters deliberately use the stricter conversion. **Fix:** map each original conversion to its matching value API; specify `PyNumber_AsSsize_tValue(value, PyExc_ValueError)` here. Test indexable objects, overflow, bools, and deletion. Also enumerate and adapt decimal’s direct setter calls in `context_setattrs`; the configured builds do not compile that module.

6. **The commit partition contradicts the permitted getter-only state.** Where: **Commits 1–2** and **C**. Commit 1 includes all of C, which removes `getset_set`’s boxing and changes setter trampolines, while the setter typedef remains object-based until commit 2. That intermediate tree does not build. **Fix:** split C explicitly by family. Put setter dispatch, setter trampolines, and setter-comment retirement in commit 2; preserve rung 2’s setter bridge throughout the getter-only cut.

## Non-blocking issues

- **Record actual module coverage.** Both configured builds lack `_decimal`, `_ssl`, `_uuid`, and `_tkinter`; `_sqlite3` is disabled. Full-suite success cannot substantiate “the build finds every caller” for those files. List compile, runtime, or inspection coverage explicitly; Python fallback tests do not validate the C conversions.

- **Strengthen failure and lifetime tests.** Tests 4 should force collection after an uncached setter box and assert that collection occurred. Check the stored field after another collection, and inject boxing failure to prove that assignment raises `MemoryError` without deleting or replacing the old field. Any precise-frame anchor must hold the raw box: canonicalising it into an inline value does not root the box.

- **Make measurements discriminate.** `ndim`, `date.day`, and ordinary digest sizes already use cached small ints. Add an uncached result such as `memoryview.nbytes`. Define “heap float” concretely—NaN or a subclass—to exclude the acknowledged receiver box. Inspect constructors hidden in expressions too: `_hashlib_hmac_get_digest_size` returns its number through a ternary that `getset_classify.py` classifies as “object.”

- **Complete the signature rule.** Removing casts also requires normalising typed `self` parameters, for example `BinaryReaderObject *` in `_remote_debugging/module.c`. Changing only the return type, as E.1 states, leaves incompatible initialisers.

## Questions

- Which named step owns retirement of the Clinic numeric getters deferred in D?
- Rung 3 assigns `checked_tp_call_result`’s retirement to rung 4 if still present. It remains in [call.c](/home/power/code/cpython/.worktrees/boil-6d/Objects/call.c:318); how is that obligation reconciled with this rung’s explicit exclusion of `call.c`?
- Which hash implementation does the `hash.digest_size` audit exercise, and how are deferred implementations reported?

## Verdict

revise
---
Reviewer: codex, 2026-10-28 06:05 PDT
