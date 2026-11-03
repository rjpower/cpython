# Stage 1: coherent runtime integration

State: in progress. Owner role: coordinator, with bounded Sol design/review and
Luna analysis/mechanical implementation packets. Current workers are stopped
for handoff; the role list is not an active assignment list.

Deliver one runtime with canonical eight-byte PyValues across frames,
containers and attributes; a register interpreter; RC with cyclic collection;
and correct owning native boundaries. The selected baseline profile is Linux
x86-64/GIL. Do not reopen the closed tracing-GC/nursery work.

| Workstream | Responsibility | Current next step |
| --- | --- | --- |
| [Consumer ports](workstreams/consumers.md) | Core/startup and selected bundled container consumers | Reuse reviewed ownership patterns for remaining compiler families |
| [Type system](workstreams/type-system.md) | Value-aware lookup, slots, mutation, cache and guard contracts | Checkpoint held projection and close lookup/version packets |
| [Native boundaries](workstreams/native-boundaries.md) | Parser/converter leases, public scopes and caller migration | Review/compose backend and public/caller closure |
| [Integration](workstreams/integration.md) | Exact composition, commits and merged build | Integrate accepted increments and repeat build/error analysis |

Exit when the selected build links, known required ownership and admission
contracts are closed, and startup plus the first focused value/lifetime tests
execute successfully. Stage 2 owns broader qualification and the first matched
performance baseline. Owning temporary object views are valid; removing their
boxing costs is later tuning.

See [current status](../../STATUS.md), [handoff](../../HANDOFF.md),
[architecture](../../design/runtime-architecture.md) and
[runtime experiment records](../../issues/pyvalue-rc-restoration/status.md).
