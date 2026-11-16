# PyJIT goal

Build an optimized Python runtime and a shared method/tracing JIT on canonical
eight-byte PyValues and a register IR. Rebuild and port extensions. Preserve
Python behavior, explicit ownership and useful observation APIs.

The selected interpreter uses one owning frame stack, overlapping argument homes
and explicit recipients and continuations. Carry established value and layout facts
through the generator. Source attribution belongs to observation; execution and
exception recovery use semantic state. Source positions do not grant ownership
or initialization facts.

Latest checkpoint: [execution-boundary73](design/proposals/execution-boundary73.md)
is implemented, protocol-qualified and measured at `dee619e80ab`. Complete
generator/coroutine paths improve about 6%, but logging and raytrace regress,
so selected57 remains golden. Existing specialization coverage is preserved;
callback-free completed receiver destruction now avoids effect packaging.
The [report](issues/register-semantics59/report.md) separates these real gains
from the failed protection gate. Next: source-review isolation of generator
return handling from the shared ordinary return path, whose native body grew.
Counters show increased cycle cost rather than substantial extra instructions;
they do not prove the hardware cause. Retain the completed fast paths and
validate a changed candidate before promotion. Astra owns grouped architecture
review; Sol 6.1 owns source and cross-review; root owns integration and measurement.
Frame packing waits for demonstrated layout and store reductions.

Current stage: **register-interpreter performance**, within stage 2 of the
[roadmap](ROADMAP.md). The [active plan](issues/interpreter-performance56/plan.md)
combines value transport, borrowing, call/return conventions and specialization
under one architecture. The active [IR-semantics workstream](issues/register-semantics59/README.md)
has completed the [execution-state72](design/proposals/execution-state72.md)
attempt. Sol 6.1 workers implemented and cross-reviewed its compiler/validation
and execution/activation changes; root integrated and validated. Both layout
and carried-owner units lost their protected screens. The latter demonstrates
actual carried values, but increases common transfer work and retains existing
frame size/initialization. Neither is selected.

The objective remains logical values before physical allocation, a cheap result
channel, typed effect boundaries and exact ownership maps. Ordinary execution
chooses layout; exception cleanup consumes it. Before another broad rollout,
the adopted boundary73 contract requires a complete producer/effect/consumer net
saving and retained specialization coverage. No third72 lane is adopted.
Smaller source or native frames alone do not predict throughput. The selected
runtime retains its contracts until a measured winner lands.

Selected57 remains the runtime baseline. Round68 and earlier losing packets
also remain closed. Preserve their evidence without making them prerequisites
or claiming a combined parity budget. The
[trial report](issues/register-semantics59/report.md) owns those outcomes.

Logical values, uses, edge results and continuations precede storage and
execution lowering. Sol workers implement and cross-review bounded changes
after exact source-contract approval; the coordinator owns integration,
qualification and measured selection. Repeat until the unchanged hot set
reaches upstream parity while retaining broad gains. A completed proposal does
not complete this stage.

Remove repeated work from ordinary execution and improve unchanged workloads
toward upstream parity while preserving logging performance. Frame clearing is
one hypothesis, not an established explanation for the gap. PyValue integration
is complete; remaining work concerns transport, ownership and how established
facts survive lowering and native boundaries.

Start here, then read the active plan, [architecture](design/runtime-architecture.md)
and [status](STATUS.md). Status distinguishes selected, measured and experimental
source. Selected57 adds dictionary provider cleanup and lifetime repair;
golden56 and golden54 remain immutable comparison points.
The [active trial report](issues/register-semantics59/report.md) records each
current integration and performance decision. The
[selected57 report](issues/comprehensions57/report.md) records the rejected
comprehension optimizations, isolated dictionary repair and baseline upstream
gap. The [selected56 report](issues/interpreter-performance56/report.md) retains
its accepted broad regressions. The [model55 report](issues/call-model55/report.md)
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
