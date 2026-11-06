# PyJIT

We are rebuilding CPython around universal eight-byte PyValues, a register
interpreter, and a cohesive method and tracing JIT. Memory management uses
reference counting with cyclic collection. Extensions are rebuilt and ported.

**Current outcome:** runtime HEAD is `6a0e4c12c17` (constructor and format
work), following `05286694774` (native child accounting).
All five build-info objects match their respective source HEADs: upstream
`3a440b65695`, interim `eba351fee7d`, and candidate `6a0e4c12c17`. The module
rosters are 116/116/117. Both full profiles passed 54,351 tests with no
failures (release: 2,531 skips; debug: 2,469). The strict whole-source R27
census failed: 6,512 dumps contain 11,028,818 core/core-pending/unresolved
boxes and incomplete process identity/accounting. The raw capture finished
with 87 records across 29 kernels: 18/29 startup-inclusive raw-zero, all loop
deltas zero, but strict kernel admission failed. See the [full census receipt](issues/checkpoint-closure/full-census-6a0e4c12c17.md).
The matched-build preflight passed but is ineligible as a measurement.

The earlier full-v2 failures remain recorded as historical evidence. Audit
static-gate violations and process-accounting gaps before changing marks or
source. A diagnostic benchmark run is active, but it cannot qualify performance
while R27 fails. No result is available yet.

Start with [current status](STATUS.md) for progress and blockers, or the
[stage map](ROADMAP.md) for the route to completion. A new coordinator should read
the [handoff](HANDOFF.md) before assigning or executing work.

```mermaid
flowchart TD
    Overview[Project overview and status] --> Stages[Stage outcomes and exit gates]
    Stages --> Streams[Workstreams and current packets]
    Streams --> Experiments[Design, review, implementation and evidence]
```

| View | Question it answers |
| --- | --- |
| [Status](STATUS.md) | What works, what blocks us, and what happens next? |
| [Roadmap](ROADMAP.md) | What must each stage deliver? |
| [Architecture](design/runtime-architecture.md) | How do values, IR, interpreter, memory and JIT fit together? |
| [PyValue](design/pyvalue.md), [native boundary](design/native-boundary.md), [register IR](design/register-ir.md), [value API](design/value-api.md) | What are the canonical specs for the value, the C boundary, the IR and the value-native API? Read them after the architecture page. |
| [Process](PROCESS.md) and [gates](design/validation.md) | How do we admit a change? |
| [Standing direction](GOAL.md) | Which decisions must survive a handoff? |
| [Experiment index](issues/README.md) and [log](WORKLOG.md) | Where is the detailed evidence? |
| [Documentation ownership](DOCUMENTATION.md) | Which page should an agent update? |

The `pyjit/` directory owns project documentation, benchmarks and local gate
tools. CPython implementation stays in its usual source directories. Build
trees, historical artifacts, existing branches and ABI identifiers retain their
recorded names; see the handoff before reusing an older script.
