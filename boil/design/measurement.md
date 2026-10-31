# Performance measurement

The internal benchmark registry is `boil/bench/run.py`; workloads and drivers
live under `boil/bench/internal/`. Use the declared row/variant/loop set and
preserve every result, including regressions. Micro diagnostics explain a cost;
they are not extra rows in the application/kernel geometric mean.

## Comparable inputs

Each runtime uses its own source, standard library and compatible extensions.
Declare compiler, optimization, evaluator flags, frame pointers, zero-init,
JIT, PGO/LTO and backend policy. Controlled attribution and fastest-production
comparisons answer different questions. Preserve exact sources, configuration,
native products/aliases, imported Python origins and loaded ELF dependencies.

Run correctness first. Measurement plans state the hypothesis, owner, concrete
time budget, finite commands, load/admission limits and stop rule. Use existing
quiet/gate protocols; do not invent retries, expand sample counts or tune
workloads after seeing the score.

## Interpretation

- Ratio means baseline elapsed divided by candidate elapsed. Above one favors
  the candidate. State the baseline for every table.
- Report raw vectors, median, minimum, maximum and
  `100 * (max / min - 1)` spread. Five serial samples in a process are correlated,
  not independent randomized process pairs or a confidence interval.
- A geometric mean is a normalized workload comparison, not a weighted total
  application time. Do not infer an individual patch's effect from a union.
- Distinguish hot PID/time-filtered sample periods, hardware counters, GC/root
  timer windows and whole-process CPU/RSS. Inclusive categories overlap; a
  disjoint attribution gives each selected sample one category and retains
  unknown/lost/residual evidence.
- GC timers do not include every queued callback. A sampled residual is not
  automatically interpreter execution; absent samples do not prove zero calls.
- Preserve raw and adjusted instruction results. State the empty-loop correction
  and elapsed normalization. Invariant unused `x + y` diagnostics are not an
  `i += 1` recurrence; verify the executed path before interpreting arithmetic.

The [benchmark guide](../bench/README.md) describes the existing CLI. The
[landed results](../issues/gc-cleanup-scanning/performance.md) supply the current
comparison; [current direction](gc-and-interpreter-performance.md) uses that
budget without promising a general 2× improvement.
