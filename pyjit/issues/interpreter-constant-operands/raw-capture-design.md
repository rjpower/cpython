# Preserve original instruction measurements

Hypothesis: the current benchmark runner loses evidence needed to audit its
normalized instruction differences. It overwrites `perf.txt` and removes its
temporary directory. Fix that evidence loss before the held quiet comparison;
do not change measured work, loop counts, normalization or sample order.

Proposed focused change to the primary checkout's `boil/bench/run.py`:

- Always include each original low/high integer pair and its numbered
  invocation IDs in the successful instruction result, alongside the existing
  normalized values. Preserve the existing skeleton-subtraction `raw` field.
- Add a tool argument `--perf-records FILE` for an exclusive new JSONL journal.
  The quiet manifest will require it. It is a measurement artifact option,
  unrelated to runtime architecture or operand selection. Create it before
  `build_info()` can invoke the target; reject a path aliasing the report output.
- Journal a numbered start before every perf invocation, including probe and
  warmup. Journal completion with exact argv, purpose, return code, stdout,
  stderr, original perf text, parsed count or error. Flush/close each append
  outside the measured child. A terminated attempt remains visibly incomplete.
- Remove only the owned scratch `perf.txt` before each invocation, so a failed
  tool cannot inherit the previous invocation's count. Preserve complete
  stdout/stderr and perf text before shared error formatting can truncate them.
  Record spawn errors and missing output explicitly, then propagate the error.
- Keep existing subprocess failure reporting shared. Preserve the public
  scalar count return from `perf_instructions` and current normalized report
  fields. Add the journal path/hash to successful run metadata. Failed runs
  still produce no measurement report; their journal remains for diagnosis.
- Refuse an existing journal before invoking a target. Do not collect a full
  environment or introduce a new benchmark/cache/build directory policy.

Tests will use mocked perf subprocesses, never a target or actual perf counter.
Cover known low/high normalization and invocation mapping, raw successful
records, nonzero tool exit, invalid counter text, spawn/missing-output errors,
stale scratch rejection, output aliasing and journal survival after scratch
cleanup. Use a dedicated module that imports `run.py`, not `test_run.py` whose
decorators invoke perf. Keep the standard-library unittest harness, temporary
fixtures and no new dependency. Review source and tests independently before
any native smoke or quiet measurement.

The runtime source/image candidates remain frozen. The primary runner can
measure both images using the same driver and unchanged workload bytes; the
eventual manifest must bind this exact tool revision and both image/source
bridges. This proposal authorizes no timing. Allow twenty minutes for the
focused host implementation after independent design review.
