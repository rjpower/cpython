# PyJIT goal

Build an optimized Python runtime and a shared method/tracing JIT on canonical
eight-byte PyValues and a register IR. Rebuild and port extensions. Preserve
Python behavior, explicit ownership and useful observation APIs.

## Current checkpoint

The selected DeltaBlue checkpoint is runtime48: canonical class values and owned
classmethod forwarding. The complete leaf experiment was tested and rejected
on performance. Keep useful incremental progress and report its tradeoffs.
The [report](issues/deltablue-call-model-20261110/checkpoint-report.md) and
[status](STATUS.md) own results and qualification limits.

The next target remains DeltaBlue ordinary call setup/execution/teardown, while
retaining logging parity. The reviewed [C2 proposal](issues/deltablue-call-model-20261110/C2-proposal.md)
selects eligible CALLs during specialization, resolves immutable feedback once,
consumes checked-prefix facts and simplifies common ownership completion.
The [completion review](issues/deltablue-call-model-20261110/C2-completion-review.md)
asks how to publish a coherent caller before callback-capable retirement; a
nonfinal reference-count ledger is an implementation choice, not Python semantics.
Its performance is unproved. Do not expand syntax or add a miniature evaluator
to justify activation omission. See the [work plan](issues/deltablue-call-model-20261110/plan.md).

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
| [Report](issues/deltablue-call-model-20261110/checkpoint-report.md) | Current results, profiling and analysis |
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
