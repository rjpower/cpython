# Young-space coverage: movement is not the first useful boundary

2026-10-30, source-only. This revises the moving-first recommendation in
`design-reflection.md`; it adds no allocation counts, age measurements or native
runs. Current landed source is `0b15` / integration `aee46`. Allocator feasibility
is being checked separately; no new allocation mechanism is assumed here.

**Recommend one generational collector whose first useful young cohort includes
address-stable exact generators/coroutines and their embedded frames.** Copying
admitted managed layouts can follow within that same collector. A moving
cell/tuple-only first slice proves relocation but does not establish coverage
of the main observed recursive coroutine/generator allocations.

## Concrete source coverage

| Workload | Source establishes | What cell/tuple-only movement misses |
| --- | --- | --- |
| [Coroutines](../../../bench/internal/kernels/coroutines.py), `fibonacci`, `bench_coroutines` | Each recursive `await fibonacci(...)` creates another coroutine; results are Python integers. | The coroutine object and frame storage. With ordinary origin depth0, `make_gen` does not construct origin tuples. No source reason makes cells/tuples the dominant allocation family here. |
| [Generators](../../../bench/internal/kernels/generators.py), `Tree.__iter__`, `setup`, `bench_generators` | The large Tree graph is created in setup. Hot traversal repeatedly invokes a recursive yielding `__iter__`. | Generator objects and embedded frames created by repeated iteration. Moving the prebuilt graph is not the hot allocation issue. |
| [Comprehensions](../../../bench/internal/kernels/comprehensions.py), `_add_widgets`, `_any_knobby` | Hot work builds sort-key tuples, lists and dictionaries, plus `any(w.has_knob for ...)` generators. | Lists/dict tables, generator/frame allocations and native cleanup. Tuples provide genuine coverage, but its byte/count share and survival rate are not measured. Cells are not demonstrated dominant traffic. |

`Objects/genobject.c:1315–1336` allocates generator/coroutine storage through
`PyObject_GC_NewVar(..., slots)`. `gi_iframe` and its owned slots are in that
same allocation, not a separately allocated movable frame
(`:1233,1263,1392–1405`). Origin-depth0 bypasses tuple construction
(`:1362–1379`). A movement policy leaving generators/coroutines in stable
native space therefore leaves these allocations outside its copying nursery.
If stable objects are also old from birth and only major-reclaimed, their
dead enumeration, cleanup and storage return remain major work.

Ordinary thread frames are different: `_PyFrame_PushUnchecked` and
`_PyFrame_PushOverlapping` use thread stack chunks / overlapping call windows
(`Include/internal/pycore_interpframe.h:483–526`). An allocation policy must
not count those frame entries as separate heap objects. `RETURN_GENERATOR`
copies the executing frame into the generator-owned storage
(`Python/bytecodes.c:5404`).

The fresh profiles establish substantial death totals and repeated heap work,
not the age/type histogram needed to promise a nursery hit rate. Source gives
good candidate coverage, not a measured percentage or a speed estimate.

## Minimum useful vertical slice

Use one mark/sweep/minor/major transaction with explicit generation membership.
Allocate newly created exact generators/coroutines as **young but nonmoving**;
young death is eligible for minor reclamation through the existing finalizer,
weakref, native-clear, retention and sweep semantics. Survivors promote in place.
Do not defer all their deaths to a major just because they expose raw pointers.
Allocation-origin admission must cover both common constructors, including
`PyGen_New` / `PyCoro_New`, not reclassify objects as young only once finished.

The same young inventory, remembered-set boundary, major debt and liveness
oracle later support copying cells/tuples. Movement becomes a storage strategy
for eligible young layouts, not another collector or a prerequisite to proving
useful minor coverage. Existing mimalloc may supply stable young allocation;
how membership/enumeration avoids a full old-heap walk is the outstanding
allocator boundary. A full-heap scan followed by a generation test is a useful
oracle, not the claimed performance implementation.

Minimum safety contracts remain substantial but narrower than relocation:

- Old core owners need complete nonallocating old-to-young barriers. Current
  `IR_STORE_*` writes (`Python/ceval_macros.h:533–567`) are ordinary assignments;
  representation uniformity is not a remembered-set proof. Cover constructor
  publication and post-promotion frame/container writes using existing ledgers.
- Permanently scan allocated old opaque/native owners each minor; readonly
  traversal is sufficient for nonmoving targets. Unreachable old owners may
  conservatively retain young children until the next bounded major. An edge
  through an old core child still relies on that core child's store barrier.
  Registry/space removal must precede free; no manual type list or stale owner.
- `gen_traverse` visits non-cleared frames, including executing frames, and
  origin/exception links (`Objects/genobject.c:65–90`). Preserve initialized
  constructor prefixes and current root/SHALLOW/OOM contracts. Roots may retain
  raw addresses because this first young cohort never moves; writable-local
  migration is needed when copying is activated, not before every useful minor.
