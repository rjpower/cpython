# R27 manual aliases for wrappers with no production callers

Reviewed against runtime source `42696e9fcca276d0cc1667a9d20613378589da67`.
This is a metadata proposal only. It does not claim that the R27 inventory is
complete and does not set `r27_complete`.

## Fresh caller census

A source scan over `Include/`, `Objects/`, `Python/`, and `Modules/` found only
the declarations and definitions below. It found no production call sites for
these wrappers. This closes the stale caller rows in
`r27-current-inventory-gap-audit.md`: SQLite no longer calls
`PyMapping_GetOptionalItemString`; the earlier `dictobject.c` and
`memoryobject.c` calls to `PyIter_NextItem` are also gone. The two public APIs
still appear in `_testcapi` compatibility probes. Those test-only wrappers
exercise the object contract but do not block a core alias.

| Object wrapper | Value provider | Contract and reason it is an alias |
| --- | --- | --- |
| `PyIter_NextItem(PyObject *, PyObject **) -> int` | `_PyValue_IterNext(PyObject *) -> _PyStatusValue` | The wrapper keeps the explicit non-iterator TypeError check, then adapts NEXT/RETURN/ERROR to the public 1/0/-1 status and new-reference output. The shared iterator provider already has an inventory row for `PyIter_Next`. |
| `PyMapping_GetOptionalItemString(PyObject *, const char *, PyObject **) -> int` | `_PyValue_MappingGetOptionalItemString(PyValueBorrow, const char *) -> _PyStatusValue` | The wrapper preserves found/absent/error as 1/0/-1 and boxes the owned result only at the object API boundary. The value provider creates and releases the Unicode key from the C string. |
| `_PyMapping_GetOptionalItem2(PyObject *, PyObject *, int *) -> PyObject *` | `_PyValue_GetOptionalItem(PyValueBorrow, PyValueBorrow) -> _PyStatusValue` | The wrapper unpacks the provider's status into `*err` and adapts its owned result to the object return. It is the internal tail-call shape of `PyMapping_GetOptionalItem`; no production caller remains. |
| `_PyLong_GCD(PyObject *, PyObject *) -> PyObject *` | `_PyLong_ValueGCD(PyValueBorrow, PyValueBorrow) -> PyValue` | The wrapper adapts the owned value result at the object return boundary. It has no production caller after math and decimal consumers moved to the value provider. |

The corresponding object declarations should carry `Py_VALUE_COMPAT` with
their existing value provider. Add four `[[manual]]` rows to
`Tools/build/value_ownership.toml`. The value-sweep table intentionally does
not rewrite calls from these records; the manual inventory makes the checker
validate the object/provider pairing without pretending that their signatures
are mechanically interchangeable.

## Validation

Use the checker fixture to require each manual object declaration's exact
`Py_VALUE_COMPAT(provider)` annotation and provider declaration. A positive
table passes; removing any one compatibility mark must produce the matching
missing-mark diagnostic. Do not add a completion flag or broaden the change to
the unrelated inventory gaps.
