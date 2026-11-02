# Restoration integration and interpreter checkpoint

- Stage: eight source checkpoints; first real diagnostic build failed
- Owner: coordinator; fresh wiki_audit reviews checkpoint boundaries
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

Eight reviewed source commits end at `015eb226fb3`; their exact receipts are
linked from the [board](../../README.md). The shared composition commit
`c21d0a374ee` excludes three object/type files with concrete scalar/cache holds.
Codegen `ebc9740c4d5` and sys.flags `015eb226fb3` passed independent reviews and
compiled under the new debug configuration, alongside symtable.

The first configure passed; `make -k -j4 python` exited 2 before its 240-second
limit. It produced 141 objects but no executable. All 7,328 source manifest
paths were unchanged. The [build record](../first-build/status.md) owns raw
logs, grouped diagnostics, configuration and source hashes. No runtime pass.

Next: review and implement six dictionary boundary closures and the first
scoped Clinic/parser argument lease. Resolve held object/type bridges and cache
proof separately; port remaining concrete consumers. Reuse the configured build
for affected rebuilds, then link and run focused tests. Broad qualification and
quiet timing wait for a coherent candidate.
