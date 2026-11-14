# PyJIT goal

Build an optimized Python runtime and a shared method/tracing JIT on canonical
eight-byte PyValues and a register IR. Rebuild and port extensions. Preserve
Python behavior, explicit ownership and useful observation APIs.

The interpreter uses one owning frame stack, overlapping argument homes and
explicit recipients and continuations. Carry established value and layout facts
through the generator. Source attribution belongs to observation; execution and
exception recovery use semantic state. Compiler line restrictions need an
observer reconstruction contract before removal.

Current stage: **register-interpreter performance**, within stage 2 of the
[roadmap](ROADMAP.md). The [active plan](issues/interpreter-performance56/plan.md)
combines value transport, borrowing, call/return conventions and specialization
under one architecture. Astra completed the [combined design](design/proposals/interpreter-performance56.md)
and [critical audit](issues/interpreter-performance56/review.md). Sol workers
peer the concrete increments and implement them; the coordinator owns
integration, qualification and selection.

Remove repeated work from ordinary execution and improve unchanged workloads
toward upstream parity while preserving logging performance. Frame clearing is
one hypothesis, not an established explanation for the gap. PyValue integration
is complete; remaining work concerns transport, ownership and how established
facts survive lowering and native boundaries.

Start here, then read the active plan, [architecture](design/runtime-architecture.md)
and [status](STATUS.md). Status distinguishes selected, measured and experimental
source. Selected56 is the incremental golden checkpoint; golden54 remains immutable.
The [current report](issues/interpreter-performance56/report.md) records accepted
broad regressions and the remaining upstream gap. The [model55 report](issues/call-model55/report.md)
retains its measured results and unresolved qualification; its pending ownership
and exception-transfer components are inputs to the combined plan, not an
independent requirement to finish every old experiment first.

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
