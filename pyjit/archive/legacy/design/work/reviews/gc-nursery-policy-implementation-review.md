# Nursery implementation and allocation policy

Source/design review, 2026-10-30. No runtime edit, build, native run or timing.
Supported scope is Linux, the main interpreter, ordinary threads and the GIL.
The qualified full collector remains the correctness reference. This proposal
does not reopen repaired S3 work or deferred subinterpreter machinery.

## Recommendation

If profiling establishes material allocator/reclamation cost after the selective
death slice, the first nursery candidate is a **nonrelocating allocation-region
experiment**, keeping full-heap collection. Give GC-owned storage-only core layouts bump allocation,
reset wholly dead regions, and keep surviving pages at their existing addresses.
This tests allocation and bulk storage recycling without first implementing a
remembered set, handles or relocation. It is an allocation/reclamation change,
not yet a minor collector. There is no promise that its page survival or workload
coverage will make it profitable. The minimal experiment still enumerates and
classifies dead objects and builds their records; page reset does not remove
that cost. Skipping classification would require a separate region-descriptor/
bitmap batch design preserving graph membership and resurrection, not an
unadvertised consequence of bump allocation.

The intended later architecture remains a common movable nursery plus stable
native/large-object storage. Choose between evacuating survivors and retaining
pages using measured page survival, retained slack and pinned bytes. Nonmoving
generations are an alternative after the common barrier/root contracts, not a
mandatory intermediate product. Do not build both collectors just to complete
a historical plan.

Current GC consumes47%/54%/30% of three slow hot windows; mark/root closure is
8%/4%/11% of wall time, already included. Reducing old tracing alone cannot remove
their death processing. The selective-death patch is separate and must retain
its own correctness/performance decision.

## What can be reclaimed without per-object teardown?

Use an explicit **internal storage-only layout contract**, covering allocation,
initialization, outgoing references and destruction. It means that after normal
reachability/lifetime processing the object's remaining teardown only changes
collector metadata and returns its own storage. It does not mean pointer-free,
untracked, immutable or `tp_traverse == NULL`. Exact layout admission must not
silently apply to C subtypes with different cleanup.

Two concrete first layouts are exact cells and nonempty exact tuples:

| Current source | Proof boundary for the region experiment |
| --- | --- |
| `Objects/cellobject.c:PyCell_New`, `cell_dealloc` | One embedded PyValue. Current dealloc only untracks and calls GC_Del. Initialize its field before any collecting/publication boundary. |
| `Objects/tupleobject.c:tuple_alloc`, `PyTuple_New`, `tuple_dealloc` | Inline PyValue array; current dealloc only untracks and calls tp_free. Keep the immortal empty tuple outside regions. Cover every tuple_alloc caller: some return uninitialized items, whereas PyTuple_New explicitly writes NULLs. Use safe initialization/prefix rules before publishing the object-start record. |

These are illustrative first proof layouts, not a claim that tuple/cell traffic
dominates the measured deficit. Lists separately free `ob_item`; generators clear frame,
exception and origin state and have finalizers. They are **not** storage-only
merely because reference counting disappeared. Leave their current cleanup intact
until a separate, complete layout proof removes unnecessary work. The same applies
to buffers, watchers, weakref entries and native resources.

Finished generator/coroutine traffic is the stronger measured target, but its
admission needs actual frame state, origin/exception cleanup, profiling links
and weakref/finalizer eligibility, not a type-only exemption. Compare a proven
GC-owned teardown fast path within existing mimalloc before undertaking a new
allocator backend merely to accelerate that traffic.

Ref-tracer DESTROY notifications, verifier quarantine/poisoning and debug allocator
semantics remain observable. An active observer may require a per-object walk;
that does not justify skipping notifications. Bulk reset occurs only after all
required work and metadata retirement, with no borrowed dead-list pointer left.

## Minimum region implementation

Use page-segmented regions with a bump cursor and allocation-time object records.
Record exact starts/sizes, initialized state, owning interpreter and live/pin
state outside conservatively scanned object storage. Reserve metadata before
publication. Under the GIL one shared region allocator is sufficient initially;
thread-local allocation buffers are not a prerequisite.

Source interfaces to close together:

- `Include/internal/pycore_object_alloc.h:_PyObject_MallocWithType` supplies the
  exact layout. Keep generic/unknown/large/native allocations in mimalloc.
