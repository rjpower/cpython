# Stage 2: register-interpreter baseline

The deliverable is a trustworthy register interpreter with canonical values,
explicit ownership and useful observation APIs. Measure unchanged workloads
with the JIT disabled against matched immutable controls. Assess throughput,
Python behavior and tooling separately; exact CPython display or destruction
timing is not a performance gate.

The [current packet](../../issues/interpreter-round51/plan.md) evaluates bounded
interpreter improvements and their composition. [Status](../../STATUS.md) records
the selected runtime, qualification, performance and limits. Native method and
tracing JIT tiers belong to later stages; owning bridges support incremental
extension ports.

The durable contracts are [register IR](../../design/register-ir.md),
[interpreter](../../design/interpreter.md), and
[calls and frames](../../design/call-and-frame-lifetimes.md). See also
[measurement](../../design/measurement.md), the
[benchmark registry](../../bench/README.md), and
[validation](../../design/validation.md).
