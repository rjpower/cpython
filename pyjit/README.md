# PyJIT

We are rebuilding CPython around canonical eight-byte PyValues, a register
interpreter, reference counting with cyclic collection, and a shared method
and tracing JIT. Extensions are rebuilt and ported.

The **2026-11-08 golden runtime** is published as
[`pyjit-golden-20261108-runtime`](https://github.com/rjpower/cpython/tree/pyjit-golden-20261108-runtime).
It is the reproducible baseline for the next experiments. Release and debug
qualification passed 54,394 tests each. It remains slower than upstream:
about 1.38× elapsed time on logging and 1.24× on the broad benchmark screen.
The native JIT is unfinished. Read the [golden manifest](issues/golden-20261108/README.md)
for exact identities, qualifications and measurement limits.

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