- `Objects/object.c:new_reference` publishes object-start identity before CREATE;
  `Objects/obmalloc.c` owns charging, free/reuse, realloc and dead-block recycling.
  Region identity must survive initialization and clear before address reuse.
  `_PyStolenTuple_Free` and explicit tuple resize/free paths also need region-aware
  handling; a dead hole can wait for page recycling, but must cease to be an object.
- `Python/gc_trace.c:live_lookup`, `live_walk_heaps`, page preparation and snapshot
  enumeration currently depend on mimalloc segment/tag/block metadata. Add an
  explicit space-aware lookup/enumeration boundary; do not forge mi_page_t data
  or make region objects invisible to conservative interior-pointer lookup.
  Migrate the direct segment-based ownership query as well.
- `Python/gc_live.c:sweep` separates lifetime processing from block return.
  Keep complete graph connectors, ordered teardown and rollback; only proven
  storage-only dead records may use the bulk path. Failed preparation resets
  nothing. Region pages containing live objects stay mapped and registered.

Initially the existing full mark visits both spaces, so **no new generational
barrier is required**. Any page with survivors is retained/promoted in place and
remains fully traced on subsequent full collections. Partly live pages can keep
holes until completely dead; do not immediately add an intra-page allocator.
Measure live bytes versus retained page bytes. If sparse survivors retain too
much memory, evacuation or size-class reuse becomes an evidence-based next choice.

Done: ordinary tuple/cell results and errors match; cyclic graphs and raw/interior
roots survive collections; freed addresses lose their object identity; allocation
and metadata OOM retain a valid heap; dead regions actually return/reuse pages;
retained slack, requested bytes, allocation/death coverage and per-object teardown
counts are reported, with elapsed cost and peak RSS on fixed work.
Test one survivor per region deliberately. Reject a fast allocation result that
merely accumulates partially live pages. Stop the region route if its live-data
retention exceeds the declared process-memory allowance or its RSS/slack cost
outweighs the measured shared CPU benefit; no object-count quota decides that.

## Common movable nursery and stable pool

If page retention is the limiting cost, evacuate survivors to nonmoving old space
after one minor. Large and explicitly native/address-stable layouts allocate
there directly. This avoids whole-heap compaction. Region allocation and the
storage-only proof above are reusable; shipping nonmoving minors first is optional.

**Layout eligibility and pointer escape are separate.** A tuple may be relocatable
in layout but have an outstanding raw C pointer. `PyValue_ObjectBorrow` returns
the actual pointer; `_PyValueSpan_AppendObject` often roots a duplicate while the
original C local remains in use. Updating the duplicate alone is unsafe.

Use two explicit root modes:

1. Reloadable slots/handles: update their PyValue/object pointer during evacuation;
   callers reload after any collecting call. Handler temporaries and live C locals
   must obey the same rule. Existing precise-frame classification is not a writable
   location map or a reload proof.
2. Address-stable roots/borrows: pin their target before moving anything. Existing
   raw spans and ambiguous C/PyMem/data/interior roots initially have this meaning.
   A known no-safepoint borrow needs no cross-collection pin. Unknown pointer words
   are never rewritten.

Pin **in place**: an already exposed pointer cannot be moved into a pinned pool.
Keep its containing page(s), evacuate other live objects, and retire those pages
into stable space by changing ownership metadata, not addresses. Track trapped
slack and release a retired page when its last object dies. Known unusual objects
allocated directly in stable space avoid this cost. This is more machinery than
the nonrelocating region experiment; measure whether it earns its place.
Stable address is not promotion evidence: partially initialized/SHALLOW objects
remain young until their fields are safely traversable. Commit age/remembered
state before callback reentry and preserve subsequent writes and failed epochs.

For the first moving slice, publish an address identity (`id`, default pointer
hash) only after pinning the object, before an allocating result conversion.
Likewise pin before exposing a stable buffer address or registering an address-keyed
weakref/native association. This preserves current identity and registry contracts
without first adding a global identity-handle table. It is a declared policy with
measured pinning cost, not an assumption that PyValue already supplies stable IDs.

`_PyGC_TraceTraverse` and `visitproc` currently pass child values, not field addresses;
shape traversal similarly calls `_Py_VISIT_VALUE`. Add update-capable layout and
root visitors for the admitted layouts. Existing nonmoving native traversal remains
valid, but any reference it cannot update pins its nursery target. Do not announce
general moving support until every possible incoming edge is either updateable or
covered by this explicit pin contract, including queue payloads, saved exceptions,
weakrefs and running/suspended frame fields. A small rebuilt ports set should test
borrowed pointers, external tables and buffer leases early.

