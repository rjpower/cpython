# Slot-call patch post-review

Reviewed the slot-call patch against the clean runtime baseline, with emphasis
on typed-base dispatch, iterator producer ownership, and direct slot-call lint.
The first frozen patch had one must-fix R27 finding, corrected in the refreshed
source. A subsequent patch artifact had unrelated reverse bulk-port hunks;
the final artifact was regenerated from exact `58db4324fec` commit blobs and
contains only the 16 selected slot-call files. **The final source patch is
approved.**

In `Objects/floatobject.c`, `float_as_integer_ratio_impl` now invokes
`_PyValue_Lshift` and immediately projects its result with
`_PyValue_AsObjectStealAt(..., BOX_SITE_NUMBER)`. `_PyLong_ValueLshift`
returns inline integers for small results; for example, the denominator of
`(0.5).as_integer_ratio()` is shifted from one to two. This is a reachable
CORE box on an ordinary path. The function ultimately calls
`_PyTuple_FromPairTakeValue`, which accepts owned values. Keep numerator and
denominator as owned `PyValue` through the shift and transfer them to that
tuple, with the existing error cleanup adapted to close each owner once.
The old direct object slot returned a `PyObject *`, so this was a new box
introduced by the port. The refreshed source keeps numerator, denominator
and exponent as owned `PyValue` values, closes the old owner after each shift,
and transfers the final pair to `_PyTuple_FromPairTakeValue`, which consumes
both owners even on allocation failure. This resolves the finding. The author
reports the focused `test_floatasratio` and 54 `test_float` cases passing.

The earlier concern that the PyLong and PyDict base comparison projections
box normal boolean results was incorrect: `PyValue_True` and `PyValue_False`
are references to immortal bool objects, not inline booleans. Those exact
base-slot calls do not create a box on their ordinary result. Keep this
correction in the review record.

The three itertools value producers hold and close step results consistently
with their object iterator wrappers. `chain` uses `_PyValue_IterNext` where
the old object code used `PyIter_Next` and drops source/active exhaustion
values; `takewhile` and `starmap` use `_PyValue_IterStep` to preserve raw
iterator error/return behavior. The typed-base power helper selects the
existing `PyComplex_Type` value slot and otherwise adapts the exact passed
slot; it does not redispatch to the float base. The typed-base setter likewise
selects the passed base's value row and otherwise invokes the passed object
slot. The `repr` and rich-compare exports only change linkage. The checker
patch rejects direct slot calls in marked families while leaving mere reads
and copies nonfatal; its fixture does not certify indirect calls through
stored pointers. Those remain a separate finite audit.

The old Clinic-generated `chain_next` wrapper projected a possible inline
item in CORE. The final patch removes that private wrapper and its Clinic
producer, then uses `_Py_VALUE_THUNK_ITERNEXT(chain_next,
chain_next_value)`. The slot table still registers `chain_next` and the
paired value producer; `chain_next_value` retains the critical section.
The generated Clinic header removes only the obsolete wrapper. This fixes
box attribution without changing registration or locking. The author
reports `test_itertools` passing 140 cases (five skipped) after the change.

The author also reports a release build with all 117 modules and eight
focused test modules passing (1,480 run, 92 skipped), plus `test_float`
passing 54 cases (one skipped). The final patch was checked by the author
against exact clean commit blobs and reconstructs the 16 selected final W5
files; no inverse bulk-port hunk remains. Integration still needs merged
lint and focused tests.
