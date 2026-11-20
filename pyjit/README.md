# PyJIT

PyJIT rebuilds CPython around canonical eight-byte PyValues, a register
interpreter, reference counting with cyclic collection, and a shared method and
tracing JIT. Extensions are rebuilt and ported.

## Start here

Read the [goal](GOAL.md), [active plan](issues/interpreter-performance56/plan.md),
[runtime architecture](design/runtime-architecture.md), and [status](STATUS.md),
in that order. The [roadmap](ROADMAP.md) gives stage outcomes; [process](PROCESS.md)
and [handoff](HANDOFF.md) explain development and runtime coordination.

**Current work (2026-11-20):** the
[handlers81](design/proposals/handlers81.md) rollout, then
[operand-encoded calls](design/proposals/operand-calls80.md), on
`pyjit/handlers81` from checkpoint78. The
[active plan](issues/interpreter-performance56/plan.md#current-rollout-handlers81-then-calls80)
lists the steps and [handoff](HANDOFF.md) the worktrees.

The selected runtime is golden73 at `44f066f5fd5`, branch `pyjit/round73-golden`.
Both independently implemented
[execution-state72](design/proposals/execution-state72.md) units are closed and
rejected: cold ownership maps demonstrated no packing benefit, and carried-value
execution added more ordinary transfer work than it removed on the protected
benchmark set. Sol 6.1 workers implemented and peer-reviewed; root integrated
and validated. The [trial report](issues/register-semantics59/report.md) gives
exact results, limits and the next design constraints. Follow the goal → plan →
architecture/status chain above; rejected trials are evidence, not prerequisites.

[Execution-boundary73](design/proposals/execution-boundary73.md) is implemented
and measured, including the RETURN isolation correction. Coroutines improve
8.70% and generators 6.73%; the user accepted logging's 13.73% regression
and selected this checkpoint on 2026-11-16.
The complete fast paths and retained specializations are preserved in the
golden runtime. The trial report owns this checkpoint; the active plan identifies
the remaining call/return cycle-cost problem.
The [bounded diagnosis](issues/register-semantics59/execution73-diagnosis.md)
records confirmed specialization losses in rejected72 and the distinct logging
regression. The active plan records review, implementation and integration roles.

## Architecture and evidence

| Read | Purpose |
| --- | --- |
| [Runtime architecture](design/runtime-architecture.md) | Runtime structure and planned JIT tiers |
| [Register IR](design/register-ir.md), [interpreter](design/interpreter.md) | Encoding, execution and observation |
| [Calls and frames](design/call-and-frame-lifetimes.md), [activation transfers](design/activation-transfers.md) | Ownership, storage and completion |
| [PyValue](design/pyvalue.md), [value API](design/value-api.md), [native boundary](design/native-boundary.md) | Values and rebuilt extensions |
| [Selected57 report](issues/comprehensions57/report.md), [benchmark tables](issues/comprehensions57/benchmark-tables.md) | Selected change, comparisons and qualification |
| [Trial report](issues/register-semantics59/report.md) | Golden73 selection, prior trials, exact results and evidence limits |
| [Selected56 report](issues/interpreter-performance56/report.md), [model55 ledger](issues/call-model55/implementation.md) | Prior selected work and unfinished components |
| [Documentation rules](DOCUMENTATION.md) | Documentation ownership and placement |

The [lifetime policy](design/lifetimes-and-language-semantics.md) governs
Python behavior and ownership without requiring exact CPython destruction timing.
