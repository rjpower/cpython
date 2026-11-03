## Summary

The in-place conversion fits the architecture and names retirement steps for its temporary bridges.  
Two correctness gaps remain: retained ctypes boxes and changed heap-setter semantics.  
Reviewed `27d98492f7f`, excluding concurrent edits; runtime probes were blocked by the read-only uv cache.

## Blocking issues

1. **The ctypes plan protects retained boxes only during the setter call.**

   **Where:** §D, ctypes fields; Tests 5/6. [`O_set`](/home/power/code/cpython/.worktrees/boil-6d/Modules/_ctypes/cfield.c:1035) stores a raw `PyObject *` in native memory. For a structure’s `py_object` field, [`KeepRef_lock_held`](/home/power/code/cpython/.worktrees/boil-6d/Modules/_ctypes/_ctypes.c:2968) saves that object through `PyDict_SetItem`, which canonicalises a numeric box into an immediate. [`PyCData_traverse`](/home/power/code/cpython/.worktrees/boil-6d/Modules/_ctypes/_ctypes.c:3014) visits the keep-reference tree, not the native cell.

   Once §D’s span is popped, nothing necessarily traces the box whose pointer remains in the field. Conservative stack copies can conceal this existing lifetime hole; reference counting provides no protection.

   **Fix:** retain the exact box through a traced holder in ctypes’ keep-reference tree, or trace the native object cells correctly. Keep the span through publication of that retaining edge. Add a test storing uncached ints and floats into a `Structure`’s `py_object` field, returning from the setter, removing incidental stack roots, collecting, and reading the field under stress and kill mode.

2. **The heap-type setter conversion changes accepted inputs.**

   **Where:** §C specifies `PyLong_AsLongValue` for `heapctypesetattr_setattro`. The current [`heaptype.c`](/home/power/code/cpython/.worktrees/boil-6d/Modules/_testcapi/heaptype.c:1337) first calls `PyNumber_Long`, then `PyLong_AsLong`.

   The proposed replacement rejects floats, numeric strings, and objects implementing only `__int__`, which the existing setter accepts. The current targeted test exercises only an integer and deletion.

   **Fix:** preserve the two stages with `PyNumber_LongValue`, check its result, then call `PyLong_AsLongValue`. Add float, string, `__int__`, conversion-error, and overflow cases; failed conversion must leave the field unchanged.

## Non-blocking issues

- **Allocation budgets contradict required boxes.** Test 11 assigns one allocation to `getattr(100000, "bit_length")`, but this requires an uncached receiver box and a bound-method object: two allocations. Likewise, Test 9’s `Pickler.persistent_id = 12345` requires the native-field box described in §C, despite Test 11’s blanket zero for forwarding stores. Give these branches explicit budgets.

- **The ctypes metaclass setters have another object boundary.** §C describes forwarding to `PyType_Type.tp_setattro`, but [`_structunion_setattro`](/home/power/code/cpython/.worktrees/boil-6d/Modules/_ctypes/_ctypes.c:1153) first passes `_fields_` to the object-form `PyCStructUnionType_update_stginfo`. Specify that branch’s conversion and rooting, including invalid numeric input and deletion.

- **State the failed-box/deletion distinction in §D’s bridges.** `getset_set` and `PyCField_set` must return immediately when boxing a non-NULL value fails. Passing that NULL onward can turn allocation failure into deletion; `func_set_defaults`, for example, accepts NULL as deletion. Add failure tests asserting both `MemoryError` and unchanged state.

- **Reconcile the debug-check specification.** §F initially says `Py_FatalError`, then explicitly forbids it. Note 12 §7 retains the old description. Update both and record the C checker’s classification of the actual stdio/`abort` helper.

## Questions

- What is the exact signature and failure contract of the new public `PyDict_GetItemRefValue`? Its declaration, documentation, tests, and commit placement are absent from §§G/Commits.
- When will §C’s promised per-function C-mode results be recorded? No “C mode” section is present. Which integration commit will also verify the receiver span after 10a promotes `method_vectorcall_*`?

## Verdict

revise
---
Reviewer: codex, 2026-10-28 02:28 PDT
