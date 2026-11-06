# W5 R27 gate post-review

Reviewed the frozen `w5-gate.diff` after the real-shape and stale-dump repairs.
No must-fix issue remains in this patch. The `check_r27` focused run reports
32 tests passing, one skipped; this review also inspected the benchmark writer
and pystats emit/reset paths rather than treating its fixtures as the schema.

`bench/run.py` writes `results.boxes[benchmark].raw_counters` with `warmup`,
`low` and `high` process maps. The gate now requires the 29 declared names,
those three maps, four `Boxing by kind` counters and three inline-box totals in
each map. A nonzero box in any map fails. Missing process and missing thunk
counter fixtures demonstrate the previous false-pass is closed. The fixed
suite directory is rejected if nonempty before launch, so old dumps cannot
satisfy the new positive-dump requirement. The suite also checks regrtest's
return code and matches each dump's kind total to owner rows plus dropped
owners. Pystats emits both tables in each dump and clears both on stats reset,
so the per-dump equality is meaningful.

The static gate now requires a nonempty compile database, readable units, no
deferred marks, declaration marks on inventory rows, and a reviewed
`r27_complete=true` assertion. The four test-unit exemptions exactly match
the W5 boundary and retain `_testclinic_value.c` as production coverage. The
nonempty `MARKED_SLOT_FAMILIES` check deliberately keeps static admission red
until the direct-slot matcher is implemented. Likewise, `r27_complete` must
only be set after an inventory audit; the marker asserts completeness but does
not discover missing rows on its own. These are outstanding implementation
work, not reasons to reject this fail-closed gate patch.

One operational limit remains: old benchmark result files created before the
box-kind counters were emitted cannot pass this gate; use a fresh stats-build
result from the current runtime. The checker accepts historical list-shaped
input only when it carries the same complete raw counter evidence.
