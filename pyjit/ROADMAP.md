# Stage map

Stages describe outcomes and gates, not percentages or an inflexible waterfall.
The baseline extension profile is part of runtime integration; wider extension
migration can proceed alongside compiler work. [Status](STATUS.md) records
current qualification, measurements and limitations.

```mermaid
flowchart LR
    Runtime[1 Runtime integration] --> Baseline[2 Interpreter baseline]
    Baseline --> Method[3 Shared compiler and method JIT]
    Method --> Trace[4 Tracing JIT]
    Runtime --> Extensions[5 Extension ecosystem]
```

| Stage | Deliverable | Exit evidence |
| --- | --- | --- |
| [1. Runtime](stages/01-runtime/README.md) | Canonical-value runtime under RC | Integrated runtime passes behavior and ownership gates |
| [2. Baseline](stages/02-baseline/README.md) | Trustworthy register interpreter | Qualified behavior and measured performance, with limits recorded |
| [3. Method JIT](stages/03-method-jit/README.md) | A baseline method JIT generated from the interpreter's operation contract | Native transitions, exceptions, generators, observation and ownership validated with every code object compiled; workloads measured |
| [4. Tracing](stages/04-tracing-jit/README.md) | Hot-loop compilation through SSA from the same contract, sharing the baseline's backend | Guards, exits, recovery and baseline interactions validated |
| [5. Extensions](stages/05-extensions/README.md) | Incremental source ports and opaque interfaces | Declared extensions rebuild and pass behavior and lifetime tests |

Workstreams can proceed in parallel when ownership is disjoint. The coordinator
validates composition and stage exit evidence. The [tiers87 proposal](design/proposals/tiers87.md) sets the path from the
interpreter's last wave through both JITs; the
[active plan](issues/interpreter-performance56/plan.md) describes the work; [designs](design/runtime-architecture.md) describe contracts.