- Finalizer ordering may traverse old graph connectors, resurrection can cross
  generations, and callbacks can create new edges. Reuse the existing prepared
  transaction with generation-aware candidates; do not infer safety from age
  or drop resource cleanup into an unrelated later cadence.

Done means a real minor reclaims dead recursive gen/coro allocations without
inventorying all old objects, retains promoted suspended/executing frames and
young payloads, preserves weakrefs/finally/escaped-frame/origin behavior, and
recovers correctly from allocation failure. Compare its liveness with an
independent full-trace oracle; a missed-barrier negative must lose the expected
child. Check data/results/errors, bounded RSS/slack and eventual resource
release, including native→old-core→young chains and dead old cycles reclaimed
by a major. Measure admission/survival later rather than fabricate them now.

## Choice and limits

This is a revision of sequencing, not a second nonmoving-GC project. It avoids
making C-alias relocation the gate for all generational value. The cost is
per-object young tracing/deallocation and remembered/native-owner scanning;
it does not deliver bump allocation or bulk corpse reset. Those remain optional
extensions with explicit cleanup/storage contracts.

Cells/tuples are still good first **copying proof** layouts. Treat that proof
as performance-relevant only once a useful share is admitted or expansion is
ready. Conversely, a nonmoving young gen/coro cohort is only a source-backed
coverage candidate until membership/enumeration, barriers and bounded memory
are implemented and measured. Neither sequence promises parity or2×.

## Critical qualification: young deaths can retain almost all their cost

Nonmoving minors do **not** by themselves remove the measured per-death work.
`Python/gc_trace.c:2186–2285` enumerates blocks, classifies each unmarked
object, grows `_PyGCDeadObject` storage and publishes DEAD. `gc_live.c:1349–1457`
then dispatches `tp_dealloc` and later frees every block. Restricting that
pipeline to young objects helps only to the extent that old heap work was
material. If most current candidates are already young deaths, the same
O(young-deaths) pipeline remains, plus barriers and old-native scanning.
The 13–23% sampled mark bucket includes enumeration/classification; it is not
all old-live tracing. The pending captured-stack decomposition, not that
aggregate alone, must select the first performance mechanism.

The **same young space** may therefore need collector-owned bulk storage
discard for eligible completed gen/coro objects to be useful. This does not
require copying them. Source supports a finite eligibility proof, not a blanket
deallocator bypass:

- `gen_finalize:94` is no-op for finished states. But `gen_dealloc:202–232`
  still untracks, clears weakrefs, clears coro origin, may clear the frame,
  requires NULL exception state, drops code/name/qualname fields, and calls
  `PyObject_GC_Del`. Completion alone is not the full proof.
- A strict storage-only cohort can require exact gen/coro, FRAME_CLEARED,
  no escaped frame, NULL exception value, no actual weakrefs/hooks/observers
  and completed lifecycle work. Origin/code/name/qualname are references, not
  separately owned native arrays. In this fork `Py_CLEAR` is NULL assignment
  plus a keepalive barrier (`Include/refcount.h:108–167`); it does not dispatch
  recursive destruction. Once no observer can inspect the dead object, those
  stores need not precede reclaiming its entire storage merely to emulate RC.
- Suspended genexprs are a different cohort: they need the existing
  `_PyGen_CanClearUnreachable` proof and actual no-Python frame cleanup first
  (`genobject.c:550–582`), not a finished-type assumption. Escaped frames can
  take ownership of frame contents (`Python/frame.c:160–188`); retain that
  existing path. Never discard active/finalizable/resource state by calling
  it temporary.
- `_PyGC_FreeDeadBlock` still drops any weakref side list, clears the object
  map, counts a free and returns storage through the allocator
  (`Objects/obmalloc.c:1701–1710`). A batch path must perform equivalent map,
  accounting and reuse transitions and preserve debug/quarantine/custom
  allocator/reftracer behavior, or reject that mode. Finish every journal,
  ordering and diagnostic read before releasing a containing page.

A page/region descriptor or compact bitmap must let the collector prove a
whole reclaimable cohort and its remaining exceptions without constructing
the same record and dispatch sequence for every corpse. Merely replacing the
last individual free with reset leaves classification/journal cost intact.
Partly live pages retain valid objects/metadata; raw uninitialized blocks,
manual frees and outstanding allocator ownership also constrain reset. Stats'
allocator feasibility review owns that boundary; this note adds no allocator
API or per-object side record.

Accordingly the minimum **useful** vertical slice remains conditional: choose
one young-space implementation with age/membership and, if death work dominates,
bulk storage reclamation for the proved common cohort. Copying cells can share
it later. Do not mandate either nonmoving-first or moving-first from layout
convenience. If the captured cost instead shows substantial old-live traversal,
a plain nonmoving minor may earn a first step; if it shows young-death work,
age partition alone has not addressed the budget.
