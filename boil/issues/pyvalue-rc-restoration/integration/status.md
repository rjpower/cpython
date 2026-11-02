# Restoration integration and interpreter checkpoint

- Stage: five source checkpoints committed; shared composition and consumers pending
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

Foundation `359ea4732de` commits the frozen reviewed patch without rewriting
later working changes. Exactly three Windows files use their declared Git
clean filters; all 152 working source paths are unchanged. It remains a
non-runnable source checkpoint. [Foundation record](../foundation/status.md).

Canonical storage/provider checkpoint `edab780a715` commits exactly 38 reviewed
files, preserving all later working changes. [Storage record](../container-storage/status.md).

Dictionary composition `9b4ff426d1a` includes the corrected scalar-builtins
bridge; symtable `23cec84d282` includes its reviewed getter correction; private
ABI `359d10f4d4d` separates universal-value artifacts. All three passed independent
source review. No candidate has been configured or built. Six legacy Dict
functions and core consumer ports still block a coherent build.

Next: review and commit shared interpreter/native/compiler composition, implement
sys.flags and codegen ports, then close remaining concrete build failures. The [commit plan](../../../../build/pyvalue-source-integration-20261102-v1/commit-plan.md)
records exact exclusions; no blanket shared-file staging is permitted.
