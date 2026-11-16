# PyJIT

PyJIT rebuilds CPython around canonical eight-byte PyValues, a register
interpreter, reference counting with cyclic collection, and a shared method and
tracing JIT. Extensions are rebuilt and ported.

## Start here

Read the [goal](GOAL.md), [active plan](issues/interpreter-performance56/plan.md),
[runtime architecture](design/runtime-architecture.md), and [status](STATUS.md),
in that order. The [roadmap](ROADMAP.md) gives stage outcomes; [process](PROCESS.md)
and [handoff](HANDOFF.md) explain development and runtime coordination.

The selected runtime remains selected57. Independent source review stopped the
[round70 design](design/proposals/interpreter-parity70.md) before implementation:
canonical CALL results added consumer work without a credible net saving.
The adopted [round71 design](design/proposals/interpreter-parity71.md) orders
canonical-value fact transport, then generated hot/slow execution separation.
Exact source contracts are in independent cross-review before implementation.
The stopped round69 summary trial removed child execution on hits but lost its
early protected screen with callback qualification incomplete. The
[trial report](issues/register-semantics59/report.md) owns results and limits.

## Architecture and evidence

| Read | Purpose |
| --- | --- |
| [Runtime architecture](design/runtime-architecture.md) | Runtime structure and planned JIT tiers |
| [Register IR](design/register-ir.md), [interpreter](design/interpreter.md) | Encoding, execution and observation |
| [Calls and frames](design/call-and-frame-lifetimes.md), [activation transfers](design/activation-transfers.md) | Ownership, storage and completion |
| [PyValue](design/pyvalue.md), [value API](design/value-api.md), [native boundary](design/native-boundary.md) | Values and rebuilt extensions |
| [Selected57 report](issues/comprehensions57/report.md), [benchmark tables](issues/comprehensions57/benchmark-tables.md) | Selected change, comparisons and qualification |
| [Trial report](issues/register-semantics59/report.md) | Current rejected candidates, exact results and evidence limits |
| [Selected56 report](issues/interpreter-performance56/report.md), [model55 ledger](issues/call-model55/implementation.md) | Prior selected work and unfinished components |
| [Documentation rules](DOCUMENTATION.md) | Documentation ownership and placement |

The [lifetime policy](design/lifetimes-and-language-semantics.md) governs
Python behavior and ownership without requiring exact CPython destruction timing.
