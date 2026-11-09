# Deque iterator source review

Reviewed the frozen `w5-manual-deque-iterator.patch` against the current deque and value-iterator contracts. No must-fix issue found.

`_PyValue_IterStep` deliberately leaves `PYGEN_RETURN` and `PYGEN_ERROR` unnormalized. The patch closes a returned value, then retains `finalize_iterator` to clear a pending `StopIteration` or propagate another error. For `PYGEN_NEXT`, the append helpers consume the owned `PyValue`, including on block-allocation failure. The Clinic critical section still surrounds the loops, and the `maxlen == 0` path still consumes to exhaustion. Luna reported a shadow release build with 117 module imports and passing `test_deque`; I did not rerun it.
