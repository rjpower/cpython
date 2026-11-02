# Standing goal and recovery

Carry the CPython rebuild through implementation, independent review,
integration, qualification and measured outcomes. The coordinator owns the
whole result, documentation and user updates. This page records standing user
direction; Loom's `goal` artifact mirrors it.

## Decisions to retain

- Use universal canonical eight-byte PyValues across interpreter operations,
  containers and attributes. A frame-only value format with boxed heap fields
  is an interim regression, not the target. Do not add competing representations
  or runtime switches merely to preserve optionality.
- Reference counting with cyclic collection is selected. The tracing-GC
  timebox ended and that work is closed. Do not reopen nursery/collector work
  from an old plan.
- Rebuild and port all extensions. Hard backward source/binary compatibility
  is not a gate and requires no further approval. Use judgment to make porting
  possible and incremental through owning, fallible bridges, then opaque APIs.
- The method/tracing JIT is a cohesive selected direction with shared optimizing
  IR, recovery and backend. Experiments improve mechanisms inside it. Existing
  Python JIT, interpreter and object patterns may be replaced when they are poor.
- First complete and measure the unified-value register interpreter, with JIT
  explicitly disabled, against matched interim RC and upstream controls.

## How to work

Use useful incremental chunks: design → independent review → implementation →
focused tests → independent code/result review → commit. The coordinator merges
reviewed results and validates the merged tree before claiming integration.
Parallelize work with separate ownership or worktrees. Keep cheap gates early;
reserve costly qualification and quiet timing for coherent candidates. Correctness
and measured performance matter; code size alone is not an arbitrary veto.

Maintain a wiki: entrypoint, process, overall design with IR/interpreter details,
testing/admission gates, and a short current board. Keep about one line per
concluded experiment in the log; detailed evidence belongs to that experiment.
Preserve failures, source identities and unrelated user work. Do not turn
component source review, AST checks or a host control into a runtime pass.

## Recover after restart or compaction

1. Read [README](README.md), [PROCESS](PROCESS.md),
   [the board](issues/README.md) and the linked current experiment records.
2. Inspect actual source/worktree changes, commits, processes and live agents.
   Historical instructions, prepared scripts and queued messages are not active
   work. Do not restart expired work or old agents by assumption.
3. Reconcile the board and any machine monitor with that evidence. Assign new
   bounded work and an independent reviewer, then continue the selected milestone.

At the November 2 coordination reset, previous subagents were stopped and fresh
agents audited documentation and state. Current source identities, actual build
results and assignments live on the board and its experiment pages; do not
recover a live task list from a dated snapshot or old artifact revision.

Loom is a local service and writes are authorized. Keep `goal` aligned with
this page and publish concise linked status; retain prior artifact revisions.
