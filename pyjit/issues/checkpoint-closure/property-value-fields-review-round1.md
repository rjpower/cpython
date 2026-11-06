# Property value fields: first source review

The initial `w5-property-value-fields.patch` has two must-fix ownership errors:

1. The object descriptor getter created an owned value from `Py_NewRef(obj)` and returned through `_PyValue_THUNK_EXIT` without closing that owner. Every object-slot access leaked an instance reference. Keep the value-slot result, close the argument owner, then convert the result at the object boundary.
2. The `__doc__` and `__name__` value setters used `PyValue_DUP(value)` when deletion passes `PyValue_NULL`. `PyValue_DUP` asserts a non-null argument in debug; the former object setters accepted deletion and cleared their fields. Use `PyValue_XDUP(value)` before publishing the new field and closing the old owner, and test both delete operations.

No further source blocker was found in the field snapshots across callbacks, `property_copy`, descriptor calls, GC traversal/clear, or the specialization type guard. This is a conditional review; a refreshed frozen patch and focused runtime tests are needed before integration.
