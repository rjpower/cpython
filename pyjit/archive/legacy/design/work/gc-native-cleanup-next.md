# Native cleanup of unreachable suspended generators

Implementation handoff, 2026-10-30. This record contains source analysis and a
concrete next slice; implementation and native validation have not begun. Start
from selective-death `10d8773815c` (implementation `788695e3`,
corrected behavioral tests `69c34fb1`), retaining the qualified serving products.

The next small cut should avoid Python-finalizer preparation for an entire
collection whose only unfinished finalizers are provably native-only ordinary
generators. Keep the existing slow path for every ambiguous collection. This
needs neither a scheduler nor another object registry.

## Cost target and evidence boundary

In the qualified fork's 32 MiB comprehension profile, disjoint sampled CPU cost
was 13.07% finalizer-closure retention, 9.05% finalizer ordering, and 2.23%
eligibility checks, versus 4.06% queue delivery. These three preparation buckets
total 24.36% of sampled CPU; they are the target, not a predicted speedup. The
profile measures CPU samples, not callback invocation counts. It is from native
`3c6`, not a comprehension profile of selective-death `10d`.

The workload's short-circuiting `any` leaves generator expressions suspended.
They need cleanup, unlike the finished-generator no-ops removed by the first
slice. Current raw profiles, counters, budgets, hardware counts and RSS are in
[the finite13 index](../../../build/checkpoint-i-current-cost-attribution/native-comparison-v1/completion.json).
The clean paired comprehension elapsed ranges overlap; neither that result nor
this plan establishes a gain for the next cut.

## Exact first boundary

Use the existing `gen_close` clear-only proof in `Objects/genobject.c:583–597`:
the next instruction is `RESUME` (including its instrumented form), its original
oparg has `RESUME_OPARG_DEPTH1_MASK`, and `_PyEval_NoToolsForUnwind` is true.
`Python/flowgraph.c:974–984` supplies the depth fact; `Python/ceval.h:293–303`
reads the current local/global monitoring bits without allocating.

For this first implementation require exact `PyGen_Type`, exact ordinary
`FRAME_SUSPENDED`, generator-owned frame storage, and `frame->frame_obj == NULL`.
Exclude yield-from/locked/executing states, async generators, coroutines,
created generators, custom slots, and materialized frame objects. Excluding an
escaped frame is deliberate: `take_ownership` can copy frame contents into a
different object, which is more than deleting edges from the prepared graph.
Keep its existing behavior on the slow path.

Choose the native cohort only after classifying the complete dead set. Fall
back for the whole collection if any of these applies:

- Any actual unfinished finalizer is outside the exact native subset; any
  legacy `tp_del`, destroy hook, registered weakref (including callback-free
  refs and static builtin lists), referent-reading first-pass deallocator, or
  memoryview/managed-buffer readiness work is present in the dead set.
- An old queue is nonempty or a drainer is active. Existing queue roots still
  protect their closures; do not reinterpret them as dead native work.
- GC callbacks or debug flags (including `DEBUG_SAVEALL`), reference tracing,
  Python tracing/profiling,
  or monitoring is active. The local unwind predicate remains mandatory even
  with this conservative observer gate.
- The mark domain is not the supported main-interpreter GIL domain, or the
  collector cannot maintain the no-detach/no-Python interval through commit.

These gates are intentionally broader than the eventual minimum. A type's
mere `tp_finalize` capability does not disqualify an already finished/finalized
object; use the existing actual eligibility facts. Registered weakrefs do
disqualify the pass, because another callback can use a surviving plain ref to
observe a generator. A real finalizer can inspect or resurrect its generator
before the latter's close. Neither eager clearing nor dropping the generator's
queue record preserves that case. The entire unchanged slow path preserves
those closures and all ordering connector objects.

## Preparation, commit, and storage

1. Complete the mark and its dead-record allocation. Inspect every dead record
   without changing any frame, finalization flag, or reference edge. Decide
   whether all required generator cleanup qualifies; otherwise use the old
   preparation and delivery unchanged.
2. Complete every potentially failing operation before cleanup: existing
   batching/retention, weakref/debug handling, and `order_dead_first` preparation
   must either finish or take the existing no-sweep rollback. The conservative
   native cohort normally has none of this slow work. Do not make correctness
   depend on that remaining true as helpers evolve.
