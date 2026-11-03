## Summary

The rung’s scope is sound: counters, measurements, and the final record.  
The proposed occupancy metric is invalid, and conversion reasons can overlap or count failures.  
The benchmark selection and stats isolation need revision before the results support the stated claims.

## Blocking issues

1. **Dictionary-mode counters lack a single successful-transition accounting point.**

   **Where:** §1’s reason table; [shapeobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/shapeobject.c:644), [object.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/object.c:1412), and [typeobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/typeobject.c:7425).

   **Problem:** Both a non-exact-name load and `__class__` assignment call `_PyObject_EnterDictMode()`, so counting there as `on_request` overlaps their specific reasons. Existing increments precede conversion, so simply replacing them counts failed attempts. The capacity predicate also misclassifies a REMOVE at full capacity: deletion needs a new shape, not additional capacity. `del obj.__dict__` reaches `_PyObject_SetSlotDict(NULL)`, but the table describes assignment only.

   **Fix:** Pass the reason through the existing conversion path and increment exactly once after successful publication. Count direct dict replacement only when it first changes the mode; explicitly classify dict deletion. Define precedence when ADD hits both limits; REMOVE can hit only the shape limit. Test the complete counter vector, repeated operations, and allocation failures, including `_PyObject_GetDictPtr()`’s suppressed-error path.

2. **“Slots used” measures cumulative additions, not fixed-capacity waste.**

   **Where:** §1, lines 32–41; allocation sites in [typeobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/typeobject.c:2325) and [gc_live.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/gc_live.c:1722); transition handler in [bytecodes.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/bytecodes.c:2885).

   **Problem:** Repeating `o.x = 1; del o.x` reuses existing transitions indefinitely. One six-slot allocation can produce arbitrarily many ADD commits, so the stated error is unbounded. Conversion also leaves allocated inline storage behind while subsequent dict stores stop contributing. Accounting at `_PyType_AllocSlotsSize()` counts allocation attempts unless placed after successful allocation.

   **Fix:** Rename the event counter to `inline_slot_adds` if useful, and measure occupancy separately over a defined live population using shape metadata, without materialising `__dict__`. Count allocated slots only after allocation succeeds. Specify treatment of holes, dictionary-mode objects, and cross-capacity `__class__` changes. Test generic and warmed transition paths, deletion/re-addition, conversion, and failed allocation.

3. **The shared-directory fallback cannot attribute suite statistics safely.**

   **Where:** §2’s suite measurement and Risks; [pystats.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/pystats.c:428) and [initconfig.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/initconfig.c:2723).

   **Problem:** This build hard-codes `/tmp/py_stats`; `PYTHONSTATS` enables collection but does not select a directory. A before/after listing includes files written by concurrent processes. Unlike the benchmark runner’s single-child check, a multiprocess suite cannot identify contamination by expecting one new file. Summing and deleting that difference can consume another run’s data.

   **Fix:** Establish actual isolation before running: support a per-run output directory inherited by workers, or use a private filesystem namespace. Remove the directory-difference fallback. Aggregate and delete only owned files, and verify isolation with concurrent writers.

4. **The selected microbenchmarks miss two expressly required cases.**

   **Where:** §2; note 09 §9; rung 4’s hand-off; [attr_load_poly.py](/home/power/code/cpython/.worktrees/python-boil-the-ocean/boil/bench/internal/micro/attr_load_poly.py:14), [attr_store.py](/home/power/code/cpython/.worktrees/python-boil-the-ocean/boil/bench/internal/micro/attr_store.py:4), and [attr_store_poly.py](/home/power/code/cpython/.worktrees/python-boil-the-ocean/boil/bench/internal/micro/attr_store_poly.py:6).

   **Problem:** `attr_load_poly` exercises two shapes, not the specified five-shape site that exceeds the four-row cache. Both selected store fixtures initialise and write inline integers, so neither establishes the cost of the vacuous BOXED-kind guard promised by rung 4.

   **Fix:** Add separately named five-shape and monomorphic BOXED-field variants. Confirm the intended plan and warmed handler before measuring. Retain existing rows for historical comparisons, and run each comparison with the same benchmark source.

## Non-blocking issues

- **Explicitly gate the Py_STATS tests.** §4 names gates without build/test arguments. The new counter tests skip on ordinary release and debug builds. Require gate 1 on dev-pystats, including the counter tests and dump/summary integration.
- **Reconcile the architecture’s earlier statements.** Adding note 06 §20 alone leaves §3.3 saying kind changes transition shapes and describing members as non-transitioning shapes. Its §6 deletion list also removes handlers that note 09 retains. Update those statements to the final design.
- **Tighten attribution language.** §2 permits “the step’s change it follows from” as a cause. Require a demonstrated explanation; otherwise label it a hypothesis and carry it forward. Instructions cannot establish dependent-load latency when wall time is unavailable.

## Questions

- What is the transient-class measurement protocol: current or peak RSS, collection points, weak-reference checks, and repeated batches? With no reference counting and conservative C-stack roots, one RSS delta cannot distinguish live retention, stale roots, and allocator reservation.
- Will the fan-out wrapper follow `driver.py`’s `setup(variant)` contract and report counter deltas? `chaos` and `json_loads` require setup; the generalisation counters are process-global. Decide whether setup and cold-start generalisations belong in the reported number.
- Where will the rebuilt `a9e02dccada` baseline be stored? Preserve the original `6c-rung3.json`, which predates the storage-marking fix, and record the new baseline’s source revision, benchmark revision, and build configuration separately.

## Verdict

revise
---
Reviewer: codex, 2026-10-27 21:57 PDT
