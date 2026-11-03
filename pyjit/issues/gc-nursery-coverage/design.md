# Exact native coroutines: young coverage and completed-object death work

Status update, 2026-10-31: **superseded as an implementation grant; needs
rebasing after [nursery adoption](../gc-nursery-adoption/status.md).** The user
has decided to keep and improve the nursery as the runtime architecture.
Global nursery enable/disable controls are being removed. The original
proposal and review below remain historical evidence; their opt-in and
automatic-major-only assumptions are not the next runtime contract. Future
comparisons may use temporary source ablations, never permanent runtime flags.

Planning proposal, 2026-10-31. Source inspected at frozen union
`e9e10d5b503eaf219797a5db44766eeab6fad4dc` in
`../boil-nursery-shape-qualification`. No implementation, native runs or new
measurements accompany this proposal. Implementation waits for that union's
landing decision and independent review of this design.

## Selected family and cost hypothesis

Choose exact `PyCoro_Type` objects, including their embedded frame allocation.
The hot recursive `await fibonacci(...)` workload creates this family at each
call. Ordinary generators, asynchronous generators, await wrappers and thread
frame chunks are outside this experiment. Both native-coroutine constructors
must participate; age is assigned at construction, never retroactively at
completion.

Hypothesis: admitting these objects to the existing stable-address young
cohort makes actual recursive coroutine allocations minor-reclaimable, while
a narrowly guarded completed-object path removes enough redundant deallocator
work to offset its admission and promotion costs. Keep the nursery opt-in,
automatic collections major-only and all-old minor scanning unchanged.

This is a deliberately small, falsifiable cost hypothesis. The saved coroutine
profile at cleanup source `0b15b4b` contains 301 collections, 48,557,697
GC-capable deaths and 10,876,836,400 freed bytes during an 8.393-second hot
window. Its exclusive sampled CPU shares are:

| Work | Share |
| --- | ---: |
| Allocated-object inventory/dead classification | 12.138% |
| Roots and transitive closure | 10.749% |
| Sweep dispatch/storage and other sweep work | 9.601% |
| Native deallocator subtree below sweep | 3.261% |

The profile has 54 samples / 271,356,750 period-ns with the explicit
`gen_dealloc;...;sweep_one;sweep;collect` ancestry. These are sampled execution
costs at the historical source, not allocation-type counts or a young-object
histogram. They support examining this path, not attributing all deaths to
coroutines. The unchanged recursive source and origin-depth-zero constructor
explain why cells/tuples alone miss this allocation family.

The candidate keeps per-object inventory, dead records, storage return and
the promotion walk. Therefore it does not target the whole 38.687% GC timer
share. In particular, setting `young_pending` on every coroutine burst can
cause an extra full-heap promotion census on automatic majors. That added
work may exceed the small teardown saving. Do not alter collection policy or
raise heap budgets to hide a loss.

Evidence: `build/gc-post-cleanup-performance-v2/phase-analysis/analysis.json`,
`mark-subdivision.json` and `fork-32-coroutines.perf-script.txt`; raw window
`build/gc-post-cleanup-performance-v2/profiles32/fork-32-coroutines/stdout`;
`boil/bench/internal/kernels/coroutines.py`; prior source-only coverage review
`build/gc-post-cleanup-performance-v2/young-coverage-review.md`. These locators
refer to primary-checkout artifacts. They are selection evidence, not evidence
that this candidate works or runs faster.

## Minimum source change

1. In `Objects/genobject.c`, call `_PyGC_YoungAllocated` for exact
   `PyCoro_Type` in both `make_gen` and `gen_new_with_qualname`, after their
   traverse-safe prefixes are initialized and immediately before tracking.
   This covers `_Py_MakeCoro` and `PyCoro_New`, including origin construction
   that can collect after publication. Update the cohort comment in
   `Include/internal/pycore_gc.h`. Preserve the existing origin/constructor
   failure paths and partial-constructor protection.
