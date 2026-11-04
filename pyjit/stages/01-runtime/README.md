# Stage 1: coherent runtime integration

State: in progress. Owner role: coordinator, with bounded Sol design/review and
Luna analysis/mechanical implementation packets. The role list is not an active assignment list.

Deliver one runtime with canonical eight-byte PyValues across frames,
containers and attributes; a register interpreter; RC with cyclic collection;
and correct owning native boundaries. The selected baseline profile is Linux
x86-64/GIL. Do not reopen the closed tracing-GC/nursery work.

| Workstream | Responsibility | Current next step |
| --- | --- | --- |
| [Consumer ports](workstreams/consumers.md) | Core/startup and selected bundled container consumers | Core, stdlib, system and test modules ported; fix what suite triage finds |
| [Type system](workstreams/type-system.md) | Value-aware lookup, slots, mutation, cache and guard contracts | Type projection committed (checkpoint57); close lookup/version packets |
| [Native boundaries](workstreams/native-boundaries.md) | Per-thread borrow stack, getargs, Clinic | P1 to P3 landed; finish P4 and the post-review |
| [Integration](workstreams/integration.md) | Exact composition, commits and merged build | Suite triage, then the representation packet |

The selected build now links and runs (debug and release; debug suite 484 of
619 files OK at 17:31, 30 failed, 3 env changed). Exit when known required
ownership and admission contracts are closed and the suite is near green.
Stage 2 owns broader qualification and the first matched performance baseline. Owning temporary object views are valid; removing their
boxing costs is later tuning.

See [current status](../../STATUS.md), [handoff](../../HANDOFF.md),
[architecture](../../design/runtime-architecture.md) and
[runtime experiment records](../../issues/pyvalue-rc-restoration/status.md).
