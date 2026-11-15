# PyJIT goal

Build an optimized Python runtime and a shared method/tracing JIT on canonical
eight-byte PyValues and a register IR. Rebuild and port extensions. Preserve
Python behavior, explicit ownership and useful observation APIs.

The selected interpreter uses one owning frame stack, overlapping argument homes
and explicit recipients and continuations. Carry established value and layout facts
through the generator. Source attribution belongs to observation; execution and
exception recovery use semantic state. Source positions do not grant ownership
or initialization facts.

Current stage: **register-interpreter performance**, within stage 2 of the
[roadmap](ROADMAP.md). The [active plan](issues/interpreter-performance56/plan.md)
combines value transport, borrowing, call/return conventions and specialization
under one architecture. The active [IR-semantics workstream](issues/register-semantics59/README.md)
executes the adopted [round67 design](design/proposals/interpreter-parity67.md)
under [review67](issues/register-semantics59/review67.md). Independent source peer
and shared-interface readback pass. Sol-high workers implement two lanes from
selected57: complete native callout through rooted retirement/poll and a real
result consumer; caller-anchored read-only exact-method self with complete
promotion and owner-consumer closure. A qualifies but fails its balanced screen
(logging +21.1%, DeltaBlue +2.2%, spectral_norm +5.3%) and is rejected. B is
integrated independently and needs generator repairs before runtime qualification.
Root combines only independently qualified broad winners; selected57 is unchanged.

Round66 is closed: frozen B qualified but failed logging/generator protection;
its auxiliary union was not implemented. The logging diagnostic showed
instruction ratio .992588 and cycle ratio 1.085426, without establishing a mechanism.
Rejected branches and evidence remain inputs, not prerequisites. No demonstrated
combined parity budget exists; selected57 remains the runtime baseline.
Logical values, uses, edge results and continuations precede storage and
execution lowering. Adopted design67 extends the shared owner/effect contract
through native consumption and one inter-frame parameter loan. Sol workers
implement and cross-review bounded changes, and the coordinator owns integration, qualification and
measured selection. Repeat until the unchanged hot set reaches upstream parity
while retaining broad gains. A completed proposal does not complete this stage.
Selected57 remains the
runtime baseline. Keep rejected packets separate and preserve their evidence.

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
