# PyJIT

PyJIT rebuilds CPython around canonical eight-byte PyValues, a register
interpreter, reference counting with cyclic collection, and a shared method
and tracing JIT. Extensions are rebuilt and ported.

Start with the [goal](GOAL.md), [active plan](issues/call-model55/plan.md)
and [status](STATUS.md). The register interpreter and universal PyValues are
implemented; native method and tracing JIT tiers remain unfinished. Round54
selects a correctness checkpoint with explicit qualification limits and no
performance lift. The [checkpoint report](issues/frame-state54/report.md)
records the rejected experiments and selected cold constructor correction.
Round55 has a reviewed holistic call/frame/register design and migration plan.
The [target design](design/proposals/register-call-model55.md) and
[review resolution](issues/call-model55/review-resolution.md) are complete;
implementation has not started. Proposed contracts do not replace the as-built designs.

| Read | Purpose |
| --- | --- |
| [Roadmap](ROADMAP.md) | Stage outcomes and dependencies |
| [Runtime architecture](design/runtime-architecture.md) | Runtime structure and planned JIT tiers |
| [Register IR](design/register-ir.md), [interpreter](design/interpreter.md) | Encoding, execution and observation |
| [Calls and frames](design/call-and-frame-lifetimes.md), [activation transfers](design/activation-transfers.md) | Ownership, storage and completion |
| [PyValue](design/pyvalue.md), [value API](design/value-api.md), [native boundary](design/native-boundary.md) | Values and rebuilt extensions |
| [Process](PROCESS.md), [documentation](DOCUMENTATION.md) | Development and documentation rules |
| [Handoff](HANDOFF.md) | Execution paths, build state and recovery |

The [lifetime policy](design/lifetimes-and-language-semantics.md) governs
Python behavior and ownership without requiring exact CPython destruction timing.
