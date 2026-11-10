# PyJIT goal

Build an optimized Python runtime and a shared method/tracing JIT on canonical
eight-byte PyValues and a register IR. Rebuild and port extensions. Preserve
Python behavior, explicit ownership and useful observation APIs.

## Current checkpoint

Frozen runtime39 `4d666e1b482` meets logging parity; broad upstream parity
remains open. The [object/call checkpoint](issues/object-call-20261109/checkpoint46-report.md)
is reviewed at **b8e7e1bee66**. Class lookup and keyword binding improve markedly,
while the full29 mean is 1.18120/upstream and material golden-relative regressions
remain. Promotion is held. [Status](STATUS.md) records full diagnostic limits,
API/fixture repairs, current counters and actual source identities.

Keep golden39, combined snapshot `cb906f9bc52`, and both generations of immutable
controls unchanged. The [next plan](issues/object-call-20261109/plan.md) targets
confirmed CPI regressions, repeated immutable keyword checks, direct owning
coroutine construction, weighted lookup sites and authoritative IR effects.
The [entry-scope calibration](issues/object-call-20261109/entry-scope-calibration.md)
keeps that larger experiment bounded. The coordinator owns merged validation,
measurement and promotion.

The current target is lower object/call execution cost and a smaller broad
upstream gap, while retaining logging parity. Use unchanged kernels,
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
| [Report](issues/object-call-20261109/checkpoint46-report.md) | Current results, profiling and analysis |
| [Next priorities](issues/object-call-20261109/plan.md#next-work-in-evidence-order) | Evidence, risks and bounded experiments |
| [Architecture](design/runtime-architecture.md) | Runtime and intended compiler tiers |
| [Register IR](design/register-ir.md) | Executable encoding, effects and lowering |
| [Interpreter](design/interpreter.md) | Dispatch, execution and completion |
| [Frames and calls](design/call-and-frame-lifetimes.md) | Storage, binding, recipients and ownership |
| [Values](design/pyvalue.md) | Canonical representation and boundaries |
| [Handoff](HANDOFF.md) | Local paths, evidence and recovery |

Loom mirrors these pages; repository documentation owns execution state.
Historical mandates are preserved in
[the archived goal](archive/20261109/GOAL-before-round-closure.md).
