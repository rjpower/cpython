# Restore universal PyValue under RC

- Stage: eight reviewed source checkpoints; first diagnostic build failed.
- Owner: coordinator; fresh state_audit designs dictionary boundary closure and
  wiki_audit designs the first scoped Clinic/parser family. Previous agents stay stopped.
- Goal: canonical PyValues across frames, containers, attributes and operations,
  retaining RC with cyclic collection.

## Actual state

Qualified interim images remain `eba351fee7d`, private `boilrc1`, boxed heap
fields. Restoration is `build/worktrees/pyvalue-rc-universal`, branch
`boil/pyvalue-rc-universal`, base `95be1d28a12`, current HEAD `015eb226fb3`.
The [board](../README.md) lists all eight reviewed commits. The only remaining
original uncommitted paths are Objects/abstract.c, object.c and typeobject.c,
held for named scalar-return/call and type-cache correctness findings.

The [first build](first-build/status.md) configured with the new `boilvalrc1`
namespace and produced 141 objects, including symtable, codegen and sysmodule.
It did not link; 616 repeated/cascading diagnostics span 96 source/header paths.
There is no restored runtime execution, qualification or timing result.

Foundation, containers, attributes, constants, compiler and selected VM/native
components have independent source reviews in the
[component index](source-components-20261102-v1.json). The latest
[compiler](compiler-metadata/status.md), [dictionary entry](owned-dict-entry/status.md)
and [native caller](native-callers/status.md) receipts match current source.
These are component reviews, not whole-system acceptance.

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
