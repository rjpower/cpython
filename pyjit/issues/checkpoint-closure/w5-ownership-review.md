# W5 ownership inventory review

Audited the 36 pairs in `w5-tools/candidates.tsv`, the two in
`candidates2.tsv`, all 19 `Py_VALUE_COMPAT_NUMBERS` declarations, their value
prototypes in `Include/internal`, and the existing ownership table. The
standalone [processor-safe rows](w5-ownership-additions.toml) contain 29
surveyed pairs plus 19 numeric extractors: 48 rows, no duplicate object name
with the existing table, and an object declaration and value prototype for
each. These rows use the existing `borrow`, `keep`, `new`, `object` and `status`
categories. Here `status` means the unchanged non-value result, including
integer, floating and pointer scalars; amend the canonical table's explanatory
comment accordingly when integrating.

The other nine `[[manual]]` pairs in the same TOML are real twins but **cannot**
be handed to `value_sweep.py` as ordinary rows. Five change an object/out
result into `_PyStatusValue`; three add a `PyThreadState *` argument; one
changes an object pointer vector into a PyValue window. Port these sites
manually against the existing object thunk's status/error mapping and explicit
ownership. Mark each public object declaration only after its core callers
have been ported. The static R27 inventory audit must include these nine
separately; the current checker reads only `[[row]]`, so setting
`r27_complete=true` before the manual side is checked and reviewed would
be a false completeness claim.

The incompatible signatures, written as return type `(arguments)`, are:

| Object API | Value provider |
| --- | --- |
| `PyObject_GetOptionalAttr`: `int (PyObject *, PyObject *, PyObject **)` | `_PyValue_GetOptionalAttr`: `_PyStatusValue (PyValueBorrow, PyObject *)` |
| `_PyObject_LookupSpecial`: `PyObject * (PyObject *, PyObject *)` | `_PyValue_LookupSpecial`: `_PyStatusValue (PyValueBorrow, PyObject *)` |
| `PyMapping_GetOptionalItem`: `int (PyObject *, PyObject *, PyObject **)` | `_PyValue_GetOptionalItem`: `_PyStatusValue (PyValueBorrow, PyValueBorrow)` |
| `PyIter_Next`: `PyObject * (PyObject *)` | `_PyValue_IterNext`: `_PyStatusValue (PyObject *)` |
| `PyIter_Send`: `PySendResult (PyObject *, PyObject *, PyObject **)` | `_PyValue_Send`: `_PyStatusValue (PyObject *, PyValueBorrow)` |
| `PyObject_Call`: `PyObject * (PyObject *, PyObject *, PyObject *)` | `_PyValue_Call`: `PyValue (PyThreadState *, PyValueBorrow, PyObject *, PyObject *)` |
| `PyObject_CallOneArg`: `PyObject * (PyObject *, PyObject *)` | `_PyValue_CallOneArg`: `PyValue (PyThreadState *, PyValueBorrow, PyValueBorrow)` |
| `PyObject_Vectorcall`: `PyObject * (PyObject *, PyObject *const *, size_t, PyObject *)` | `_PyValue_Vectorcall`: `PyValue (PyValueBorrow, const PyValue *, size_t, PyObject *)` |
| `PyObject_CallNoArgs`: `PyObject * (PyObject *)` | `_PyValue_CallNoArgs`: `PyValue (PyThreadState *, PyValueBorrow)` |

The 19 numeric declarations currently carry conditional
`Py_VALUE_COMPAT_NUMBERS(value)` marks. Their value implementations are the
functions invoked by the object wrappers, so the scalar result and error
contract match. Do **not** run `value_sweep mark` on these unchanged headers:
its matcher does not recognize the conditional mark and would add a second
mark. As the numeric core call sites are ported, replace each conditional
mark with the normal unconditional `Py_VALUE_COMPAT(value)` and remove the
`Py_VALUE_MARK_NUMBERS` migration define where no longer needed. `PyLong_AS_LONG`
is an alias of `PyLong_AsLong` and is listed as such. `PyLong_AsPid` is a
platform-dependent alias or inline implementation and must be checked at its
actual call sites rather than assigned one unconditional row.
All 19 value providers take `PyValueBorrow` in place of the first
`PyObject *`; any remaining integer, pointer or flag arguments are unchanged.
Their exact return C types also stay unchanged: `int` for `AsInt`, `AsInt32`,
`AsUInt32`, `AsInt64` and `AsUInt64`; `long` or `long long` for the corresponding
`AsLong*`; `Py_ssize_t` for `AsSsize_t` and `AsNativeBytes`; `size_t` for
`AsSize_t`; `unsigned long` or `unsigned long long` for the unsigned forms;
`double` for both `AsDouble` providers; and `void *` for `AsVoidPtr`.

This is a reviewed candidate packet, **not** a proof that the ownership
inventory is complete. In particular, `PyMapping_GetOptionalItemString` has
a value twin but was absent from the supplied candidates. Continue the
declaration census before setting the R27 completeness marker.
