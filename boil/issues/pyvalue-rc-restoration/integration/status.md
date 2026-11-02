# Restoration integration and interpreter checkpoint

- Stage: dependent source checkpoints; merged diagnostic build failed
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

The [board](../../README.md) owns the current restoration HEAD and checkpoint
list. Object composition and small tokenizer/bytes consumers are committed;
typeobject.c is the remaining original held file. Clinic covers 20 further
positional wrappers; classobject/codeobject compile. Core mutation dependencies and object calls are checkpointed; graph/type_new
deltas remain held. C3/MRO providers plus one wrapper passed independent
code/native attribution review. The generated AST sequence and constructor
batches are committed; their full TU/object pass. The unified Clinic rule now
covers 48 production wrappers and is committed, as are the list-sort assertions.
Remaining actual parser, container/
startup and bundled consumer ports still block a baseline executable.

The latest [merged diagnostic build](../merged-build/status.md) binds the
25-checkpoint chain at 4bf934bb4ec plus then-unaccepted frozen Clinic v2 and
reviewed held typeobject through the corrected MRO providers. It exited 2, with
161 objects present and no executable. All 7,341 source paths stayed unchanged;
430 repeated/cascading error
diagnostics remain raw evidence. Focused component checks do not make this a
passing merged build. Clinic v2 subsequently passed review and was committed
as checkpoint26. Three pure predicates are checkpoint27; core14's first draft
needs finite corrections after early scope/cleanup/syntax review.

Prioritize a coherent register interpreter with the unified format and JIT off.
Use concrete failed compiler/linker/startup routes to select finite ports, preserving
errors and owning canonical values. Rebuild the merged tree after useful accepted
boundary changes; use runtime failures to choose the next correction. Broad
qualification and quiet timing wait for a coherent candidate. New optimization
and JIT implementation wait for this baseline.
