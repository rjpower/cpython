# Boil the ocean

We are rebuilding CPython with universal eight-byte PyValues, a register
interpreter, and a cohesive method and tracing JIT. Reference counting with
cyclic collection is the selected memory policy. Extensions are rebuilt and
ported; hard source or binary compatibility with upstream is not required.

## Where we are

The qualified interim interpreter is built from `eba351fee7d`: register IR and
NaN-boxed frame values, with boxed heap fields. Universal PyValue restoration
is on `boil/pyvalue-rc-universal` at `aa7a4335ddf`: forty-one reviewed
dependent source checkpoints. The remaining original held file is typeobject.c.
There is no linked or qualified restored interpreter or timing result.

**Current step:** close compiler/linker blockers under reviewed ownership rules.
Merged v12 has 332 repeated/cascading errors, 166 objects present and no executable;
all 7,347 source inputs stayed unchanged. Clinic producer rules now cover fixed
positional/keyword wrappers and seven constructor helpers. Three iterator
constructors, the recording-only MRO counter foundation and thirteen native
slot wrappers are committed. The earlier type_new boundary and ten construction
readers with selected base/metaclass pins are now committed. Full MRO
publication/rollback remains in revision.

The fifteen-function operator/LRU batch and flat-format parser kernel with five
callers are committed after independent code/native review. Luna corrects one
owner leak in the six-reader type packet and implements four reviewed MRO-adjacent
readers, in isolated copies. Sol designs the shared parser backend closure.
The coordinator reviews actual code/results, integrates
accepted deltas, rebuilds and repeats. Luna completed the latest error-family
census; counts are compiler observations, not independent edits or percent done.
See [porting rules and remaining work](issues/pyvalue-rc-restoration/porting/status.md)
and the [work board](issues/README.md) for scope, assignments and next gates.

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
