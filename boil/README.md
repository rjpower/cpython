# Boil the ocean

A CPython performance fork with compiler-produced register IR, eight-byte
NaN-boxed values, shapes and tracing garbage collection without reference counts.
The upstream base is `321925c92c9`. Interpreter improvements precede JIT work;
roughly 2× on simple arithmetic is guidance, not a numerical gate.

| Need | Start here |
| --- | --- |
| Current work, owners and next actions | [Issue board](issues/README.md) |
| Runtime structure and contracts | [Runtime architecture](design/runtime-architecture.md) |
| Performance findings and recommendation | [GC and interpreter performance](design/gc-and-interpreter-performance.md) |
| Measured results | [Performance report](issues/gc-cleanup-scanning/performance.md) |
| Development workflow | [Process](PROCESS.md) |
| Document organization and experiments | [Documentation](DOCUMENTATION.md) |
| High-level outcomes | [Worklog](WORKLOG.md) |

Each issue's `status.md` owns its current state. Designs explain contracts;
plans describe the work; results retain evidence. Start with the relevant issue
and actual source/process state rather than inferring an assignment from a design.

The coordinator integrates changes and maintains the board. Owners execute
bounded tasks; independent reviewers check significant changes. Source, build
and measurement identities belong in the relevant result record.

Keep WORKLOG append-only: concise, high-level actions and outcomes with links.
Detailed evidence stays with the linked issue results.
