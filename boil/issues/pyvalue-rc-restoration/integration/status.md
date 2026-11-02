# Restoration integration and interpreter checkpoint

- Stage: sixteen source checkpoints; merged diagnostic build failed
- Owner: coordinator; independent peer reviews precede each source checkpoint
- Goal: produce one coherent universal-value RC runtime, validate composition,
  then measure the register interpreter before further JIT performance work.

## Commit and integrate

Review the current composed worktree and create dependency-ordered source
commits. Preserve unrelated work and existing evidence. Avoid another long
uncommitted accumulation: a useful dependent source checkpoint can be committed
with its unqualified/non-runnable status explicit. Commit identity is not a
runtime pass. Merge reviewed increments and resolve shared interfaces in the
coordinator-owned integration tree.

Build as soon as the source dependency chain permits; fix concrete compile/link
and consumer gaps with scoped reviewed increments. Verify rebuilt extension and
private ABI identities. Execute focused lifetime, value, container, attribute,
iteration and native routes on the merged tree, then applicable broad release/
debug/C-extension/stress/sanitizer qualification. Follow [gates](../../../design/validation.md).

## Measure

Prove JIT execution is disabled, then compare the matched unified interpreter,
interim RC and upstream controls over the unchanged 29-workload roster. Preserve
raw vectors and report workload/aggregate elapsed and memory, separating startup,
lowering and warm execution. No restored timing exists yet; earlier RC/GC/JIT
results cannot supply it. Use results to improve specific mechanisms.

## Current checkpoint

Sixteen reviewed source commits end at `3296af9ba58`; their exact receipts are
linked from the [board](../../README.md). The shared composition commit
`c21d0a374ee` excludes three object/type files with concrete scalar/cache holds.
Codegen `ebc9740c4d5` and sys.flags `015eb226fb3` passed independent reviews and
compiled under the new debug configuration, alongside symtable.

The first configure passed; `make -k -j4 python` exited 2 before its 240-second
limit. It produced 141 objects but no executable. All 7,328 source manifest
paths were unchanged. The [build record](../first-build/status.md) owns raw
logs, grouped diagnostics, configuration and source hashes. No runtime pass.

The dictionary API cut `18b6053c75f` removes the six incomplete borrowed paths;
actual Dict and fixture TUs pass and dictobject.o compiles. Exception consumers
`d1c8041c78c` pass full syntax and errors.o compilation. Runtime cases remain
unexecuted, and their header/caller changes have not yielded a new linked build.

Canonical read providers `99012f782ea` compile and define four strong symbols;
existing compile.o and flowgraph.o reference GetValue. Thirteen candidate methods
remain unexecuted. PopValue is still a separate missing mutation provider.

Generic NativeScope adapters `1a2adf3da57`, scoped constructor parsers
`55d410ca39f` and marshal consumers `69a1b3ea533` passed independent review and
are committed. NativeScope, Struct and marshal compile as real objects. Parser
diagnostics include failures in unchanged consumer bodies; these are not full-TU
passes. Candidate tests remain unexecuted. See
[native/parser status](../native-parsers/status.md) and
[marshal status](../core-consumers/marshal/status.md).

The [caller transaction](../native-parsers/caller-transaction/status.md)
`6780ba211ac` is committed; actual evaluator and observer TUs and ceval.o pass.
Eight candidate methods remain unexecuted. The subsequent
[merged diagnostic build](../merged-build/status.md) has 145 objects, exits 2
with no executable and preserves all 7,332 source inputs. It includes the three
held object/type files for diagnostics; it does not admit them.

The [instance/subclass port](../core-consumers/abstract/status.md) is now
committed as `3296af9ba58`; full TU/object and 23 host controls pass, while
twelve candidate methods remain unexecuted. No full build was repeated merely
for this component; the merged diagnostic record above binds the earlier chain.

Next: implement the reviewed type-cache provider and design the fixed-arity
positional Clinic port. Remaining parser/Clinic
and bundled consumers need owning views. Reuse affected checks, then link and
run focused tests. Broad qualification and quiet timing wait for a coherent
candidate.
