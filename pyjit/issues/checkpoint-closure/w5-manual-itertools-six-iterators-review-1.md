# Six itertools iterators: first source review

The frozen `w5-manual-itertools-six-iterators.patch` has two blocking status errors. Do not integrate this version.

`dropwhile`, `islice`, `compress` and `filterfalse` previously called the input's raw `tp_iternext`. A custom iterator raising `StopIteration("payload")` leaves that exception pending, and the wrapper propagates its arguments. The patch uses `_PyValue_IterNext`, which consumes the exception and returns a bare end status. These four producers need `_PyValue_IterStep`; their non-NEXT paths must pass through its owned `PYGEN_RETURN` payload or pending error unchanged. In `islice`, remove the `PyValue_XCLOSE` on a non-NEXT status before returning it, because the outer thunk owns and closes any return payload. `batched` intentionally clears StopIteration and `cycle` used `PyIter_Next`, so their use of `_PyValue_IterNext` is appropriate. A direct custom-iterator probe confirms all four wrappers preserve `StopIteration.args == ("payload",)` in the pre-port behavior.

The partial-batch branch in `batched_next_value` calls `_PyTuple_Resize(&result, i)` and then always returns `PYGEN_NEXT`. On resize failure, `result` becomes NULL with an exception. The old object slot returned NULL/error; the new status violates the NEXT/non-null invariant. Check the resize result and return `PYGEN_ERROR` with `PyValue_NULL` on failure.

The callback order, stored callable field ownership, GC traversal and normal yielded-value transfer otherwise match the reviewed paths. A corrected packet needs focused `test_itertools`, the payload probe for all four wrappers, and a narrow resize-failure check or source proof before post-review approval.
