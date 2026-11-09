# R27 ownership declaration census, next pass

This pass compares the existing 98-row ownership table, the 48 processor-safe
and nine manual additions in `w5-ownership-additions.toml`, marked public and
internal declarations, and object entry wrappers with value implementations.
It is **not** a completeness certificate. In particular, the compiled Linux
module set and indirect slot calls still require disposition before
`r27_complete = true`.

## Additional exact pairs

`w5-ownership-census-additions.toml` records six further manual pairs. Their
object declarations and value providers exist in the W5 source:

| Object entry and signature | Value provider and signature | Port condition |
| --- | --- | --- |
| `PyMapping_GetOptionalItemString`: `int (PyObject *, const char *, PyObject **)` | `_PyValue_MappingGetOptionalItemString`: `_PyStatusValue (PyValueBorrow, const char *)` | Preserve found/absent/error, the original C string, and owned output. |
| `PyIter_NextItem`: `int (PyObject *, PyObject **)` | `_PyValue_IterNext`: `_PyStatusValue (PyObject *)` | Preserve its explicit non-iterator TypeError before stepping. |
| `_PyMapping_GetOptionalItem2`: `PyObject * (PyObject *, PyObject *, int *)` | `_PyValue_GetOptionalItem`: `_PyStatusValue (PyValueBorrow, PyValueBorrow)` | Preserve the separate status output and absent/error distinction. |
| `_PyType_LookupRef`: `PyObject * (PyTypeObject *, PyObject *)` | `_PyType_LookupValue`: `_PyStatusValue (PyTypeObject *, PyObject *)` | Keep the found value owned; status zero means absent. Do not treat the object thunk's possible projection failure as a provider error. |
| `_PySuper_Lookup`: `PyObject * (PyTypeObject *, PyObject *, PyObject *, int *)` | `_PySuper_LookupValue`: `_PyStatusValue (PyTypeObject *, PyValueBorrow, PyObject *, int)` | Pass `want_method = method != NULL`, and set `*method` only for status one. |
| `_PyObject_Call`: `PyObject * (PyThreadState *, PyObject *, PyObject *, PyObject *)` | `_PyValue_Call`: `PyValue (PyThreadState *, PyValueBorrow, PyObject *, PyObject *)` | Preserve the existing precondition assertions and call-accounting increment at ported call sites. |

These rows belong in `[[manual]]`, because the current processor handles
neither `_PyStatusValue` output rewrites nor the object/result control-flow
changes. Their presence in the inventory alone does not port a caller.

## Recipes for the nine earlier manual pairs

* `PyObject_GetOptionalAttr`, `_PyObject_LookupSpecial`, and
  `PyMapping_GetOptionalItem`: read `_PyStatusValue.status` before the value.
  Status one transfers an owned value to the caller; zero is absence without
  an exception; minus one propagates the error. Keep receiver and key/name
  owners alive across descriptor or mapping callbacks. Close the result on
  every branch that does not transfer it. Preserve KeyError suppression in
  the mapping operation.
* `PyIter_Next`: `_PyValue_IterNext` provides an owned item on `PYGEN_NEXT`,
  null on normal exhaustion, and null with an error on failure. The iterator
  and any yielded value remain live through consumer callbacks. Do not replace
  it with raw `_PyValue_IterStep`, whose StopIteration semantics differ.
* `PyIter_Send`: map `PYGEN_NEXT`, `PYGEN_RETURN`, and `PYGEN_ERROR` to the
  existing `PySendResult` and output contract. A yielded or returned value is
  owned; transfer or close it exactly once. Keep the iterator and sent value
  alive across the callback. Preserve the explicit returned value, including
  inline None or a scalar.
* `PyObject_Call`, `PyObject_CallOneArg`, and `PyObject_CallNoArgs`: pass the
  active `PyThreadState *` where the value signature requires it, keep the
  callable and argument owners through arbitrary Python callbacks, and
  transfer or close the owned `PyValue` result. Preserve each wrapper's
  argument validation and error state; core callers must not project the
  result merely to keep their old local type.
