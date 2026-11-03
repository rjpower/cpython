# Universal-value runtime restoration

Parent: [runtime integration](../../stages/01-runtime/README.md).
The [project status](../../STATUS.md) owns current aggregate progress and the
[handoff](../../HANDOFF.md) owns exact worktree, commit and packet identities.

The selected target is canonical eight-byte PyValues across interpreter
operations, containers and attributes, with RC and cyclic collection. The
qualified interim image has inline frame values but boxed heap fields; it is
not the restored target. No restored-runtime qualification or timing is claimed.

The reviewed source chain covers representation/storage, dictionary boundaries,
compiler/VM/native producers and consumers, and later bounded ownership ports.
The [component index](source-components-20261102-v1.json) binds those checkpoints.
The remaining original held file and test have a finite accepted projection but
still need fresh integration gates. Separate lookup and version/feedback packets
close concrete remaining contracts.

| Local workstream | Detailed records |
| --- | --- |
| [Consumers](../../stages/01-runtime/workstreams/consumers.md) | [Port rules/census](porting/status.md), [core/bundled consumers](core-consumers/status.md) |
| [Type system](../../stages/01-runtime/workstreams/type-system.md) | [Type/cache admission](typecache/status.md) |
| [Native boundaries](../../stages/01-runtime/workstreams/native-boundaries.md) | [Scoped parser records](native-parsers/status.md) |
| [Integration](../../stages/01-runtime/workstreams/integration.md) | [Merged build](merged-build/status.md), [qualification/measurement](integration/status.md) |

Correct owning object views are valid at C boundaries. Value-native bridge and
iteration tuning can follow the first measurement. Extensions are rebuilt and
ported; hard compatibility is not an approval gate. Read the
[process](../../PROCESS.md) before assigning a packet.
