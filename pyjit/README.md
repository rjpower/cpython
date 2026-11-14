# PyJIT

PyJIT rebuilds CPython around canonical eight-byte PyValues, a register
interpreter, reference counting with cyclic collection, and a shared method
and tracing JIT. Extensions are rebuilt and ported.

Start with the [goal](GOAL.md), [active plan](issues/interpreter-performance56/plan.md)
and [status](STATUS.md). The register interpreter and universal PyValues are
implemented; native method and tracing JIT tiers remain unfinished. The active
stage unifies value transport, borrowing, calls and specialization. The [combined design](design/proposals/interpreter-performance56.md) and
[critical audit](issues/interpreter-performance56/review.md) define the next
increments; the coordinator integrates reviewed and measured changes. Selected56 is golden; its [report](issues/interpreter-performance56/report.md)
and [benchmark tables](issues/interpreter-performance56/benchmark-tables.md)
record gains, accepted regressions and remaining limits. The
[model55 report](issues/call-model55/report.md) retains the preceding experiments
and pending source components. Proposed contracts remain separate from the
as-built designs below.

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
