# Register state and frame layout: round54

Status: **closed**. All five implementations, two revisions and two
compositions are measured and rejected for performance admission. Published
branch `pyjit/round54-golden` and annotated tag
`pyjit-golden-20261112-frame-state` select only the separate three-file cold
public-construction correction at
`61015a9452d9069646fb5fede9ff963896833b89`, with explicit qualification limits.
Final direct measurements establish no general performance lift. Golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254` remains an immutable control.
Active [round55](../call-model55/plan.md) is holistic architecture research and
plan only; review the [proposal](../../design/proposals/register-call-model55.md)
before implementation.

The completed round requested one broad Astra review with five concrete improvements,
followed by Sol implementation lanes, measured selection and a combined golden.
Assess bitmap/tag tracking versus NULL homes, producer/instruction ownership
proofs, fixed frame headers and variable/cold activation storage together.

## Completed work and admission process

1. Independent Sol inventories document every frame field and every consumer
   of register-home validity. Astra researches these choices against selected
   source, upstream and relevant primary literature, then ranks five complete,
   falsifiable options. No implementation before proposal and peer review.
2. Each Sol lane has an isolated golden51-derived worktree, explicit owner and
   callback/GC/observer contracts, bounded scope and an independent peer review.
   Root owns source integration, generation, builds and native execution.
3. Start with focused release/debug checks relevant to each change. Native
   inspection must show the removed work and new bookkeeping. Screen unchanged
   generators, coroutines, DeltaBlue, Richards, comprehensions and logging silent
   against actual golden51 with two balanced pairs; confirm with six.
4. Measure all29 for promising options and combine winners in a separate tree.
   Qualify merged ownership, full diagnostics and direct upstream/prior-golden
   performance at the final integration point. Preserve known debugger/LLTrace
   limits and the narrow GDB pretty-print quarantine. Cold review precedes
   publication. A component win does not prove a combined win.
5. Publish the selected design, report, complete tables, next plan and Loom
   mirrors. Preserve rejected evidence and immutable controls; retire only this
   task's scratch directories after verified archival.

Use one GCC15.2 configuration, CPU15, hash0 and JIT off. Do not overlap quiet
timing/profiling with builds, tests, GDB or bulk cleanup. A NULL-to-bitmap change
must count validity writes on every transfer, not just saved teardown scans.
Do not assume every header pointer is optional or that observer state is cold
without tracing all readers and owner lifetimes.

## Paths

Canonical C=`/home/power/code/cpython/.worktrees/python-jit`.
BASE=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
Selected source `BASE/worktrees/frame54-cold-construction`; native builds
`BASE/frame54-cold-construction-{release,debug}`. Prior golden51 source
`BASE/worktrees/round51-golden` and builds `BASE/round51-golden-{release,debug}`
remain immutable controls. Upstream source
`BASE/worktrees/upstream-3a440b6`; build `BASE/upstream-release-devlibs-3a440b6`.
Packet `pyjit/issues/frame-state54`; evidence `BASE/pyjit-link-wave1/frame-state54`.
The selected [activation design](../../design/activation-transfers.md),
[previous report](../activation53/report.md) and [handoff](../../HANDOFF.md)
define baseline facts and prior negative evidence.

## Closed checkpoint

The [report](report.md), [tables](benchmark-tables.md) and
[qualification](qualification.md) retain the selection and its limits. Both
54,692-test full diagnostics failed; the original profiling finding remains
unresolved despite ten passing comparative subprocesses. Selection is a
bounded correctness checkpoint, not a clean full-suite or performance claim.
All nine experimental branches are published; 27 task-owned directories were
retired after verification of 18 archives containing 3,042 files. The selected
source/builds, golden51 and upstream controls remain. No further runtime work
is authorized by this closed plan. Follow [round55](../call-model55/plan.md)
for the new architecture request.
