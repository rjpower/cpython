# PyJIT

PyJIT rebuilds CPython around canonical eight-byte PyValues, a register
interpreter, reference counting with cyclic collection, and a shared method and
tracing JIT. Extensions are rebuilt and ported.

## Start here

Read the [goal](GOAL.md), [active plan](issues/interpreter-performance56/plan.md),
[runtime architecture](design/runtime-architecture.md), and [status](STATUS.md),
in that order. The [roadmap](ROADMAP.md) gives stage outcomes; [process](PROCESS.md)
and [handoff](HANDOFF.md) explain development and runtime coordination.

The selected runtime remains selected57. Round67's two independently qualified
source candidates both lost their screens and were rejected. The grouped
[round68 proposal](design/proposals/interpreter-parity68.md) advances retained
generator delegation through independent architecture and source-contract
approval. Complete source, behavior and native mechanism qualification pass,
but protected hot8 losses reject selection despite a 29.2% generator elapsed
gain. Selected57 remains the runtime baseline. The next grouped review will
use these results before any further implementation.
The [trial report](issues/register-semantics59/report.md)
owns current qualification, rejected trials and retained evidence.

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
