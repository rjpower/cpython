# `_ctypes` b_objects home: rejected first packet

`ctypes-b-objects-home.diff` must not land. `cfield.c:O_set` writes the exact `PyObject *` input into foreign memory and returns a second raw reference as the keepalive token. Converting that token with `PyValue_FromObjectSteal(keep)` may inline and free an exact integer or float, leaving the foreign pointer dangling. The proposed `b_objects` value home therefore violates the foreign-memory lifetime contract despite passing focused tests.

The existing W5 `KeepRef_lock_held` dictionary branch has the same problem: `PyValue_FromObjectBorrow(keep)` can pass an inline word to `PyDict_SetValue`, then `Py_DECREF(keep)` frees the raw pointer held by `O_set`. This is inherited debt, not a new finding confined to the packet.

A raw pointer word cannot simply be forced into `b_objects` or the dictionary: `_PyValue_IsHome` explicitly rejects boxed exact numeric pointers in value fields and dict entries. The next design needs a separate GC-traversed raw `PyObject *` pin home for foreign pointer targets, including keyed replacement and cast/base sharing. The Python-visible `_objects` value may remain canonical. Test `py_object` fields and arrays with exact int/float inputs across GC, replacement and casting before any value-home migration is approved.

The existing `_PyValue_TpCallAdapter` does create a word-preserving argument tuple and reach normal `type_call`, `tp_new` and `tp_init`; mutable `Simple` subclasses and Python overrides can therefore use that path. This resolves the constructor *interface* question but does not resolve the foreign pointer keepalive defect.
