# Deque iterator value-step port

Baseline: S worktree `build/checkpoint-bulk-20261105` at `e983958bedeca23a192f3a566d2f27b4a0c9a7db`.

This patch updates the three `tp_iternext` loops in `consume_iterator`, `deque_extend_impl`, and `deque_extendleft_impl` to use `_PyValue_IterStep`. Successful item owners are either closed in the maxlen-zero consumer or transferred directly to deque append, which consumes the owner on both success and allocation failure. On every non-NEXT result, a returned generator value is closed before the existing finalizer handles pending StopIteration or other errors.

Validation: S's translation unit compile remains blocked by nine unrelated marked compatibility calls elsewhere in `_collectionsmodule.c` (`PyObject_CallOneArg`, `PyIter_Next`, and `_PyType_LookupRef`). A W5 shadow build using the shared release lock imported all 117 modules with no failures; `test_deque` passed as part of the combined locked validation (619 tests run, 11 skipped across ten focused modules). The W5 source was restored and rebuilt while still holding the lock. Review is pending.
