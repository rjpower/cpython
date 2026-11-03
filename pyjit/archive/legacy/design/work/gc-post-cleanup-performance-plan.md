# Post-landing performance reuse plan

2026-10-30. Measurement protocol. Run only after the actual cleanup/scanner/warnings
union is qualified, landed and the runtime/quiet window is released. Native
correctness and build receipts take priority. No measurement or new runner is
created here.

Executed2026-10-30 on qualified0b15/integrationaee46; finite outcome and raw
replay independently clear. [Results](gc-post-cleanup-performance.md) retain the
actual330 timing values, diagnostics, profiles, source identities and limits.
The protocol below is retained for the next matched structural comparison.

Use the qualified ordinary O3 GCC cohort: frame pointers including leaf frames,
zero initialization, evaluator `-fno-gcse`, no JIT/PGO/LTO/tail/Py_STATS, actual
off/extras0/stress0. Reuse pinned upstream 321925's accepted eval-nogcse cohort;
bind the actual final candidate image, runtime Lib, compiler/config and ELF
loads separately. Do not call a docs-only integration union-built. Preserve all
earlier scores and the failed selective campaign.

## Three distinct comparisons

1. **Refresh all 29 versus upstream.** Rebind the existing
   `build/checkpoint-i-qualified-refresh/quiet29/{paired.py,phase_runner.py}`
   and its registered row/loop/variant vectors. Refresh candidate origins;
   reuse the upstream origins only if its exact source/products/ELF and shared
   driver/kernel bytes still match. Keep the existing two RUNS 0 JSON-tail
   witnesses. One fresh process per runtime/row, unchanged warmup and five
   serial samples, alternating first runtime, retain all 290 values. Use CPU0,
   sibling8 idle, both global slots and ordinary build locks, existing load
   rejection and cleanup. Report29 upstream/candidate ratios, geometric mean,
   regressions and `100*(max/min-1)` sample spread (percent), matching the baseline. Prior run took 77s; reserve roughly
   3min for targets and existing admission/cleanup, with the accepted 900s ceiling.

2. **Same-fork causal cleanup comparison.** Rebind
   `boil-gc-selective-death/build/selective-death-paired-four`'s existing paired
   protocol to retained qualified 75e8 products (serving 4da/runtime DAC6/native 3c6)
   versus the landed union. Four rows only: coroutines, comprehensions,
   generators, Richards, registered loops and five serial samples per side.
   Alternate first side exactly as before and retain all 40 values. This
   estimates the combined selective/native/scanner/warnings change, not an
   isolated benefit for any one patch. Do not compare only to the historical
   selective timing or attribute upstream differences to GC. Allow about 1–2min
   including guards; a separate quiet reservation is unnecessary if it follows
   the 29 transaction under the same admitted window.

3. **Four candidate hot profiles at 32 MiB.** Reuse the accepted
   `build/checkpoint-i-current-cost-attribution/native-comparison-v1/window_probe.py`
   and four existing fork32 launch bodies. Keep their loops/repeats, ordinary
   GC enabled, no initial extra collection, setup/warmup snapshots and explicit
   `gc.set_threshold(32*1024*1024)`. Retain the FIFO/ACK enabled hot interval,
   `cpu-clock:u` 199 Hz/frame-pointer call chains, CLOCK_MONOTONIC bounds/PID,
   cycles:u/instructions:u and GNUtime whole-process peak RSS. No 64 MiB repeat
   or new upstream profile is needed. Four targets should take roughly 1–2min;
   keep existing per-command ceilings and stop on unexpected outcomes.

The uninstrumented elapsed comparisons remain separate from these profiles.
Report GC/roots snapshot deltas, collection counts, cumulative GC time and
available max-pause gauges with their scope. GNUtime RSS includes setup,
warmup and shutdown; sampled CPU proportions are not wall-time decomposition.
Hardware-count intervals include the acknowledged control boundaries. Compare
historical parent32/upstream samples as context, not a new contemporaneous
paired CPU experiment.

The root metric `unpinned` counts visits to detached-thread snapshots without
a pinned C-frame extent (`gc_trace.c:scan_other_thread`). Their captured registers
and stack receive conservative scanning. Counts can include repeated visits to
one snapshot; they are report-only under the existing qualification policy.
Here pinning describes the captured frame extent. Nursery object pinning has a
separate movement contract.

Offline attribution reuses the existing phase decoder: filter the actual PID
and hot monotonic timestamps; report exclusive mark, preparation, native-clear,
other teardown, storage return, residual and unknown/lost samples. New native
clear samples must be attributed by their collector ancestry; `gen_clear_frame`
also runs from public close/finalizer paths. `_PyGen_CanClearUnreachable` is
admission/preparation, `_PyGen_ClearUnreachable` is native commit. An observed
clear stack proves execution, not an exact admission rate. Inlining and finite
sampling mean absent samples do not prove no execution. No production counter,
callback or observer API is added to make this measurement easier.

## Small arithmetic diagnostic, outside the kernel score

Existing micro rows can supply useful guidance: `int_add` (2000+3000),
`int_add_small` (1+2), `float_add`, `attr_load`, `attr_store`. They isolate simple
int32/non-NaN float and monomorphic attribute cases, not overflow, NaNs,
polymorphism or whole-application arithmetic. The upstream cached-small-int
distinction is intentional. Loop/dispatch and store costs remain in raw data.
These existing arithmetic rows repeatedly assign invariant-input expressions;
they are not increment recurrences. Source review indicates runtime arithmetic
survives while local load/store dispatch can fold. Retain cold and warmed
standard-library disassembly for the actual two images and the three arithmetic
rows before interpreting instruction counts. These two untimed witnesses add no
measurement row or score; no standalone recurrence benchmark is introduced here.

Use existing `boil/bench/run.py --set micro --only
int_add,int_add_small,float_add,attr_load,attr_store --metric instr
--instr-runs 4 --max-load 4` for each matched image. The established 200/600
differenced calls and shared `empty_loop` skeleton cancel startup and report
instructions per inner iteration; retain raw vectors and raw/unsubtracted
medians alongside the existing adjusted values. No boxes/Py_STATS build.

The registry currently permits only instr/boxes for micro, so `--metric time`
does **not** produce micro timings. For inexpensive elapsed data, use the
unchanged generic `internal/driver.py time <micro-path> 200 5` for these rows
and `empty_loop`, one warmup plus five samples. Normalize nanoseconds by
200*1000 for an explicitly labeled observed ns per inner iteration. Keep
uncorrected elapsed medians/ranges; do not subtract two noisy elapsed medians
or silently enlarge loops if the calls are short. Existing driver/Context
commands already express this; no timing framework or registry edit is needed.
Include this small appendix; allow roughly 1–2min of gated process overhead.

Arithmetic diagnostics do not enter the 29 geomean, establish a 2× gate, or
justify benchmark-specific specialization. Check the actual executed operation
source/compiled path before interpreting very low counts as an ABI win. Serial
samples are correlated and have no formal confidence interval. Large common
deficits guide the next slice; near-parity rankings and small differences need
their spread stated rather than automatic reruns. Freeze final literal bindings
and scheduling once the landed native capsule exists; this plan reserves no
window and launches nothing.
