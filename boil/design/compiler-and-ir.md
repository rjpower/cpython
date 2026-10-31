# Compiler and register IR

The compiler emits the register representation directly. The main source paths
are `Python/compile.c`, `codegen.c`, `flowgraph.c`, `regcfg.c`,
`regcfg_passes.c`, `assemble.c` and `ir.c`.

## Value flow

- The stack CFG supplies Python control flow, exception edges and source locations.
  Register conversion assigns physical frame slots and explicit operands.
- Eligible local loads and result stores fold into consumer operands and producer
  destinations. Copy propagation and compaction operate on the register graph.
- Folding must respect operations that can execute Python or rebind state.
  A borrowed local read is not a permanent value snapshot across a reentrant call.
- Hidden control flow, exceptional results, call windows and initialization have
  the same semantic force as explicit inputs and outputs. Unsupported analysis
  must retain a safe path rather than infer purity or liveness.
- Assembly emits code, feedback and exception/location information together.
  Changes to executing formats or handler identity must keep generators, readers
  and cache-version policy coherent.

## Executing state

Immutable code words and the mutable executing copy have distinct roles.
Specialization consumes observations under the current owner/type/version and
value guards. Instrumentation and monitoring can change which handler executes;
consumers must use the generated domain rather than assumed numeric ranges.

Frame slots carry live values, temporary results and call windows. Ordinary
thread-owned caller frames can share a callee argument window. Generator-owned
callers use their own frame storage. Neither path removes the need to publish
initialized roots, preserve exception state or keep the current instruction
position meaningful at observation points.

## Shared improvements

Operation names should express useful shared contracts, not proliferate
benchmark-specific typed variants. Receiver admission, stored representation,
result facts, call adaptation and observer behavior are separate dimensions.
A producer promise is useful only when consumers and fallbacks agree on it.

Source and emitted-code findings belong to the
[interpreter issue](../issues/interpreter-shared-costs/status.md); active bounded
investigation belongs to the [work index](../issues/README.md). No speedup or
new optimization follows from this reference alone.
