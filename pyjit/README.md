# PyJIT

PyJIT rebuilds CPython around canonical eight-byte PyValues, a register
interpreter, reference counting with cyclic collection, and a shared method and
tracing JIT. Extensions are rebuilt and ported.

## Start here

Read the [goal](GOAL.md), [active plan](issues/interpreter-performance56/plan.md),
[runtime architecture](design/runtime-architecture.md), and [status](STATUS.md),
in that order. The [roadmap](ROADMAP.md) gives stage outcomes; [process](PROCESS.md)
and [handoff](HANDOFF.md) explain development and runtime coordination.

**Current work (2026-11-22):** contract87, the interpreter's last wave and
the first phase of [tiers87](design/proposals/tiers87.md); then a
Sparkplug-style baseline method JIT, then a tracing JIT, all generated from
one operation contract.

**Previous (2026-11-22):** [tight86](issues/register-semantics59/tight86.md)
landed hot85's deferred items in eight units as clean, tightly defined code.
Its integration (`3cac420ddbf`) runs 0.970 of golden85's time and 0.877 of
upstream's and was selected as golden86.

**Earlier (2026-11-21):** [parity84](issues/register-semantics59/parity84.md)
implemented profile83's clean changes in six units. Its integration IP2
(`4be2eb5d31f`) runs 0.961 of golden82's time and 0.953 of upstream's and
was selected as golden84.

**Earlier (2026-11-20):** [gap82](issues/register-semantics59/gap82.md),
H5's remaining gap to upstream. The
[handlers81](design/proposals/handlers81.md) rollout and
[operand-encoded calls](design/proposals/operand-calls80.md) landed as H0–H5
on `pyjit/handlers81` (`d6d1ed5ac93`,
[record](issues/register-semantics59/handlers81.md)). Gap82 fanned analysis
and implementation out to agents from there. It is integrated as
`pyjit/gap82` at `5194e957c5d`, with a timed all-29 geomean of 0.992 of
upstream's time ([results](issues/register-semantics59/gap82.md#results)). The
[active plan](issues/interpreter-performance56/plan.md#previous-wave-gap82-the-gap-to-upstream-after-h5)
lists the phases and [handoff](HANDOFF.md) the worktrees.

The selected runtime is golden85 at `2a9ea8c4f69`, branch `pyjit/round85-golden`,
selected on 2026-11-22. Golden84 (`4be2eb5d31f`, `pyjit/round84-golden`),
golden82 (`5194e957c5d`, `pyjit/round82-golden`) and
golden73 (`44f066f5fd5`, `pyjit/round73-golden`) remain comparison controls.
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
