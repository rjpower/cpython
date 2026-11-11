# Project status

Round 51 is measuring five peer-reviewed candidates in isolated runtime
subtrees. Release builds and focused checks pass; final debug and merged
qualification remain pending. The hot set covers DeltaBlue, Richards,
generators, coroutines, comprehensions and spectral norm, with silent logging
as a mandatory guard.
The [active plan](issues/interpreter-round51/plan.md) records selection and
integration decisions. No round-51 performance changes have landed yet.
Known GDB pretty-printer diagnostics are quarantined in the qualification
runner; a focused check confirms the other four GDB groups still pass. Python
behavior and observer checks remain enabled.

The selected round-50 checkpoint is runtime `bf810ec1a33`: canonical guarded
method owners plus explicit observer-error recovery. Release/debug/stats builds,
focused checks and the API gate pass. Both full diagnostics are complete, with
the established failure groups and counts. [Report](issues/deltablue-call-model-20261110/report50.md)
→ [all benchmark tables](issues/deltablue-call-model-20261110/benchmark-tables50.md)
→ [active plan](issues/interpreter-round51/plan.md).
[HANDOFF](HANDOFF.md) owns exact source, build and job state.

## Performance

Six balanced pairs confirm targets; all 29 unchanged kernels have two pairs
against each control. GCC 15.2, CPU 15, hash 0, JIT disabled. Lower elapsed
candidate/control ratio is better.

| Case | / golden39 | / upstream | / preceding checkpoint48 |
| --- | ---: | ---: | ---: |
| DeltaBlue, six pairs | 0.685934 | 1.123299 | 0.927374 |
| Logging silent, six pairs | 0.895185 | 0.804557 | 1.043134 |
| All 29 kernels, geometric mean | 0.996588 | 1.177045 | 0.997971 |

DeltaBlue takes 31.4% less time than golden39 and 7.3% less than checkpoint48,
but remains 12.3% slower than upstream. Logging parity holds in every upstream
confirmation pair. Broad upstream parity remains open: the suite mean is
essentially flat against golden39, and 17.7% slower than upstream.

The mean hides material losses. Spectral norm is 37.1% slower than golden39
(six pairs) and 10.6% slower than checkpoint48 (screen); coroutines are 11.0%
slower than golden39 (six pairs). Logging is 4.3% slower than checkpoint48,
despite its upstream headroom. Direct selected/A also reports a 7.4% logging
cost. The report separates inherited and incremental gaps; do not describe
this as an across-the-board speedup.

Matched PMU diagnostics show DeltaBlue at 1.35310 native instructions and
1.12964 user cycles per upstream iteration; logging at 0.95748 and 0.79973.
The selected/A logging comparison executes effectively the same instruction
count but uses more cycles. Its cause remains unproved; source-map decoding
or added dynamic instructions cannot explain that measured difference.

## Changes and design

Guarded method capture retains canonical function and receiver ownership facts
through publication for with-values, no-dict and lazy-dict layouts. Native
numeric recanonicalization is removed while lookup/mutation and owner guards
remain. The ordinary exact CALL implementation is unchanged.

Source coordinates do not control ordinary dispatch or call admission. A failed
observer publishes its explicit semantic exception recipe into activation-local
state; the unwinder consumes it without reading a source map. Attribution and
the frame's display ordinal remain observation data. Compiler line predicates
and serialized observer metadata still need a separate reconstruction design.

The adjacent method-to-frame experiment and its direct-overlap follow-up were
implemented, reviewed and tested, then rejected: the final follow-up loses 2.8%
on DeltaBlue and 14.0% on logging versus A. It is retained on an experimental
branch; no optional refinement gate or second evaluator remains selected.
Earlier leaf/C2 experiments remain deferred historical evidence.

## Qualification

Merged release/debug focused checks each pass 804 tests; stats smoke passes 150.
All three builds check 117 modules. The strict value API gate passes 306 units,
with zero failing findings or parse errors. Source/native, performance and final
diagnostic review approve this increment with the stated tradeoffs.

Full release/debug diagnostics each run 54,708 tests. Release has 16 GDB display
failures and nine debugger-jump errors; debug has 21 display failures, one trace-
format failure and nine debugger-jump errors. These are failed diagnostics with
the same module groups and counts as checkpoint48; no additional modules fail.
Quiet logs do not establish individual failing-case identity. Python behavior
and explicit ownership govern admission, separate from exact CPython display
and finalizer transcripts.

Universal PyValues are integrated throughout runtime and rebuilt extensions.
Native method/tracing JIT tiers remain unfinished. Golden39, upstream and all
historical comparison controls remain immutable. Current contracts:
[IR](design/register-ir.md), [interpreter](design/interpreter.md),
[frames/calls](design/call-and-frame-lifetimes.md), [values](design/pyvalue.md).