2. In `Python/gc_live.c`, consider a completed-coro shortcut at the existing
   `sweep_one` dispatch. Admit only dead, unswept, young, exact native
   coroutines with `FRAME_CLEARED`, no weakref side list, no remaining
   exception state, no attached escaped frame and no origin metadata. Require
   `d->kinds == _PyGC_DEAD_GC`; retain the ordinary path for any record originally
   classified with weakref/finalizer work, even if preparation retires it.
   Require generator-owned frame storage and NULL frame/exception predecessor
   links, excluding the `make_gen` partial prefix that temporarily uses
   `FRAME_CLEARED` before `RETURN_GENERATOR` installs a real frame. Also exclude
   reftracer, GC debug/SAVEALL, verifier/quarantine and any observer context
   for which independent review cannot prove the ordinary path equivalent.
   Compute and freeze pass-wide exclusions after callbacks/preparation and
   immediately before the first deallocator; avoid a repeated global observer
   walk per corpse. A failed predicate executes ordinary `tp_dealloc` if the
   object still reaches dispatch.
3. Keep the existing sweep state, collected accounting, untracking, SWEPT
   publication and deferred `PyObject_GC_Del` call. That deferred call preserves
   the existing `PyObject_Free` instrumentation and sweep guard. Only omit
   the indirect `gen_dealloc` dispatch and clearing of the proven dead
   object's code/name/qualname fields. `_PyGC_FreeDeadBlock` still clears the
   object map, handles allocator accounting and invokes the same allocator
   once in the existing final free loop. Do not free a block early.

The proof relies on existing tracing-fork `Py_CLEAR`/DECREF semantics: those
remaining field drops do not recursively destroy their referents. Existing
referent-reading deallocators run in phase zero before this ordinary phase.
Queued hooks, weakrefs, resurrection retention and finalizer ordering retain
their current transaction. An object's completed state alone is insufficient:
all listed admission conditions must hold at dispatch, after preparation.

The GC-only kind test is reachable for ordinary completed native coroutines:
`gc_trace.c:2299–2306` excludes `_PyGen_FinalizerIsNoop` from FINALIZE
classification. Exact `PyCoro_Type` uses `gen_finalize`, as does `PyGen_Type`,
so the helper returns true at `FRAME_CLEARED`. Do not require FINALIZED for
this cohort: its finalizer is already a no-op. Conversely, do not erase stale
FINALIZE/WEAKREFS kinds to expand eligibility; use the ordinary path instead.

`clear_gen_frame` already clears exception state and detaches escaped frame
ownership before a completed object returns from execution. The proposal does
not bypass that work. A historically escaped frame may already have detached:
verify its independent payload remains valid rather than forcing fallback
solely because it once escaped. Never-created, suspended, executing,
origin-tracked, weakly referenced or retained objects continue through their
existing paths. Retained objects need not dispatch any deallocator yet.
No mimalloc/page-transfer, root traversal, barrier, object-map or storage API
change belongs here. If the proof needs one, stop and re-scope.

Tests belong in `Lib/test/test_gc.py`. A small debug-only witness in
`Python/gc_live.c`, declared through the existing internal GC header and
exposed in `Modules/_testinternalcapi.c`, must count actual young admissions
separately for the two constructors, actual freed young coroutines and bytes
in the final free loop, plus shortcut and ordinary coro dispatch counts/bytes.
Use `d->block_size` for death/dispatch bytes. Optional constructor requested
bytes must be labeled as requested object bytes, not allocator block bytes;
do not subtract those unlike units to infer survival. Count admissions only
when the young bit is actually assigned. Read/reset the witness outside
collection. Compile its counters out of release builds; add no permanent
runtime option or object-age registry. The design review must approve this
exact witness before implementation so success cannot mean an unobserved
inactive branch.

## Useful behavior and controls

- Prove both constructor paths enter generation zero, survive one minor with
  correct data, promote in place and remain usable across resume/throw/close.
  Exercise collection during origin construction and its failure path. Use
  existing aux-aware child recipes and explicitly assert verifier/stress off.
