# Stage map

Stages describe outcomes and gates, not percentages or an inflexible waterfall.
The baseline extension profile is part of runtime integration; wider extension
migration can proceed alongside later compiler work.

```mermaid
flowchart LR
    Runtime[1 Runtime integration] --> Baseline[2 Qualified interpreter baseline]
    Baseline --> Method[3 Shared compiler and method JIT]
    Method --> Trace[4 Tracing JIT]
    Runtime --> Extensions[5 Extension ecosystem]
```

| Stage | Deliverable | Exit evidence |
| --- | --- | --- |
| [1. Runtime](stages/01-runtime/README.md) | One coherent canonical-value runtime under RC | Selected build links and runs (done); suite near green and ownership contracts closed (open) |
| [2. Baseline](stages/02-baseline/README.md) | A trustworthy register-interpreter starting point | Declared correctness/profile qualification and raw matched 29-workload JIT-off results |
| [3. Method JIT](stages/03-method-jit/README.md) | Cohesive method compilation through shared optimizing IR | Whole-method/native transitions, recovery, ownership and measured workloads validated |
| [4. Tracing](stages/04-tracing-jit/README.md) | Hot-path compilation using the same compiler core | Trace guards, exits, recovery and interactions with method code validated and measured |
| [5. Extensions](stages/05-extensions/README.md) | Practical incremental source ports and opaque interfaces | Declared extensions rebuild and pass their behavior/lifetime tests against the selected API |

Within a stage, workstreams may proceed in parallel when ownership is disjoint.
Each workstream links its current experiment and states the next gate. An agent
working on one packet does not change stage status to complete; the coordinator
does that after validating the composition and its exit evidence.

Keep progress labels distinct: designed, independently reviewed, implemented,
tested, committed, integrated, qualified and measured. A successful component
compile closes only that component gate. The current states live in
[STATUS](STATUS.md); selected subsystem contracts live in [design](design/runtime-architecture.md).
