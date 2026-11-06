# Tuple getter value descriptor

Apply `tuplegetter-value-slot.patch` to the main runtime after the current source freeze. The new value descriptor returns the owned `PyTuple_GetValue` item directly; the object descriptor is a true `_Py_VALUE_THUNK_DESCRGET` boundary. This removes the observed 3,758,000 CORE boxes at `_collectionsmodule.c:2911` without changing tuple field ownership or descriptor error behavior.

The value implementation preserves `descriptor.__get__(None, type)` and missing-instance access, tuple subclass acceptance, the old TypeError text for other receiver types, and the same bounds check and `IndexError`. The generated heap-slot machinery recognizes `_Py_value_descr_get` as a paired slot.

Source patch: `git apply --check` passed against the runtime preimage. A syntax-only compile of the patched C unit using the existing release build flags passed. No runtime interpreter was modified for this packet. At integration, run the namedtuple portion of `test_collections` plus a small descriptor probe for missing/None receiver, tuple subclass, non-tuple receiver, out-of-range index, and inline int/float field values. Recount the namedtuple workload under stats; its CORE tuplegetter site should disappear and object-descriptor calls should carry THUNK attribution.