* `PyObject_Vectorcall`: construct a value argument window with positional
  and keyword tails in the same order, preserve the
  `PY_VECTORCALL_ARGUMENTS_OFFSET` scratch slot and its lifetime, hold the
  source arguments until the call returns, and close/transfer the owned result.
  A pointer cast from `PyObject *const *` to `const PyValue *` is invalid.

## Further declaration dispositions needed

The object-entry scan also found `PyCFunction_GetSelfRef` and
`PyMethod_GetSelfRef`. Their available value providers are the borrowed
`_PyCFunction_GET_SELF_VALUE` and `_PyMethod_GET_SELF_VALUE` macros, not
new-reference functions. A core port must validate the receiver exactly as
the object API does, distinguish null self, and duplicate the borrowed value
before returning it. Record these as manual macro-provider pairs once their
remaining callers and marking policy are reviewed.

`PyObject_GenericGetAttr`, `PyObject_GenericSetAttr`, and
`_PyObject_GenericSetAttrWithDict` have value implementations; their object
functions also serve type-slot interfaces. `_PyObject_GenericGetAttrWithDict`
calls a private `generic_getattr_value(..., suppress)`; only the suppressing
lookup and ordinary get variants are exported. This needs an explicit
value-helper export or a bounded manual port, not an invented direct twin.
The checker must distinguish direct core calls from object-slot registration.

`PyVectorcall_Call` and `_PyObject_Call_Prepend` construct value windows but
have no signature-equivalent value provider. The former takes a tuple/kwargs
object and the latter prepends an object; a port must preserve their window
layout and error behavior. The 17 `PyArg_*` entry wrappers are variadic
format-parser bridges, not one-for-one scalar value APIs. The object slot
adapters `_PyDict_Subscript`, `_PyDict_StoreSubscript`,
`_PyObject_NextNotImplemented`, `_Py_BaseObject_RichCompare`, the `_Py_slot_*`
getattr adapters, `_Py_type_getattro`, `_PyValue_TpCallThunk`, and
`_PyAsyncGenASend_Send` need slot-boundary treatment, not automatic ownership
rows. These exclusions need a reviewed disposition in the final inventory.

The 19 numeric extractor declarations still use conditional
`Py_VALUE_COMPAT_NUMBERS` marks. Replace each conditional mark with one
ordinary `Py_VALUE_COMPAT` mark only after core callers are ported; adding a
second mark is invalid. `PyLong_AsPid` is platform-dependent and needs a
platform-specific call-site check. The `_decimal` compiled-module check is
still pending.

## Gate and indirect calls

The minimum checker change is to validate marks over
`inventory["row"] + inventory.get("manual", [])`, with object/value string
validation and duplicate object-name detection. Keep `value_sweep.load_table`
limited to `[[row]]`; manual entries must never enter the rewriter. Assert
`r27_complete` only after the reviewed inventory has been merged and the
compiled-unit and call-site audit passes. An inventory mark proves a
declaration is classified; it does not prove every caller is ported.

Direct slot-call lint misses stored-then-called slot pointers. The finite
candidate scan includes `_bisectmodule.c` (`sq_item`, `tp_richcompare`),
`_collectionsmodule.c` and `itertoolsmodule.c` (`tp_iternext`),
`mathmodule.c` and `bytearrayobject.c` (`tp_iternext`), `listobject.c`
(`key_richcompare`), and `Objects/call.c` (`tp_call`). Each needs a
source-level disposition: value-native producer/caller, genuine object API
boundary, or an explicitly reviewed remaining violation. Reads that only
compare or test a slot pointer are different. Do not infer that a checker
fixture covering a copied pointer proves these invocations safe, and do not
build an unbounded dataflow checker to solve this finite list.
