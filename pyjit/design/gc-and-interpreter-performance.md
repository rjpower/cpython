# Memory and interpreter performance: current direction

The collector decision is RC with cyclic collection. Tracing-GC work ended
under the user's timebox; no nursery or collector optimization is active.
The [old recommendation](../archive/legacy/design/gc-performance-before-wiki-reset-20261102.md)
and [decision](../issues/gc-parity-deadline/decision-20261101.md) remain evidence.

Canonical PyValue integration under RC is complete, including rebuilt bundled
extensions. Current work removes proved transport, activation, dispatch and
boundary costs within that runtime; it is not another value-representation
rollout. Object bridges remain where an object-form API requires them.

Measure the register interpreter with JIT explicitly disabled against matched
selected-runtime and upstream controls.
Separate startup, lowering, warm execution, boxing/allocation and memory costs.
Use results to improve concrete IR, dispatch, allocation and boundary mechanisms.
Measurements already exist. The [selected54 report](../issues/frame-state54/report.md)
and [integrated56 report](../issues/interpreter-performance56/report.md) retain
their controls and limits; integrated56 experiments are not selected replacements.

[Status](../STATUS.md), [current experiments](../issues/README.md),
[measurement](measurement.md) and
[testing/admission](validation.md) own the next steps and interpretation.
