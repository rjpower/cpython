# Restore universal PyValue under RC

- Stage: reviewed source checkpoints; merged diagnostic build still has no executable.
- Owner: Luna compiler-error analysis/bulk ports; Sol 6.1 designs/reviews fixes;
  coordinator rules, source review and integration.
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
gates and are committed. The type_new boundary and construction readers are
committed; reviewed graph and other provider deltas still await typeobject
composition. They do not yet form an executable candidate.
The [first bulk AST port](porting/status.md) is committed: one producer covers
74 sequence reads. The subsequent constructor port is committed and its full
TU/object pass without diagnostics. The unified Clinic rule covers 48 production
wrappers and is committed, as are three pure predicates and fourteen core
readers, Template readers, the TypeVar parser and descriptor calls. Shared
bytes/bytearray join is checkpoint32 and ABC pair readers are checkpoint33;
Clinic helper constructors are checkpoint34, iterator constructors checkpoint35,
the recording-only counter foundation checkpoint36 and slot wrappers checkpoint37.
The earlier type_new argument boundary is checkpoint38 and construction readers
with explicit caller pins checkpoint39. Two Luna streams implement operator/LRU
readers and the flat-format parser kernel/five callers in isolated copies. Sol
designs the remaining type-reader packet.
Root integrates reviewed code and validates the composition.
The MRO providers plus one immediate wrapper passed independent
code/native attribution review; their syntax comparison has 61 diagnostics,
versus 86 before, including
two missing-API first-use messages relocated to unchanged functions.

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
link. Its latest v11 snapshot at checkpoint39 plus held typeobject has 164 objects
present and 350 repeated/cascading error diagnostics. All 7,345 source paths
remained unchanged; no candidate was executed. Iterator, slot-wrapper and
construction-reader selected contexts are native-clean; the current typeobject
TU has twelve remaining errors and 23 warnings. The counter comparison uses corresponding
old/new public layouts and adds no diagnostics, without admitting a transaction.
The full MRO transaction remains in revision.

## Remaining increments

[Core/extension consumers](core-consumers/status.md),
[type-cache correctness](typecache/status.md), and
[native parsers/registration](native-parsers/status.md) precede
[coordinator integration and measurement](integration/status.md).
Use [reviewed porting batches](porting/status.md) for repetitive work.
Correct owning object bridges are allowed in the first measurement;
[value-native VM iteration](iterator-vm/status.md) is subsequent tuning.
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
