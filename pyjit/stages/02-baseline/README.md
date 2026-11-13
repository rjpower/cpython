# Stage 2: register-interpreter baseline

The deliverable is a trustworthy register interpreter with canonical values,
explicit ownership and useful observation APIs. Measure unchanged workloads
with the JIT disabled against matched immutable controls. Assess throughput,
Python behavior and tooling separately; exact CPython display or destruction
timing is not a performance gate.

The [current report](../../issues/frame-state54/report.md) records five reviewed,
implemented and measured register-state/frame-layout experiments, their revisions
and compositions. None earns performance admission. A separate public-constructor
correction is selected as a correctness checkpoint with explicit diagnostic
limits. Round54 is closed. The [active plan](../../issues/call-model55/plan.md)
and [proposed call model](../../design/proposals/register-call-model55.md)
define a reviewed whole-lifecycle call/frame/register architecture, including
a complete bitmap comparison. Research and independent post-review are complete;
implementation has not started. [Status](../../STATUS.md) records actual
selection, qualification and performance limits. Native method and tracing JIT
tiers belong to later stages; owning bridges support incremental extension ports.

The durable contracts are [register IR](../../design/register-ir.md),
[interpreter](../../design/interpreter.md), and
[calls and frames](../../design/call-and-frame-lifetimes.md). See also
[measurement](../../design/measurement.md), the
[benchmark registry](../../bench/README.md), and
[validation](../../design/validation.md).
