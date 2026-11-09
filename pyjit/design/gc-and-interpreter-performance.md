# Memory and interpreter performance: current direction

The collector decision is RC with cyclic collection. Tracing-GC work ended
under the user's timebox; no nursery or collector optimization is active.
The [old recommendation](../archive/legacy/design/gc-performance-before-wiki-reset-20261102.md)
and [decision](../issues/gc-parity-deadline/decision-20261101.md) remain evidence.

Complete universal PyValue restoration, then measure the register interpreter
with JIT explicitly disabled against matched interim RC and upstream controls.
Separate startup, lowering, warm execution, boxing/allocation and memory costs.
Use results to improve concrete IR, dispatch, allocation and boundary mechanisms.
There is no performance result for the restored runtime yet.

[Current experiments](../issues/README.md), [measurement](measurement.md) and
[testing/admission](validation.md) own the next steps and interpretation.
