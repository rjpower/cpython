# Boil the ocean

We are rebuilding CPython with universal eight-byte PyValues, a register
interpreter, and a cohesive method and tracing JIT. Reference counting with
cyclic collection is the selected memory policy. Extensions are rebuilt and
ported; hard source or binary compatibility with upstream is not required.

## Where we are

The qualified interim interpreter is built from `eba351fee7d`: register IR and
NaN-boxed frame values, with boxed heap fields. Universal PyValue restoration
is on `boil/pyvalue-rc-universal`. Reviewed ports
now include the object/attribute bridge, positional constructors, small
tokenizer/bytes readers, object C call bridges, an AST sequence producer batch
and the AST constructor. The AST translation unit and object now build.
The unified keyword Clinic batch, list-sort value assertions, core readers and
Template readers are also committed. The remaining original held file is typeobject.c.
There is no linked or qualified restored interpreter or timing result.

**Current step:** close the concrete blockers from the merged diagnostic build.
The latest merged build still failed before linking. The C API call bridges now
compile, and their focused checks and independent reviews pass. Typeobject has
reviewed provider, mutation, constructor and graph deltas. MRO providers have
passed code/native attribution review; publication, slots and lifecycle still
need closure. No merged executable exists. Luna grouped v4's compiler errors;
the reviewed unified Clinic rule now covers 48 production wrappers. The latest
v9 compiler probe has 376 repeated/cascading errors and no executable; all 7,343
source inputs stayed unchanged. Thirty-five reviewed source checkpoints now include
fourteen core readers, three pure predicates, Template readers, the TypeVar
parser, descriptor calls, shared bytes/bytearray join and ABC pair readers.
The shared Clinic helper rule and three iterator constructors are committed.
Luna groups the v9 errors; Sol independently reviews the slot-wrapper code and
type-construction design. Root integrates the isolated MRO counter foundation
and reviewed wrappers. The full MRO transaction needs revision.
Sol 6.1 designs/reviews reusable fixes; approved rules go to mechanical or Luna batches.
The coordinator integrates reviewed results, rebuilds and repeats.
See [porting rules and remaining work](issues/pyvalue-rc-restoration/porting/status.md)
for mandatory ownership ports versus later performance tuning.
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
