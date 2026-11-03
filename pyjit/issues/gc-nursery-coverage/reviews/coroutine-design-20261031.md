# Exact-coroutine design review, 2026-10-31

Decision: approve the bounded source experiment after the nursery/shape union
lands. This is design approval, not acceptance of an implementation or a
performance claim. No runtime source was changed and no native program,
benchmark, build or test was run for this review.

## Inputs and scope

- Reviewed `../boil-nursery-shape-qualification` at clean frozen commit
  `e9e10d5b503eaf219797a5db44766eeab6fad4dc`.
- Reviewed `boil/issues/gc-nursery-coverage/design.md`, including the author's
  revisions during review. The approved snapshot has SHA-256
  `eb80298825500321988b2836ef4092e4787a9a109bf40b94c18de840e9694676`.
- Initial issue status SHA-256:
  `1582ef743578e16ad06dfc30dd878309210326917c8ec956f4a906971cef8eeb`.
  During review the coordinator moved status from queued to review; the observed
  replacement has SHA-256
  `71231a8b00da3a7f7f002a5bd81adda3ac0fa5d7c07a60a2847f0becd53fb48a`.
  This review does not assign implementation ownership.
- Read repository guidance, CPython's AI policy, `boil/README.md`,
  `boil/PROCESS.md`, the minimal-nursery status and the unchanged coroutine
  kernel. Source inspection was limited to the selected lifetime, tracing,
  sweep, allocation and test interfaces.
- Historical profile numbers in the proposal are selection evidence supplied
  by the author. This review did not replay the profile or infer coroutine
  allocation counts from sampled execution shares.

## Source findings

The selected object family is useful and sufficiently narrow. The unchanged
kernel creates exact native coroutines at every recursive call. Their frame
storage is part of the same variable-size allocation; no separate frame
allocator or page protocol is needed. `Objects/genobject.c:1315` and `:1387`
are the two allocation sites. Both can call the existing young admission
helper after initializing the traversable prefix and before tracking. Origin
construction follows publication in both public and evaluator paths, so both
origin success and failure must be exercised.

The proposed skip is smaller than a new reclamation mechanism. At
`Objects/genobject.c:202`, a cleared exact coroutine's ordinary deallocator
untracks, clears weakrefs/origin, clears code/name/qualname and calls
`PyObject_GC_Del`. The proposed guards prove the weakref/origin/frame cleanup
unnecessary. `Include/refcount.h:110` makes the remaining releases lifetime
barriers rather than recursive deallocations. Omitting those field stores can
therefore be equivalent once no observer or later referent-reading cleanup can
inspect them.

`Python/gc_live.c:1349` remains the correct dispatch point. Keep the original
DEAD/!SWEPT check, GC collected count, sweep guard, reference-tracer position,
SWEPT publication and error handling. The shortcut must perform the same
untracking and `PyObject_GC_Del` call, without returning storage early.
`Objects/obmalloc.c:1674` counts the deferred `PyObject_Free` call and
`:1701` counts final block return and clears the object map. Bypassing either
would change instrumentation or lifetime behavior. Phase-zero cleanup precedes
the ordinary phase; the candidate must leave that ordering intact.

The strict `d->kinds == _PyGC_DEAD_GC` rule is reachable.
`Python/gc_trace.c:2299` omits FINALIZE for a no-op generator finalizer, and
`Include/internal/pycore_genobject.h:27` recognizes the cleared exact native
coroutine. FINALIZED is unnecessary. Conversely, keep historical FINALIZE or
WEAKREFS classification as a reason to fall back even if preparation retires
the work. Promotion at `Python/gc_trace.c:2229` leaves dead young objects young,
so a dispatch-time young check can observe them after the mark commit.

The author added the necessary partial-constructor distinction during review:
`make_gen` initially reports FRAME_CLEARED before a real generator-owned frame
is installed. FRAME_CLEARED alone is not completion proof. Require the real
frame owner, NULL frame/exception predecessor links, NULL exception value,
NULL attached frame and NULL origin at dispatch. Preserve the partial-prefix
root/traversal protocol.

