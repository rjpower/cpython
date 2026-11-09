# Stage 2: register-interpreter baseline

The universal-value baseline is qualified and frozen as the
[golden checkpoint](../../issues/golden-20261108/README.md). It is an immutable
control, not a claim of broad upstream performance parity.

The [frame/call round](../../issues/frame-call-model-20261109/README.md)
optimizes storage, binding, ownership and semantic completion. Its frozen
candidate, full qualification, logging target and complete benchmark matrix
are recorded in [status](../../STATUS.md) and the
[closing report](../../issues/frame-call-model-20261109/closing-report.md).
The [ranked next work](../../issues/frame-call-model-20261109/next-steps.md)
replaces earlier experiment orderings.

The deliverable is one trustworthy register interpreter with canonical values,
measured with the JIT disabled against matched immutable controls. Retain raw
workload vectors, compiler/build identities and native conditions. Separate
measured throughput, runtime correctness, supported observation APIs and developer
tooling. Exact CPython display or release timing is not an interpreter gate.

The native method/tracing tiers remain a later stage. Existing owning bridges
are incremental extension ports; a boxed-heap interim image cannot replace the
canonical-value baseline.

References: [IR](../../design/register-ir.md),
[interpreter](../../design/interpreter.md), [frames/calls](../../design/call-and-frame-lifetimes.md),
[measurement](../../design/measurement.md), [benchmark registry](../../bench/README.md),
[validation](../../design/validation.md).
