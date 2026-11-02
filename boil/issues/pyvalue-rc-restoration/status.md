# Restore universal PyValue under RC

- Stage: implementation composed in a worktree; fresh source assignments pending
  after the wiki reset. This is not committed integration or a qualified runtime.
- Owner: coordinator. Previous source/review agents are stopped; new ownership
  is assigned through the [current board](../README.md).
- Goal: one canonical PyValue across frames, containers, attributes and core
  operations, retaining RC with cyclic collection.

## Actual state

The qualified interim images are `eba351fee7d`, private `boilrc1`, with boxed
heap fields. Restoration source is in `build/worktrees/pyvalue-rc-universal`,
branch `boil/pyvalue-rc-universal`, base `95be1d28a12`. At the wiki audit it has
127 tracked changes and 25 untracked source/test paths, with no restoration
source commits, linked candidate, runtime qualification or measured result.
Primary HEAD at audit was `204acca6939`; its newer changes were documentation.

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
