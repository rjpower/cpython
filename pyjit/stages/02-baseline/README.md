# Stage 2: register-interpreter baseline

The deliverable is a trustworthy register interpreter with canonical values,
explicit ownership and useful observation APIs. Measure unchanged workloads
with the JIT disabled against matched immutable controls. Assess throughput,
Python behavior and tooling separately; exact CPython display or destruction
timing is not a performance gate.

The [active plan](../../issues/interpreter-performance56/plan.md) combines
value transport, borrowing, calls/returns and specialization. Astra's broad
review precedes implementation. The coordinator integrates reviewed increments
and reports their native work and performance. [Status](../../STATUS.md) owns
selection and qualification; selected54 remains golden. The
[model55 report](../../issues/call-model55/report.md) retains previous results
and pending source components. Native method and tracing JIT tiers belong to
later stages.

The durable contracts are [register IR](../../design/register-ir.md),
[interpreter](../../design/interpreter.md), and
[calls and frames](../../design/call-and-frame-lifetimes.md). See also
[measurement](../../design/measurement.md), the
[benchmark registry](../../bench/README.md), and
[validation](../../design/validation.md).
