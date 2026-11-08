# Stage 2: qualified interpreter baseline

State: qualified baseline and selected optimization batch complete at
`0267d428808`. The batch is kept with an accepted `spectral_norm` regression;
the broader screen remains 1.2412× upstream, so parity is open. See the
[checkpoint report](../../bench/results/interpreter-parity-20261108.md) and
[interpreter parity plan](../../issues/interpreter-parity-20261108/plan.md).

Deliver a trustworthy starting point for optimization: the register interpreter
with universal values, measured with JIT explicitly off. The qualified older
interim image has boxed heap fields and cannot stand in for this result.

Release and debug each passed 54,394 tests with zero failures at the functional
checkpoint. The [checkpoint report](../../bench/results/interpreter-parity-20261108.md)
records the JIT-off comparison and evidence limits.

Measure against matched interim RC and upstream controls using the unchanged
29-workload roster. Retain raw workload vectors, build/toolchain identities and
JIT-off evidence. Separate startup, lowering and warm execution; report time
and memory. No arbitrary text-size veto substitutes for performance evidence.

Baseline exit evidence is complete. The selected batch is measured and kept;
parity remains open. Borrowed-frame work is limited to diagnostics and a
representation decision, with implementation gates unresolved.

References: [gates](../../design/validation.md), [measurement](../../design/measurement.md),
[integration](../../issues/pyvalue-rc-restoration/integration/status.md),
[benchmark registry](../../bench/README.md), [current status](../../STATUS.md).
