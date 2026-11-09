# W5 direct object-slot call mapping

This is the semantic review packet for the 17 direct slot calls reported by
`check_value_api.py` on the W5 compile database. No callsite edits are included
in this packet. Base-type calls must keep bypassing subclass overrides, and
raw iterator steps must keep their object-thunk status behavior.

| Callsite | Existing behavior | Proposed value path | Review point |
| --- | --- | --- | --- |
| `Modules/_collectionsmodule.c:2454` | Calls dict's base repr for defaultdict | `_PyValue_CallRepr(&PyDict_Type, PyDict_Type.tp_repr, repr_row, value)` | Preserve base repr when a subclass overrides repr. |
| `Modules/_ctypes/_ctypes.c:1215` | Calls `type`'s base `tp_setattro` from the ctypes metaclass setter | `_PyValue_CallSetAttrBase(&PyType_Type, PyType_Type.tp_setattro, self, key, value)` | The helper uses the value row when present; otherwise it adapter-boxes only the operands and invokes the exact supplied base slot. It does not redispatch or perform generic name handling. |
| `Modules/_json.c:1717,1826` | Calls int's base repr for int-subclass values and keys | `_PyValue_CallRepr(&PyLong_Type, PyLong_Type.tp_repr, repr_row, value)` | Preserve base repr when an int subclass overrides repr. |
| `Modules/_lsprof.c:734` | Exact `PyMethodDescr_Type` descriptor guard, then its descriptor getter | `_PyValue_DescrGet(callable, self_arg, Py_TYPE(self_arg))` | Convert the owned value result at the existing object boundary. |
| `Modules/itertoolsmodule.c:1632,1943,2096` | Three raw one-step `tp_iternext` calls in takewhile, starmap, and chain | Add value-native `_PyStatusValue` producers and register `_Py_value_iternext`; retain the object slot as a thin object-boundary wrapper | Propagate `PYGEN_RETURN` values and errors as the original thunk does. Do not box an item inside the core producer or use consumer `_PyValue_IterNext` to clear StopIteration. |
| `Modules/mathintegermodule.c:501` | Calls int's base rich comparison to bypass an int-subclass override | `_PyValue_CallRichCompare(&PyLong_Type, PyLong_Type.tp_richcompare, a, b, Py_LT)` | Keep exact base comparison semantics. |
| `Modules/posixmodule.c:7609` | `PySequence_ITEM` calls `sq_item` directly with a nonnegative index | `_PyValue_SequenceGetItem(value, i)` | Same item protocol; loop index needs no negative-index adjustment. |
| `Objects/dictobject.c:1185,1244` | Exact Unicode key hash; direct Unicode type slot | `_PyValue_Hash(value)` | Exact type guard makes generic hash dispatch equivalent; retain error handling. |
| `Objects/floatobject.c:321` | Calls the object `nb_float` fallback only when the value row is absent | `_PyValue_Float(v)` | Uses the same type slot/adapter and existing result checks. |
| `Objects/floatobject.c:963` | Calls complex's base ternary power slot after boxing operands | `_PyValue_CallPowerBase(&PyComplex_Type, PyComplex_Type.tp_as_number->nb_power, v, w, z)` | The helper selects complex's typed power row when available and otherwise adapts the exact passed base slot, avoiding redispatch through float power. |
| `Objects/floatobject.c:1735,1741` | Calls long's base `nb_lshift` on newly created exact ints | `_PyValue_Lshift` on borrowed exact values; materialize result at the object boundary | Confirm identical error behavior and ownership. |
| `Objects/odictobject.c:1605` | Calls dict's base rich comparison for OrderedDict | `_PyValue_CallRichCompare(&PyDict_Type, PyDict_Type.tp_richcompare, v, w, op)` | Preserve dict comparison independent of subclass overrides. |

The two typed-base calls are implemented by narrow helpers in `object.c` and
`abstract.c`; their reviewed interfaces preserve explicit base-slot dispatch.
`_PyValue_CallRepr` and `_PyValue_CallRichCompare` have exported declarations
because `_json` and `_math_integer` are shared extension modules. The iterator
ports use value-native `_PyStatusValue` producers and expose object thunk
wrappers only at each iterator type's public `tp_iternext` boundary.