Reserve destination storage, forwarding/object-map metadata and work queues before
publishing relocation. Copy preparation failure leaves the original graph intact;
no partly rewritten graph may fall back to an old snapshot. Specify the commit
transaction before copying becomes active. No Python callback runs in relocation.

Done: aliases and `is` agree before/after relocation; published id/hash stay stable;
interior views and retained native pointers remain valid; updateable handles really
observe changed addresses; omitted updates fail a debug relocation invariant before
reuse. Include constructor failure and promotion OOM. Measure copied bytes, surviving
objects, pinned pages/slack and nursery fast-path frequency, not just a throughput
score. Whole-heap movement is outside this architecture.

## Minimum barriers and minor/major semantics

A real minor stops recursively tracing old objects. Then every old-to-young
publisher in the barriered space must remember its logical owner after allocating
preparation and before the store. This includes specialized list/attribute stores,
dict/set tables, tuple construction after promotion, cells and types/shapes.
Native-owned fields can instead use the declared permanently scanned space below.
Owner remembering can cover a noncollecting bulk batch;
there is no read barrier or concurrent-marking barrier in this STW design.

Running frames/data-stack storage are scanned each minor. At suspension, return
and escaped-frame ownership transfer remember the actual heap owner before it
leaves that root domain. Pre-link throw/send windows and constructor collection
points are explicit ledger cases. Opaque C roots do not cover mutations in old
heap objects. Pinning an object also does not remember its later young children.

The existing owner/store ledgers remain the finite checklist. During development
an independently full-traced graph checks young reachability before reclamation.
Do not silently assign missing barriers to a slow path: space membership and its
scan contract must be explicit, including constructor state and native APIs.
The allocation-region experiment can remain useful without enabling any minors.

### Declared stable-native space: a sound smaller first minor

Permanently remember every allocated, safely traversable owner in an explicit
stable-native/opaque space, and scan its strong outgoing edges every minor.
Core PyValue containers use ordinary owner barriers. This is a sound, deliberate
revision to note11's all-owner-barrier starting policy; it does not require every
native-owned field to acquire a barrier before a common-core minor can run.
Pin a young target reached through the current value-only native visitor; a
writable native visitor can instead update that slot. Use the complete existing
`_PyGC_TraceTraverse` contract, including type/managed attribute edges and external
tables, not only `tp_traverse` or `HAVE_GC`.

The reachability proof is finite: native→young edges are scanned; native→old-core
→young edges are closed by the core owner's remembered state; native→native
chains are covered because every old native owner is scanned. A dead old native
owner can falsely retain young graphs until a major. This is permitted minor
retention, charged to survival/pinning/RSS and bounded by the major-debt policy.
At a major, membership is **not a strong root**: trace ordinary roots, discover
dead native owners normally, and preserve selected cleanup/resurrection semantics.

Required boundaries are narrower than a complete native barrier migration:

- No-GIL threads may write raw buffer bytes, but strong Python-field publication
  must remain serialized by the GIL or an explicit collector synchronization
  contract. A detached native writer racing the scan is not made safe by space
  membership. This proposal makes no new free-threaded mutation guarantee.
- Construction must provide a safe prefix/complete-object transition before typed
  traversal. Stable/old allocation does not authorize calling `tp_traverse` on
  uninitialized fields. Keep the current construction/raw-root protection until
  that transition, and register before any later collecting publication.
- Native callers that directly mutate **core** storage still need the core store
  contract. For example writing `_PyList_ITEMS(old_list)[i] = young` bypasses a
  list barrier even when the caller owns a permanently remembered native object.
  Migrate such writes to barriered setters/macros; pinning the list does not fix it.
- Permanent membership is nonowning, unscanned metadata. Prefer the authoritative
  space/object index over a second lifecycle registry. Remove it before actual
  free/address reuse, transfer it on successful realloc, preserve it on failure,
  and retain it across thread exit/abandoned heaps. Never prune on provisional
  DEAD alone: failed collections and retained callbacks can keep the object alive.
- Truly opaque hidden edges need the existing full-collector trace/root contract.
  Permanently scanning a type with missing traversal cannot recover pointers it
  never reports. Raw nonobject root storage remains an independently scanned class.

Done controls: old native→young, native→old-core→young after core mutation, two
native owners in a cycle, an unreachable native owner retaining a young graph
until a major then releasing it, safe construction across collection, thread
exit, manual free/realloc/address reuse, and a copied omitted core barrier.
For copying, separately prove a native raw pointer stays valid through a minor
because its target was pinned. Report scanned native fields and induced pinned
pages alongside common-core throughput. If that cohort dominates, port measured
native families to writable visitors/barriers rather than adding hidden exceptions.

