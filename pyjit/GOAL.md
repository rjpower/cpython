# Standing goal and recovery

Carry the CPython rebuild through implementation, independent review,
integration, qualification and measured outcomes. The coordinator owns the
whole result, documentation and user updates. This page records standing user
direction; Loom's `goal` artifact mirrors it.

## Decisions to retain

- Golden baseline (2026-11-08): freeze and publish the qualified runtime as
  `pyjit-golden-20261108-runtime` (`0765e9be47f`, functional qualification
  `0267d428808`). Keep a coherent source/documentation snapshot and a concise
  [manifest](issues/golden-20261108/README.md), with current
  [IR](design/register-ir.md) and [interpreter](design/interpreter.md)
  specifications and Mermaid diagrams. Historical boards are archived;
  preserve sealed evidence and unrelated local work. The fresh
  [cold review and next plan](issues/golden-20261108/next-steps.md) supersede
  the previous ordering of experiments. Evaluate independent structural ideas
  before selecting implementation: executable NOPs are not inherent to a
  register machine, and borrowed frame homes are one alternative rather than
  a predetermined next change. The golden tag remains immutable.

- Completed optimization milestone (2026-11-08): the source-backed eight-item V8
  comparison and selected interpreter improvements are integrated, qualified
  and measured; see the [checkpoint report](bench/results/interpreter-parity-20261108.md).
  Continue driving the unchanged real `logging_silent` workload toward upstream
  parity; the current checkpoint does not reach it. A prepared packet measures
  borrowed arguments across frame boundaries and decides their lifetime and
  representation contract before implementation. Preserve universal canonical
  heap values and explicit ownership; do not treat the current independently
  owning frame-home policy as a law of eight-byte values. Complete owner/GC,
  promotion, observer, fallback and consumer proofs under independent review.
  The [active plan](issues/interpreter-parity-20261108/plan.md) retains other
  reviewed alternatives and the accepted spectral tradeoff. Use propose → peer
  review → implement → focused tests → post-review → integrate; whole suites
  and broader performance work belong at coherent checkpoints.

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
- Complete and measure the unified-value register interpreter, with JIT
  explicitly disabled, against matched interim RC and upstream controls. Keep
  the compiler fixed at GCC; do not spend effort on a two-compiler comparison.
- Require correct behavior and trustworthy workload/accounting evidence for
  performance decisions. The full zero-box R27 census is a diagnostic and a
  source of porting priorities, not an admission veto or required final count.
- Prioritize measured throughput impact. Normalize hardware work against
  workload operations, dispatch/semantic-operation counts, call shapes and
  frame activity before attributing costs to bytecodes or PyValue bridges.
- Mechanize repetitive ports and use cheaper agents for bounded batches under
  reviewed ownership rules. Prioritize a safe runnable baseline; converting
  correct owning object bridges into direct-value operations is later tuning.
- Use cheaper worker agents for compiler-error analysis, pattern inventories and
  bulk changes under approved rules. Use design and review agents for designs
  and their review. The coordinator owns assignments, integration, merged
  validation and status reporting. See [PROCESS](PROCESS.md) for model roles.

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
design reusable solutions with design agents, fan out approved batches mechanically or
to workers, review and integrate, then rebuild and repeat. A completed agent task
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
