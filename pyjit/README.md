# PyJIT

We are rebuilding CPython around canonical eight-byte PyValues, a register
interpreter, reference counting with cyclic collection, and a shared method
and tracing JIT. Extensions are rebuilt and ported.

Start with the [goal](GOAL.md), [current plan](issues/deltablue-call-model-20261110/plan.md)
and [status](STATUS.md). The active round targets DeltaBlue by improving
ordinary call resolution, execution and teardown. The [grouped design](issues/deltablue-call-model-20261110/design-overview.md)
compares canonical class lookup, classmethod forwarding and complete leaf-region
call elimination. Astra reviews the group once; Sol handles implementation and
component review. Useful incremental improvements may land with documented
tradeoffs. The [previous checkpoint](issues/object-call-20261109/checkpoint46-report.md)
retains measured results and limits.
Browser entrypoint: [Loom goal](http://127.0.0.1:7878/s/xzzlxx17/artifacts/goal).

The universal-value register interpreter is implemented. Native method/tracing
JIT tiers remain unfinished. The current [2026-11-09 golden](issues/golden-20261109/README.md) freezes
runtime39 for the active round. The earlier published
[2026-11-08 golden runtime](https://github.com/rjpower/cpython/tree/pyjit-golden-20261108-runtime)
remains an immutable comparison control; its
[manifest](issues/golden-20261108/README.md) retains qualification and identities.
Python's [lifetime policy](design/lifetimes-and-language-semantics.md) governs
semantics rather than exact CPython destruction timing.

| Read | Purpose |
| --- | --- |
| [Status](STATUS.md) | Current outcome and remaining work |
| [Architecture](design/runtime-architecture.md) | Implemented runtime and intended JIT tiers |
| [Register IR](design/register-ir.md) | Encoding, lowering, operands, control flow and source maps |
| [Interpreter](design/interpreter.md) | Dispatch, calls, frame ownership, observation and generated handlers |
| [PyValue](design/pyvalue.md), [value API](design/value-api.md), [native boundary](design/native-boundary.md) | Values, ownership and rebuilt extensions |
| [Golden review and next plan](issues/golden-20261108/next-steps.md) | Independent architectural recommendations and their admission gates |
| [Roadmap](ROADMAP.md) | Stage outcomes and dependencies |
| [Goal](GOAL.md), [process](PROCESS.md), [documentation ownership](DOCUMENTATION.md) | Standing decisions and how work lands |
| [Handoff](HANDOFF.md) | Exact local paths, hazards and recovery instructions |
| [Experiment index](issues/README.md), [work log](WORKLOG.md) | Detailed evidence and concluded experiments |

Project state belongs here; dated proposals and completed task boards are
historical evidence. The previous navigation pages are retained in
[the archive](archive/20261108/STATUS.md). Implementation stays in CPython's
usual source directories. Historical branch, build, tool and ABI names retain
their original identities.