`Python/ceval.c:1902` and `Python/frame.c:160` show that normal completion
already clears exception state and detaches an escaped frame. A completed
coroutine with a historically escaped but now detached frame need not fall
back; the detached frame's independently owned payload must remain usable.
Likewise, finalization or resurrection can retain an object without any sweep
dispatch. The revised tests correctly distinguish that case from an actual
ordinary deallocation.

## Blocking findings resolved in the design

1. **Separate admission from the shortcut's value.** The original parent vs.
   combined-candidate comparison could accept an unnecessary shortcut whose
   cost was masked by admission. A debug source-negative proved activity but
   did not isolate performance. The revised three release arms freeze parent,
   admission-only and admission-plus-shortcut separately. Both combined and
   incremental shortcut instruction gains now have an explicit 1% gate in
   each round. A combined win cannot justify an increment that loses.
2. **Observe actual work.** A generation-zero assertion proves admission, not
   reclamation or shortcut activity. The revised debug witness records actual
   admissions by constructor, dispatch counts/bytes and final young frees.
   Positive witness
   assertions and two safe source-negative builds separate admission from
   shortcut activity. No missing-root or use-after-free negative is needed.
3. **Do not demand impossible fallback events.** A queued finalizer can prevent
   deallocation entirely on the first collection, and frame escape can detach
   before completion. The revised test expectations follow those actual
   states rather than requiring immediate ordinary dispatch in every case.

There are no remaining blocking design findings in this snapshot.

## Approved witness and implementation review conditions

Approve fixed debug-only counters exposed through `_testinternalcapi`, with
read/reset outside collection. Count admissions only when young admission
actually occurs; distinguish both constructors. Count shortcut and ordinary
exact-coroutine dispatches and their bytes at dispatch, using the existing
dead record for block bytes. Count actual young exact-coroutine frees and
block bytes immediately before `_PyGC_FreeDeadBlock` in the final free loop;
read no freed storage afterward. Counters must compile out of release builds and
must not allocate, execute Python or add an object registry during sweep.
For assertions about young fallback traffic, distinguish young ordinary
dispatch from ordinary dispatch of objects created before enabling admission.
This may use a separate counter or isolated known-age batches.

Admission bytes and reclaimed bytes need explicit units: requested object
size, including embedded slots, is acceptable for constructor feedback but
must not be called allocator block bytes unless it is measured on that basis.
Use `d->block_size` for swept/dead inventory bytes. Preserve existing public
collection/byte accounting and check it alongside, rather than replacing it
with the witness. The exact small counter layout and constructor test helper
remain part of independent implementation review.

Compute pass-wide observer exclusions after callbacks/preparation and before
the first dispatch. Preserve reftracer reporting even though an active tracer
disables the shortcut. GC debug/SAVEALL and verifier/quarantine must take
their working path. There is no coroutine watcher hook to replace; watched
referents still use the existing retained-closure/queued-hook transaction.
The author must prove any observer context left enabled equivalent; copying
the wider native suspended-generator cleanup machinery without need would
expand this experiment.

Cover reentrant collection/mode changes with the existing collection guard and
check honest collected/bytes statistics. Test an actual `PyCoro_New` call,
not just two Python expressions that both reach `_Py_MakeCoro`. Test normal
return and exceptional completion, old-owner survival, resume/throw/close,
weakref callback/finalizer ordering, escaped frame contents, origin metadata,
and OOM before inventory commit. Retain the first failure and stop for a
bounded decision rather than silently widening machinery or skipping a leaf.

## Cost decision and optional refinements

The revised one-day budget, two 90-minute checkpoints and 60-minute quiet
comparison cap are adequate to reject a small-cost hypothesis quickly.
Thirty primary processes across the three arms, unchanged fixed work and
32 MiB budget, plus bounded generators/Richards and default-disabled controls,
answer the local question. Keep automatic collections major-only and do not
insert timed minors to manufacture a nursery benefit.

The extra promotion census can erase the small expected saving; this is a
reason to measure and reject, not to change collection policy. Record
promotion/collection evidence in separate debug feedback if release products
lack such counters; total matched release instructions and elapsed time must
include its cost. Do not add release hot-path instrumentation merely to obtain
a promotion sub-timer.

Optional: state the rotating arm order and control sample counts in the
measurement manifest before the quiet slot. Keep the declared deadline and
reject an inconclusive result; another family, broader old-work reduction or
an admission-only landing needs its own decision.
