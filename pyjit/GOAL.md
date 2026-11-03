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
- Mechanize repetitive ports and use cheaper agents for bounded batches under
  reviewed ownership rules. Prioritize a safe runnable baseline; converting
  correct owning object bridges into direct-value operations is later tuning.
- Use Luna for compiler-error analysis, pattern inventories and bulk changes
  under approved rules. Use Sol 6.1 for design and design review. The coordinator
  owns assignments, integration, merged validation and status reporting.

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

The coordinator proactively reports status about every two hours during active
work, and at material milestones or blockers. Check completed agents promptly,
review their outputs and assign the next bounded work; completed assignments
must not remain recorded as running. Keep the last report and next due time in
the machine monitor. A due-time record is not a durable scheduler; do not claim
unattended notifications unless a real scheduling mechanism is configured.
Drive the porting loop continuously: cluster actual build errors into causes,
design reusable solutions with Sol 6.1, fan out approved batches mechanically or
to Luna, review and integrate, then rebuild and repeat. A completed agent task
is a handoff to process, not the end of the overall run.
Give callback-heavy bulk ports explicit typed owner/control-flow recipes and
small function packets. For artifact-only batches, review generator write targets
before root execution and verify that live source and index remain unchanged.
Bind newly used APIs to actual providers; preserve
failed attempts and correct their receipts instead of silently relabeling them.

## Recover after restart or compaction

1. Read [README](README.md), [STATUS](STATUS.md), [ROADMAP](ROADMAP.md),
   [HANDOFF](HANDOFF.md) and [PROCESS](PROCESS.md), then the current stage and
   workstream. Follow the [documentation hierarchy](DOCUMENTATION.md).
2. Inspect actual source/worktree changes, commits, processes and live agents.
   Historical instructions, prepared scripts and queued messages are not active
   work. Do not restart expired work or old agents by assumption.
3. Reconcile STATUS, HANDOFF and any machine monitor with that evidence. Assign new
   bounded work and an independent reviewer, then continue the selected milestone.

The documentation root is pyjit. Current stage outcomes live in ROADMAP,
aggregate state in STATUS, bounded tasks in workstream pages, and exact evidence
in experiment directories. HANDOFF records restart identities and pending gates;
reconcile its dated snapshot before execution. Historical branches, ABI names,
build directories and sealed receipts retain their original identities. Do not
recover a live task list from an old artifact revision.

Loom is a local service and writes are authorized. Keep `goal` aligned with
this page and publish concise linked status; retain prior artifact revisions.
