# PyJIT goal

Build an optimized Python runtime and a shared method/tracing JIT on canonical
eight-byte PyValues and a register IR. Rebuild and port extensions. Preserve
Python behavior, explicit ownership and useful observation APIs.

## Current checkpoint

The round-50 checkpoint preserves canonical method owners through
guarded lookup and publication, and passes explicit semantic recovery from an
observer to the unwinder. Execution does not depend on source coordinates.
The adjacent method-to-frame experiment was implemented and tested, then
rejected because it slowed DeltaBlue relative to the simpler method change.
The [report](issues/deltablue-call-model-20261110/report50.md) distinguishes the
selected checkpoint from rejected experiments; [status](STATUS.md) owns final
qualification and performance admission.

The target remains unchanged DeltaBlue performance toward upstream parity,
while retaining logging parity and useful broad progress. Preserve one frame
stack, overlapping argument homes, explicit owners and ordinary activation and
completion. Carry established value/layout facts through the generator rather
than rediscovering them at each boundary. Source attribution belongs to the
observation layer; control flow and exception recovery use semantic state.
Compiler line restrictions need an explicit observer reconstruction contract
before removal. See the [work plan](issues/deltablue-call-model-20261110/plan.md).

Astra performs broad grouped reviews; Sol handles the inner loop and independent
actual-code review. Luna/Terra handle bounded mechanical work and documentation.
The coordinator integrates and measures. Use one GCC configuration, unchanged
kernels, fixed affinity/hash and JIT disabled. Startup RSS, zero-box counts,
CPI/layout perfection and exact CPython destruction/display are not gates.

Historical golden39 and upstream stay immutable. Universal PyValues are
integrated; native method/tracing JIT tiers remain unfinished. Broad upstream
parity remains open. Hard extension source/binary compatibility is not required;
Python value/control behavior and explicit ownership are required.

## Standing decisions

- One canonical value format across frames, containers, attributes and rebuilt
  extensions. Owning bridges are incremental ports, not a second representation.
- Reference counting with cyclic collection is selected; collector redesign is
  closed. Frame storage, calls, results, unwind and observation share one model.
- Rebuild source and binary interfaces as needed. Hard extension compatibility
  is not an admission gate. Preserve Python value/control protocols and owners.
- Use one GCC configuration for comparisons. Golden remains immutable; source,
  benchmarks and controls stay fixed during measurements.
- Workers propose, peer-review, implement and report. The coordinator reviews,
  tests, integrates and measures. Full suites belong at coherent checkpoints.
  Follow [process](PROCESS.md) and [validation](design/validation.md).

## Navigation

| Read | Purpose |
| --- | --- |
| [Plan](issues/deltablue-call-model-20261110/plan.md) | Active ordinary-call round and integration gates |
| [Status](STATUS.md) | Actual source, results and limitations |
| [Report](issues/deltablue-call-model-20261110/report50.md) | Current results, profiling and analysis |
| [Next priorities](issues/deltablue-call-model-20261110/plan.md) | Evidence, risks and bounded experiments |
| [Architecture](design/runtime-architecture.md) | Runtime and intended compiler tiers |
| [Register IR](design/register-ir.md) | Executable encoding, effects and lowering |
| [Interpreter](design/interpreter.md) | Dispatch, execution and completion |
| [Frames and calls](design/call-and-frame-lifetimes.md) | Storage, binding, recipients and ownership |
| [Values](design/pyvalue.md) | Canonical representation and boundaries |
| [Handoff](HANDOFF.md) | Local paths, evidence and recovery |

Loom mirrors these pages; repository documentation owns execution state.
Historical mandates are preserved in
[the archived goal](archive/20261109/GOAL-before-round-closure.md).
