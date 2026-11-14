# PyJIT

PyJIT rebuilds CPython around canonical eight-byte PyValues, a register
interpreter, reference counting with cyclic collection, and a shared method
and tracing JIT. Extensions are rebuilt and ported.

Start with the [goal](GOAL.md), [active plan](issues/interpreter-performance56/plan.md)
and [status](STATUS.md). The register interpreter and universal PyValues are
implemented; native method and tracing JIT tiers remain unfinished. The active
stage unifies value transport, borrowing, calls and specialization. The reviewed
[parity design](design/proposals/interpreter-parity58.md),
[research ledger](issues/interpreter-parity58/research.md) and
[workstream](issues/interpreter-parity58/README.md) define the next increments;
the coordinator integrates reviewed and measured changes. These are proposals,
not implemented round58 improvements. The
[current report](issues/comprehensions57/report.md) and
[benchmark tables](issues/comprehensions57/benchmark-tables.md) record the
comprehension experiments, their rejection and the isolated dictionary repair.
The [selected56 report](issues/interpreter-performance56/report.md) retains
the preceding performance landing and accepted regressions. The
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
