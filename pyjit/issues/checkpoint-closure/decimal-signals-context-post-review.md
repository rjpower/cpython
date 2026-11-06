# Decimal signals/context increment post-review

Reviewed `decimal-signals-context.diff` against W5 source. **Approved; no
must-fix finding.** The patch changes only the signal list/dict value access
and five context scalar setters. It does not claim that Decimal object-slot
adapters are value-native.

`PyList_AppendValue` and `PyDict_SetValue` borrow the same held exception
classes and bool objects as the old object APIs. `list_as_flags` owns each
`PyList_GetValue` result until `exception_as_flag` returns; the latter only
compares identity with held signal classes, so passing null for an inline
item follows the existing invalid-signal KeyError path without a projection.
`dict_as_flags` owns each `PyDict_GetValue` result through `_PyValue_IsTrue`,
including a reentrant `__bool__`, then closes it before checking the result.
Absent key and error statuses remain separate. `signaldict_setitem` preserves
its deletion check before the borrowed value truth test. The five context
setters retain their `-1 && PyErr_Occurred()` and range-error paths while
using the extractor that implements their prior `PyLong_AsSsize_t` call.

The author reports release `_decimal` compile/link and 70 targeted
Context flags/input/API tests passing. The next increment must keep conversion
and slot work separate, with heap Decimal owners and context/trap status
checked independently.
