# PyJIT

PyJIT rebuilds CPython around canonical eight-byte PyValues, a register
interpreter, reference counting with cyclic collection, and a shared method
and tracing JIT. Extensions are rebuilt and ported.

Start with the [goal](GOAL.md), [active plan](issues/activation53/plan.md)
and [status](STATUS.md). The register interpreter and universal PyValues are
implemented; native method and tracing JIT tiers remain unfinished. Round 51
remains the selected runtime. The [round 52 report](issues/generator-delegation52/report.md)
records the closed delegation experiment; the [round 51 report](issues/interpreter-round51/report.md)
describes the selected checkpoint.

| Read | Purpose |
| --- | --- |
| [Roadmap](ROADMAP.md) | Stage outcomes and dependencies |
| [Runtime architecture](design/runtime-architecture.md) | Runtime structure and planned JIT tiers |
| [Register IR](design/register-ir.md), [interpreter](design/interpreter.md) | Encoding, execution and observation |
| [Calls and frames](design/call-and-frame-lifetimes.md) | Ownership, storage and completion |
| [PyValue](design/pyvalue.md), [value API](design/value-api.md), [native boundary](design/native-boundary.md) | Values and rebuilt extensions |
| [Process](PROCESS.md), [documentation](DOCUMENTATION.md) | Development and documentation rules |
| [Handoff](HANDOFF.md) | Execution paths, build state and recovery |

The [lifetime policy](design/lifetimes-and-language-semantics.md) governs
Python behavior and ownership without requiring exact CPython destruction timing.
