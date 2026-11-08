# Stage 2: qualified interpreter baseline

State: qualified baseline complete at runtime `0953757f4e7`. The next
optimization milestone is the active [interpreter parity experiment](../../issues/interpreter-parity-20261108/plan.md).

Deliver a trustworthy starting point for optimization: the register interpreter
with universal values, measured with JIT explicitly off. The qualified older
interim image has boxed heap fields and cannot stand in for this result.

The selected runtime has passed release and debug full-suite qualification.
The 2026-11-07 [architecture report](../../bench/results/interpreter-architecture-20261107.md)
records the matched JIT-off baseline and its evidence limits.

Measure against matched interim RC and upstream controls using the unchanged
29-workload roster. Retain raw workload vectors, build/toolchain identities and
JIT-off evidence. Separate startup, lowering and warm execution; report time
and memory. No arbitrary text-size veto substitutes for performance evidence.

Exit evidence is complete. Current parity work ranks source-backed changes and
tests accepted increments against the qualified baseline; parity remains to be
measured.

References: [gates](../../design/validation.md), [measurement](../../design/measurement.md),
[integration](../../issues/pyvalue-rc-restoration/integration/status.md),
[benchmark registry](../../bench/README.md), [current status](../../STATUS.md).