- Build batches of completed recursive coroutines without weakrefs or origin
  metadata. After dropping the batch and collecting, require actual shortcut
  dispatches and honest collection/byte accounting; an age-list assertion
  alone does not prove reclamation. Check normal results and errors.
- Retain a coroutine through an old owner, promote it, then resume it so its
  frame references newly allocated young cells/tuples. Verify the payload
  survives minor collection and old cycles disappear after a major. Include
  executing and escaped frames; no writable-alias or new barrier is assumed.
- Exercise unawaited warnings, suspended `finally`, weakref callbacks,
  coroutine origins, escaped-frame inspection and reftracer/GC-debug contexts.
  Assert ordinary dispatch when an ineligible object actually reaches sweep,
  and no dispatch when queued finalization or resurrection retains it. Check
  deferred effects and eventual cleanup in subsequent collections rather
  than requiring an immediate fallback for a retained corpse. Creation before
  enabling the experiment stays old; disabling then major-promoting works.
- Disable admission in one source-negative build: the same positive
  generation-zero assertion must fail. Disable only the shortcut in another
  source-negative build: results and reclamation still pass, but the same
  positive dispatch assertion must fail. These are safe witness controls,
  not missing-root or use-after-free experiments. Retain first failures.
- Cover OOM before the dead inventory commits and ordinary allocator/debug
  configurations. If those contexts cannot cheaply support the shortcut,
  reject them explicitly and verify ordinary-path behavior. Do not weaken
  existing qualification or silently skip a required leaf test.

## Finite execution and decision

After landing and design approval, allow at most one working day, with a
90-minute source/independent-review checkpoint and a further 90-minute first
debug-feedback checkpoint once a native slot is available. Stop on the first
unexpected failure. Scheduling waits do not authorize another experiment or
a silent deadline extension. Source changes outside the listed family/files
require a new bounded decision.

Only after useful behavior and witness controls pass, freeze three release
arms with the same compiler/options/products and obtain one quiet slot:
the landed parent, admission only, and admission plus shortcut. The admission
arm differs from the full candidate only by disabling the shortcut; freeze
that exact diff. Use the unchanged coroutine kernel, fixed work and fixed
32 MiB heap budget. Enable the existing nursery knob before setup in all arms;
do not insert timed minor collections or change automatic-major policy.
Record actual cohort deaths/bytes in separate debug feedback, then elapsed,
instructions, peak RSS and collection/promotion work in matched release
processes. Keep collection instrumentation out of the release hot path.

Predeclare two rotating-order rounds of five independent process samples per
arm for coroutines: 30 primary timing processes total, with instructions and
elapsed captured together. Limit this quiet comparison, including controls,
to 60 minutes. Use generators and Richards as bounded unaffected controls,
and compare parent/full-candidate coroutine/default-knob-off for admission
overhead. Reuse the existing finite driver, identities and cleanup guards.
No 29-workload campaign is needed before this local cost question is answered.

Accept the selected experiment only if behavior and independent source review
pass, both rounds reduce full-candidate coroutine instruction medians by at
least 1% against the parent, elapsed medians improve in both rounds, and the
shortcut separately reduces instruction medians by at least 1% against
admission only without an elapsed regression in either round. Neither controls
nor default-disabled coroutines may regress by more than 1% in median
instructions or show a repeatable elapsed regression. Peak RSS must remain
within 5% of the matched parent and repeated batch/major cleanup must return
retained object counts to a stable
bound. Report raw samples and descriptive spreads; two rounds are not a
confidence interval.

Reject if the branch stays inactive, guard/promotion cost cancels the saving,
memory accumulates, a lifecycle contract needs wider machinery, or the finite
comparison is inconclusive. A package win cannot justify a shortcut that loses
against admission only. A behavior-only coverage result may inform a separately
approved next experiment; it does not satisfy this combined cost hypothesis
or automatically authorize landing the unhelpful shortcut. Broad old-work
reduction, page discard and other object families remain separate issues.
