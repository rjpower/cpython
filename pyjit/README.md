# PyJIT

PyJIT rebuilds CPython around canonical eight-byte PyValues, a register
interpreter, reference counting with cyclic collection, and a shared method
and tracing JIT. Extensions are rebuilt and ported.

Start with the [goal](GOAL.md), [active plan](issues/interpreter-performance56/plan.md)
and [status](STATUS.md). The register interpreter and universal PyValues are
implemented; native method and tracing JIT tiers remain unfinished. The active
stage unifies value transport, borrowing, calls and specialization. The
[IR-semantics workstream](issues/register-semantics59/README.md) follows the
adopted [round67 design](design/proposals/interpreter-parity67.md) and
[review](issues/register-semantics59/review67.md). Two independent source lanes
start from selected57: a complete native callout/result-consumer boundary, and
one caller-anchored exact-method self binding. Shared ownership/effect interfaces
have independent peer GO. Source implementation is active; neither lane has a
runtime qualification or performance result. Selected57 remains unchanged.

Round66's frozen continuation qualified but lost its logging/generator gates.
Its finite logging diagnostic measured fewer instructions and more cycles;
no cache/frontend cause was established. The [trial report](issues/register-semantics59/report.md)
owns that closed result and the active source state.
The bounded compact execution domain failed its native net-work gate. Complete deferred
await passes lifecycle/native gates and cuts coroutine time 42.9%, but loses
logging 27.7% and generators 5.2%; it is rejected as a standalone executable.
Round65 closed without a selected runtime: A loses its hot8 screen; B fails
native review after one correction and was not timed in that round. Selected57 remains
unchanged.
The [trial report](issues/register-semantics59/report.md) records rejected
round59–66 candidates; selected57 remains unchanged. The preceding
[parity design](design/proposals/interpreter-parity58.md),
[research ledger](issues/interpreter-parity58/research.md) and
[workstream](issues/interpreter-parity58/README.md) retain the earlier diagnosis;
the coordinator integrates reviewed and measured changes. Proposed contracts
remain separate from selected implementation. The
[selected57 report](issues/comprehensions57/report.md) and
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
