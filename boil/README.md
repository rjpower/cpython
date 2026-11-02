# Boil the ocean

We are rebuilding CPython with universal eight-byte PyValues, a register
interpreter, and a cohesive method and tracing JIT. Reference counting with
cyclic collection is the selected memory policy. Extensions are rebuilt and
ported; hard source or binary compatibility with upstream is not required.

## Where we are

The qualified interim interpreter is built from `eba351fee7d`: register IR and
NaN-boxed frame values, with boxed heap fields. Universal PyValue restoration
is on `boil/pyvalue-rc-universal`. Reviewed ports
now include the object/attribute bridge, positional constructors and small
tokenizer/bytes readers and object C call bridges. The remaining original held file is typeobject.c.
There is no linked or qualified restored interpreter or timing result.

**Current step:** close the concrete blockers from the merged diagnostic build.
The latest merged build still failed before linking. The C API call bridges now
compile, and their focused checks and independent reviews pass. Typeobject has
reviewed provider, mutation, constructor and graph deltas; its remaining C3/MRO,
slot and publication consumers still need closure. Wiki_audit prepares the next
finite MRO slice; state_audit reviews it. The coordinator integrates reviewed
changes and validates the merged result.
See the [work board](issues/README.md) for assignments and next gates.

Next milestone: measure the unified-value register interpreter with JIT
explicitly disabled against matched interim RC and upstream controls. Then
continue the shared method/tracing JIT. No new JIT is in the interim images.

## Project wiki

| Page | Purpose |
| --- | --- |
| [Standing goal](GOAL.md) | User direction and recovery after restart or compaction |
| [Process](PROCESS.md) | Increment lifecycle, compatibility, extension ports and integration |
| [Overall design](design/runtime-architecture.md) | System structure and IR, interpreter, values and JIT details |
| [Testing and admission](design/validation.md) | Feedback, qualification, performance and merge gates |
| [Current work](issues/README.md) | One row per experiment, current stage and next action |
| [Experiment log](WORKLOG.md) | Concise concluded outcomes; detail stays with each experiment |
| [Documentation rules](DOCUMENTATION.md) | Authoritative pages, evidence and historical records |

## Run the interim interpreter

```sh
./build/rc-integrated-release-20261101/python your_script.py
./build/rc-integrated-debug-20261101/python your_script.py
```

These are the earlier Linux x86-64/GIL/GCC builds with private `boilrc1` ABI;
they do not contain restored universal heap values. Profile and optional-module
limits are recorded in [RC integration](issues/rc-integration/status.md).
Upstream base: `321925c92c9`. Earlier tracing-GC work is closed; its
[decision](issues/gc-parity-deadline/decision-20261101.md) and evidence remain.
