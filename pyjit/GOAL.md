# PyJIT goal

Build an optimized Python runtime and a shared method/tracing JIT on canonical
eight-byte PyValues and a register IR. Rebuild and port extensions. Preserve
Python behavior, explicit ownership and useful observation APIs.

## Current checkpoint

Close the frame/call-model round at runtime `4d666e1b482`: finish release/debug
qualification, measure the complete registered benchmark set against immutable
golden and matched upstream, publish independent architectural reviews and a
ranked five-item follow-up. The coordinator owns integration and the report.
Current measurements and limitations belong in [status](STATUS.md), not here.

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
| [Plan](issues/frame-call-model-20261109/plan.md) | Round closure and ranked next work |
| [Status](STATUS.md) | Actual source, results and limitations |
| [Architecture](design/runtime-architecture.md) | Runtime and intended compiler tiers |
| [Register IR](design/register-ir.md) | Executable encoding, effects and lowering |
| [Interpreter](design/interpreter.md) | Dispatch, execution and completion |
| [Frames and calls](design/call-and-frame-lifetimes.md) | Storage, binding, recipients and ownership |
| [Values](design/pyvalue.md) | Canonical representation and boundaries |
| [Handoff](HANDOFF.md) | Local paths, evidence and recovery |

Loom mirrors these pages; repository documentation owns execution state.
Historical mandates are preserved in
[the archived goal](archive/20261109/GOAL-before-round-closure.md).