Minors use the existing weakref/finalizer/resurrection/readiness transaction for
young candidates; majors discover old and mixed-generation garbage. Selected
resource owners and their closure survive until safe delivery/consumption. Rare
cleanup is not postponed past freeing an object. Keep actual weakref callback
versus callback-free clearing order, not the older simplified PEP442 sequence.
Explicit full collection remains a major; changing gc API/stress reporting must
be declared. Test old cycles under zero promotion, raw native mutation, resumed
old generators, delayed buffer release, resurrection and missing-barrier negatives.

## Allocation policy: compare the same quantities

| Runtime/policy | Actual quantity and useful lesson |
| --- | --- |
| Current fork | `A_gross >= max(32MiB, L_objects)` after a collection. A_gross includes requested object/PyMem allocations and full realloc requests; frees do not subtract. L_objects excludes retained external payload/RSS. This is new-allocation allowance, not total heap or nursery capacity. |
| [PyPy](https://doc.pypy.org/gc_info.html#environment-variables) | Nursery exhaustion triggers a minor; documented default capacity is half last-level cache, with4M when unknown/too small. Major factor defaults1.82 times previous major live memory; threshold growth is capped by1.4. Separate small-space capacity from old-heap scheduling. These constants are not our32MiB allowance. |
| [Go](https://go.dev/doc/gc-guide#GOGC) | Target total heap is `L + (L + roots) * GOGC/100`; default100 gives allowance L+roots. Minimum total target4MiB. Its soft memory limit includes broader runtime memory, with a CPU safeguard against thrashing. Its model emphasizes live scanning and omits sweep cost, so its CPU scaling prediction does not transfer to our death-heavy windows. |
| [BDWGC](https://www.hboehm.info/gc/gcdescr.html) | At an allocation slow path, allocation-since-collection versus heap-size/free-space-divisor helps choose collection or expansion; root size/fragmentation adjust the heuristic. Sweeping/reuse is integrated with allocation. This is neither a fixed nursery size nor the fork's exact formula. |

Recommended first policy is deliberately simple:

- Keep a fixed, explicit young-region capacity during the experiment, selected
  from cache size and a declared process memory allowance. Record it; do not derive
  it from the32MiB major-allocation floor or add an adaptive controller first.
- Keep separate gross allocation-since-major debt and promotion/old-space pressure.
  A finite major debt allowance applies even with zero promotion. Retained region
  slack counts toward space pressure; pinned promotion must not disappear from it.
- A declared soft managed-memory target must leave headroom for external/native
  allocations and evacuation reserve. Near it, major-collect before expanding.
  If a completed major makes no useful room, do not spin identical collections:
  use bounded emergency headroom or fail a fallible allocation with MemoryError.
  This is not a hard RSS promise for unaccounted malloc/exporter storage.
- Report GC CPU, pauses and resource latency beside memory. Do not implement a
  pacer from only these four profiles or copy Go's numeric CPU limit. Choose any
  default only after measured headroom/CPU trade-offs, including winning controls.

The **32/64MiB comparison is now complete** in the architecture plan's finite13
batch, using identical qualified products. The actual experiment set the floor
before loading/setup/warmup with no explicit collection in either arm; it did
not use an initial full collection. The setter establishes an initial allowance;
after collection the normal maximum of floor and live-object bytes applies.
The measured instruction savings are3.24%/1.66%/6.98% for the three slow rows,
with peak RSS increases52.09%/72.62%/84.41%. Keep32MiB while reducing work.
The complete results and scope limits are in the revised architecture plan.

Implementation handoffs should be the finite contracts above: storage-only
regions/full mark, then a page-survival decision; barriers/young oracle for minors;
reloadable/pinned roots plus relocation transaction if copying is selected. Each
ends with data/error/liveness and memory evidence. No stage requires a new general
runner, whole-heap movement, or an exhaustive ports rewrite before its bounded
layout experiment can answer a useful question.

## Coordinator plan review

The rewritten architecture plan passes the nursery/policy source review with
these explicit contracts: pin discovery completes before any evacuation, and
surviving aliases reload/recompute after safepoints (derived interior pointers
pin an owner or are recomputed). Updating only a duplicate root or a base local
is insufficient. The8MiB capacity is an experimental half-LLC choice on this
host, separate from the32MiB gross-allocation allowance. No native validation
of the proposed nursery has run.
