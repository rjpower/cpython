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
advances the [round68 proposal](design/proposals/interpreter-parity68.md) under
[review68](issues/register-semantics59/review68.md). Independent architecture
and exact source-contract approval pass for retained real generator-delegation
prefixes. Sol implements the inert foundation, followed by complete lifecycle
and reader closure before activation. Keep owning frames and existing execution
semantics, resume the retained leaf directly, and update only changed suffixes.
Charge storage and ordinary-generator costs without adding checks to ordinary
function calls. This unit targets generators; no comprehension, DeltaBlue or
coroutine parity budget is claimed. Root selects only independently qualified
broad winners. Both round67 runtimes are rejected; selected57 is unchanged.

Round66 is closed: frozen B qualified but failed logging/generator protection;
its auxiliary union was not implemented. The logging diagnostic showed
instruction ratio .992588 and cycle ratio 1.085426, without establishing a mechanism.
Rejected branches and evidence remain inputs, not prerequisites. No demonstrated
combined parity budget exists; selected57 remains the runtime baseline.
Logical values, uses, edge results and continuations precede storage and
execution lowering. Sol workers implement and cross-review bounded changes
after source-contract approval, and the coordinator owns integration, qualification and
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
