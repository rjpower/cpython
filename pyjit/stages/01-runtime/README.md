# Stage 1: coherent runtime integration

State: integrated and qualified at runtime `0953757f4e7`. Current optimization
work is tracked under [interpreter parity](../../issues/interpreter-parity-20261108/plan.md).
Owner role: coordinator; role names are not an active assignment list.

Deliver one runtime with canonical eight-byte PyValues across frames,
containers and attributes; a register interpreter; RC with cyclic collection;
and correct owning native boundaries. The selected baseline profile is Linux
x86-64/GIL. Do not reopen the closed tracing-GC/nursery work.

The table records earlier integration packets. Their dated pages are history;
the qualified report and active parity packet own current admission work.

| Workstream | Responsibility | Historical next step |
| --- | --- | --- |
| [Consumer ports](workstreams/consumers.md) | Core/startup and selected bundled container consumers | Core, stdlib, system and test modules ported; fix what suite triage finds |
| [Type system](workstreams/type-system.md) | Value-aware lookup, slots, mutation, cache and guard contracts | Type projection committed (checkpoint57); close lookup/version packets |
| [Native boundaries](workstreams/native-boundaries.md) | Per-thread borrow stack, getargs, Clinic | P1 to P3 landed; finish P4 and the post-review |
| [Integration](workstreams/integration.md) | Exact composition, commits and merged build | Qualified baseline complete; continue the active parity experiment |

The selected runtime completed release and debug full-suite qualification with
no failures. The 2026-11-07 report records the qualification and matched
measurements. The active parity packet proposes source-backed interpreter/IR
changes against V8 and uses `logging_silent` as its selected workload.

See [current status](../../STATUS.md), [handoff](../../HANDOFF.md),
[architecture](../../design/runtime-architecture.md) and
[runtime experiment records](../../issues/pyvalue-rc-restoration/status.md).
