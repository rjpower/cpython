# Compiler constant-value boxing packet

`w5-flowgraph-const-values.patch` ports the remaining constant consumers in
`Python/flowgraph.c` from the object-returning `get_const_value()` helper to
`get_const_value_value()`. The two captured compiler PCs resolve through
`PyValue_AsObject()` to that helper: `0+0x460a36` is the common-constant arm,
and `0+0x460aa9` is the `LOAD_CONST` list lookup arm. The captured owner
counts include 2 common-constant and 42 list boxes for `go`, and 4 common-
constant boxes for `raytrace`.

The patch removes the object helper after all its consumers move. Constant
sequence folding stores each owned value directly into a freshly allocated
exact tuple with `PyTuple_SET_VALUE`; truth and `None` checks operate on the
owned value; folded booleans enter the constant cache through
`add_const_value()`. Existing error paths close the current value before
returning, while already-stored tuple elements remain owned by the tuple.

Source review: the coordinator reviewed ownership/error paths and approved the
recipe. A separate post-review is queued. No compile or runtime test was run
before the coordinator began the merged build; the coordinator owns the focused
`test_compile` and constant-folding probe in that integration run.

The separate attribute-thunk PC `0+0x40d079` resolves to
`Python/ceval.c:_PyEval_ImportFrom` at its object `PyObject_GetOptionalAttr`
call. This packet does not change import behavior. A value-returning
`IMPORT_FROM` path and exact miss fallback design remain under review; the
object-facing lazy-import replay remains a separate compatibility path until
its value ownership and callback ordering are accounted for.
