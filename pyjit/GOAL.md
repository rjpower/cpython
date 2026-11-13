# PyJIT goal

Build an optimized Python runtime and a shared method/tracing JIT on canonical
eight-byte PyValues and a register IR. Rebuild and port extensions. Preserve
Python behavior, explicit ownership and useful observation APIs.

The interpreter uses one owning frame stack, overlapping argument homes and
explicit recipients and continuations. Carry established value and layout facts
through the generator. Source attribution belongs to observation; execution and
exception recovery use semantic state. Compiler line restrictions need an
observer reconstruction contract before removal.

Improve unchanged-workload performance toward upstream parity while retaining
logging parity and examining broad regressions. The
[active plan](issues/call-model55/plan.md) defines the reviewed call/frame/register
architecture and migration sequence, including occupancy maps, storage, handoff
and observation. Research and independent post-review are complete;
implementation has not started. The
[reviewed proposal](design/proposals/register-call-model55.md) accounts for
complete lifecycles and recurring costs; neither a bitmap nor native JIT is
an automatic answer. Round54 selects only a cold constructor correctness
checkpoint with qualification limits, and rejects all performance variants.
The [current report](issues/frame-state54/report.md) records that closure;
[status](STATUS.md) records results and limits.

## Standing decisions

- One canonical value format spans frames, containers, attributes and rebuilt
  extensions. Owning bridges support incremental ports.
- Reference counting with cyclic collection is selected. Frame storage, calls,
  results, unwind and observation share one ownership model.
- Rebuild source and binary interfaces as needed. Hard extension compatibility
  is not an admission gate; Python value/control behavior and owners are.
- Compare fixed workloads and immutable controls using one GCC configuration,
  fixed affinity and hash, and JIT disabled. Startup RSS, zero-box counts and
  exact CPython destruction or display are not throughput gates.

The [roadmap](ROADMAP.md) describes stage outcomes; the
[architecture](design/runtime-architecture.md) and linked designs define
the runtime contracts. Follow [process](PROCESS.md) and
[validation](design/validation.md) for changes.
