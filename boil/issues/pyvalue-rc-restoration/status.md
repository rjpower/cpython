# Restore universal PyValue under RC

- Stage: reviewed source checkpoints; merged diagnostic build still has no executable.
- Owner: state_audit startup/parser design; wiki_audit type-cache core; coordinator integrates.
  Previous agents stay stopped.
- Goal: canonical PyValues across frames, containers, attributes and operations,
  retaining RC with cyclic collection.

## Actual state

Qualified interim images remain `eba351fee7d`, private `boilrc1`, boxed heap
fields. Restoration is `build/worktrees/pyvalue-rc-universal`, branch
`boil/pyvalue-rc-universal`, base `95be1d28a12`. The [board](../README.md) owns
the current HEAD and reviewed checkpoint list. Object composition is committed; typeobject.c is the remaining
original held file. The reviewed typeobject provider delta stays distinguishable
from unfinished scalar/slot work. Positional Clinic, object/attribute/repr and
small tokenizer/bytes and object call consumers have passed their finite source
gates and are committed. Type_new and graph repairs are applied/reviewed pending
typeobject composition. They do not yet form an executable candidate.

The [first build](first-build/status.md) configured with the new `boilvalrc1`
namespace and produced 141 objects, including symtable, codegen and sysmodule.
It did not link; 616 repeated/cascading diagnostics span 96 source/header paths.
There is no restored runtime execution, qualification or timing result.

Foundation, containers, attributes, constants, compiler and selected VM/native
components have independent source reviews in the
[component index](source-components-20261102-v1.json). The latest
[compiler](compiler-metadata/status.md), [dictionary entry](owned-dict-entry/status.md)
and [native caller](native-callers/status.md) receipts describe their frozen
checkpoints. The later [caller transaction](native-parsers/caller-transaction/status.md)
repairs and checkpoints their composition; its candidate runtime tests remain
unexecuted. The [merged diagnostic build](merged-build/status.md) still fails to
link. Its latest v3 snapshot at 21 checkpoints has 152 objects present and
540 repeated/cascading diagnostics; later graph/call checks remain separate.

## Remaining increments

[Core/extension consumers](core-consumers/status.md),
[actual VM iteration](iterator-vm/status.md),
[type-cache correctness](typecache/status.md), and
[native parsers/registration](native-parsers/status.md) precede
[coordinator integration and measurement](integration/status.md).
The coordinator first checkpoints reviewed source in coherent dependent commits,
then builds/tests the merged result as soon as the dependency chain permits.

Hard compatibility is not a gate or a pending approval. All extensions are rebuilt
and ported through owning, fallible bridges and gradually opaque APIs; see
[process](../../PROCESS.md) and [extension design](../../design/extension-api.md).
Prepared but unexecuted dictionary scripts/rejections are historical evidence,
not active assignments. Their existence is not an implementation result.

Current design: [values](../../design/values-and-object-protocol.md),
[memory/ownership](../../design/gc-roots-and-lifetimes.md),
[extension API](../../design/extension-api.md). Detailed prior work remains in
[implementation progress](implementation-progress-20261101.md) and the
[pre-reset status](history/status-before-wiki-reset-20261102.md).
