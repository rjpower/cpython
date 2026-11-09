# Fixed elapsed decision protocol

Declared 2026-11-01 before either active prototype has native feedback or scores.
This narrows the [budget](budget.json); it does not extend any execution window.

Use the existing quiet benchmark runner and unchanged registered driver/kernel
bytes. Bind actual source, interpreter, standard library, extensions and GCC/O3
configuration before execution. Use CPU0/sibling8, both existing machine slots,
build locks and the existing maximum-load admission of 4. No competing native
or bulk analysis runs in a quiet window. Hardware counters are diagnostic and
are not required for an elapsed decision.

## Early storage pilot

Compare the source-frozen span candidate, unchanged qualified tracing parent
and pinned upstream on coroutines, generators, comprehensions and Richards,
using their registered loops. Run two fresh-process rounds, five serial samples
after the registered warmup in each process. For row index i and round r,
rotate the three-arm order by (i+r) modulo 3. This gives 24 processes and 120
elapsed samples. Record whole-process peak RSS for each process.

The storage hypothesis requires coroutine candidate/parent median elapsed at
most 0.99 in each round, with a four-row geometric mean at most 1.0 in each
round. A control regression above 2% requires an explanation before adoption;
another row cannot erase it. Report each raw vector, spread, RSS and occupancy
facts. Clear losses are rejected promptly. Borderline or inconsistent results
earn no acceptance and no automatic extra samples.

Report absolute, parent and upstream RSS. Increased memory must be explained
by actual backing/slot occupancy and weighed against elapsed benefit. Persistent
growth in the fixed lifecycle tests or an ownership failure blocks acceptance.
There is no arbitrary evaluator-text or RSS percentage veto.

The unchanged parent measures the net source change, including common lookup
cost. A further source ablation is deferred unless it would decide a specific
remaining attribution question; it is not a prerequisite to rejecting a loss.
This supersedes the earlier storage proposal's mandatory extra ablation arm.

## Full parity decision

When a pilot warrants it, compare one coherent candidate and pinned upstream
on all 29 rows frozen in budget.json. Run three fresh-process rounds with five
serial samples after warmup per arm/row, giving 174 processes and 870 elapsed
samples. Alternate the first arm by (row index + round index) modulo 2.

For each row, take each arm's median across its 15 retained values, normalize
by registered loops, and divide candidate by upstream. The geometric mean of
the 29 ratios must be at most 1.0. Also require each round's corresponding
geometric mean, using its five-value medians, to be at most 1.0. Crossing parity
between rounds is inconclusive. These are consistency rules, not a confidence
interval: serial samples within a process are correlated.

Retain all rows and regressions, raw vectors, min/max/spread, per-round results,
peak RSS, failed admission and cleanup. Do not drop slow rows, use an isolated
GC timer as the score, add adaptive samples or infer composition gains by
adding isolated speedups. A changed candidate needs a fresh source binding and
its own declared finite activation, clamped to the hard cutoff. Performance
parity does not replace the required final correctness qualification.
