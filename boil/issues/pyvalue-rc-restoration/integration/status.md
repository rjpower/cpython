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
deltas remain held. C3/MRO design is next. Remaining actual parser, container/
startup and bundled consumer ports still block a baseline executable.

The latest [merged diagnostic build](../merged-build/status.md) binds the earlier
21-checkpoint chain at `7b923d2b935` plus held typeobject/type_new. It exited 2,
with 152 objects present and no executable. All 7,337 source paths stayed unchanged;
540 repeated/cascading diagnostics remain raw evidence. Later focused graph and
call checks are separate and do not turn that merged build into a pass.

Prioritize a coherent register interpreter with the unified format and JIT off.
Use concrete failed compiler/linker/startup routes to select finite ports, preserving
errors and owning canonical values. Rebuild the merged tree after useful accepted
boundary changes; use runtime failures to choose the next correction. Broad
qualification and quiet timing wait for a coherent candidate. New optimization
and JIT implementation wait for this baseline.
