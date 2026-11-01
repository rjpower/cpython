# Boil the ocean

A CPython performance fork with compiler-produced register IR, eight-byte
NaN-boxed frame values, boxed-slot shapes, reference counting and cyclic collection.
The upstream base is `321925c92c9`. Interpreter improvements precede JIT work;
roughly 2× on simple arithmetic is guidance, not a numerical gate.

**Current decision (2026-11-01):** stop tracing-GC optimization and adopt the
[reference-counting successor](issues/rc-integration/status.md). Candidate
qualification, local integration and fresh release/debug checks are accepted.
Register IR, eight-byte NaN-boxed frame values and boxed-slot shapes are retained. Later
core-wide inline heap values and their ABI are deferred. No collector switch
will be added. [Decision and evidence](issues/gc-parity-deadline/decision-20261101.md).

Current images, built from `eba351fee7d`:

```sh
./build/rc-integrated-release-20261101/python your_script.py
./build/rc-integrated-debug-20261101/python your_script.py
```

These Linux/GIL/GCC builds retain the private `boilrc1` ABI and the documented
optional-module limits. Global `Py_STACKREF_DEBUG` is unsupported and unvalidated.
The first JIT increment is an isolated experiment; these images contain no new JIT.

| Need | Start here |
| --- | --- |
| Current work, owners and next actions | [Issue board](issues/README.md) |
| Runtime structure and contracts | [Runtime architecture](design/runtime-architecture.md) |
| Proposed integrated runtime and experiments | [IR, VM and allocator/collector reassessment](design/integrated-runtime-candidate.md) |
| Current hypotheses and parallel assignments | [Runtime work after peer review](design/runtime-work-breakdown.md) |
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