3. Revalidate the entire selected set and observer/queue gates before changing
   the first object. From this point to sweep there must be no allocation,
   detachment, callback, or other failure exit. In the supported single-GIL
   domain, the unchanged candidates cannot start running between validation
   and cleanup. A mismatch before commit selects the slow path; it must never
   cause fallback after some frames have been cleared.
4. Perform actual frame cleanup through a small internal entry to the existing
   `gen_clear_frame` sequence: mark the frame cleared, update the last-profiled
   frame, unlink `previous`, clear locals/stack/function and exception state,
   and preserve the executable until normal generator deallocation. Complete
   the generator's once-only finalized bookkeeping only after preparation
   succeeds. This is completed native cleanup, not a no-op-finalizer label.
5. Run the normal deallocator and block-free/accounting sweep. It still clears
   names/code and owns the actual storage release. Do not directly free a
   generator or borrowed frame memory, skip its deallocator, or retain an empty
   frame indefinitely.

`gen_clear_frame` already roots the generator while clearing it. In this fork,
value-slot clearing and `Py_CLEAR`/`Py_DECREF` are liveness operations, not
reference-count-triggered callbacks. Nevertheless the new caller must satisfy
the precise-root checker and prove the complete no-allocation/no-detach path;
it cannot borrow a reference-counted CPython argument unchanged. Before commit,
OOM retains frames intact through `undo_dead` and frees nothing. After commit,
there is no OOM path to pretend to roll back. Do not weaken the existing sticky
work-stack, temporary-MARKED cleanup, or partial-delivery rules.
Commit and sweep inherit the supported native teardown/allocator contract;
this is not a new proof for arbitrary custom RAW allocator hooks. Any native
destroy operation capable of observing the generator belongs in the existing
hook/referent-reading/buffer fallback categories.

## Implementation handoff

| File | Change boundary |
| --- | --- |
| `Objects/genobject.c` | Share the side-effect-free close eligibility predicate with `gen_close`; add an internal native clear entry that preserves `gen_clear_frame` and precise roots. Public `close()` stays unchanged. |
| `Include/internal/pycore_genobject.h` | Declare the two narrow internal operations with explicit preconditions. No public API or type capability flag. |
| `Python/gc_live.c` | Classify the whole pass, retain the global slow fallback, place native commit after fallible preparation, and keep normal sweep/accounting. Reuse the dead array; no separately allocated worklist is needed. |
| `Lib/test/test_gc.py` | Add behavior tests for finalizer inspection/resurrection and a plain weakref exposed by another callback; retain ordering, work-OOM recovery, and buffer readiness controls. |
| `Lib/test/test_generators.py` | Cover complete suspended-frame cleanup, delegation/finally fallback and escaped frame ownership using existing public generator operations. |

Keep `gc_trace.c` classification authoritative for existing categories; the
new distinction depends on current frame/observer state and belongs at the
collection decision point. No scheduler, queue format, cadence, threshold,
allocator, or native-buffer protocol change is included.

## Behavioral checks and implementation checkpoints

Use one finite O1/Py_DEBUG iteration after independent source review, followed
by the existing whole generator/GC modules if focused controls pass. Preserve
these outcomes, rather than asserting preparation counts in integration tests:

- A finalizer sees its generator suspended, can resume/resurrect it, and its
  data remains usable. A callback accessing a plain weakref sees the same
  state. Callback/finalizer delivery and eventual second death stay once-only.
- `finally`, `yield from`, async hooks and monitoring retain their existing
  effects; `test_gen_unwind` and escaped-frame/locals-outlive-generator cases
  retain usable data and correct ownership.
- Repeated ordinary suspended generators holding large plain payloads do not
  leave an accumulating live set. If demonstrating native-path activation
  needs a focused internal invariant, keep it local; weakrefs/finalizers in
  that positive fixture would deliberately force the slow path.
- Existing finalizer-work OOM/next-pass recovery, closed-generator ordering
  connectors, and buffer release/readiness controls remain valid. A narrow
  pre-commit failure control must show intact frames and later recovery if
  implementation introduces any new fallible preparation.

The first source-review checkpoint is the shared native clear body and tool
checks: prove their effect contract unchanged before modifying collection.
The performance checkpoint then measures how often the conservative gate admits
the comprehension cohort and its actual cost reduction, using the existing
bounded comparison. Keep observability and escaped-frame support on the slow
path. Retain a negative result if coverage is insufficient; it does not justify
widening the semantic boundary. These are implementation and measurement tasks,
not additional architecture decisions.
