# PyJIT goal

Build an optimized Python runtime and a shared method/tracing JIT on canonical
eight-byte PyValues and a register IR. Rebuild and port extensions. Preserve
Python behavior, explicit ownership and useful observation APIs.

## Current checkpoint

The frame/call round is closed at runtime `4d666e1b482`. Full diagnostics,
the complete benchmark set, current profiles and independent reviews are
published in the [report](issues/frame-call-model-20261109/closing-report.md).
Logging meets the round's parity gate; broad upstream parity remains open.
The active [object/call plan](issues/object-call-20261109/plan.md), reviewed by
[Astra](issues/object-call-20261109/astra-survey.md), selects canonical attribute
lookup and reusable keyword binding as the next bounded improvements. Publish
the current checkpoint to the golden branch before runtime edits. Preserve the
previous immutable tags and native controls. The coordinator owns integration
and measured outcomes.

Logging parity is the throughput target for this round. Use unchanged kernels,
GCC, JIT disabled, fixed affinity/hash seed and verified native/source identities.
Report broad results and repeat material regressions; do not hide them in a
geometric mean. Startup/import RSS and zero-box totals are not promotion gates.

GDB display tooling and exact CPython transcripts are separate from interpreter
semantics and throughput. Report tooling failures and unsupported frame-jump
capabilities explicitly. They do not justify an open-ended compatibility port.
Python does not guarantee prompt finalization or CPython's internal frame layout.

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
| [Plan](issues/object-call-20261109/plan.md) | Active object/call round and integration gates |
| [Status](STATUS.md) | Actual source, results and limitations |
| [Report](issues/frame-call-model-20261109/closing-report.md) | Full results, profiling and analysis |
| [Next five priorities](issues/frame-call-model-20261109/next-steps.md) | Evidence, risks and first experiments |
| [Architecture](design/runtime-architecture.md) | Runtime and intended compiler tiers |
| [Register IR](design/register-ir.md) | Executable encoding, effects and lowering |
| [Interpreter](design/interpreter.md) | Dispatch, execution and completion |
| [Frames and calls](design/call-and-frame-lifetimes.md) | Storage, binding, recipients and ownership |
| [Values](design/pyvalue.md) | Canonical representation and boundaries |
| [Handoff](HANDOFF.md) | Local paths, evidence and recovery |

Loom mirrors these pages; repository documentation owns execution state.
Historical mandates are preserved in
[the archived goal](archive/20261109/GOAL-before-round-closure.md).
