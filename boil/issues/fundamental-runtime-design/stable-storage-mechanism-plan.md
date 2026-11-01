# First integrated allocator/collector mechanism

2026-11-01; design-only grant 02:33–02:48 UTC. No implementation or native
work occurred; frozen attribution helpers are untouched. This narrows the
[storage hypothesis](stable-storage-hypothesis.md) using the
[Fable GC audit](fable-gc-audit-20261101.md).

**Prototype collector-owned stable spans for exact native coroutine objects
plus embedded frames, retaining ordinary deallocation for every state.**
Measure type/size/live/dead/occupancy in that prototype's debug witnesses;
do not require a separate counter-only image first. This tests allocation,
inventory and storage return together, with no completed-state admission gate.

## Current evidence and minimum prototype measurements

Source predicts 48,557,000 coroutine constructions versus 48,557,697 actual
total deaths in the saved hot window. This nominates a consumer, not a measured
type percentage or savings. Actual classes, live/dead storage and slack remain
unmeasured; the earlier tuple comparison is weaker.

Add debug-only scalar buckets within the ownership implementation:

| Insertion site | Required observation |
| --- | --- |
| `_PyObject_GC_NewVar` / typed allocation, both coroutine constructors | Requested bytes, actual stride/class, span allocations and ordinary-space exclusions; total allocation debt separately |
| Span reservation/publication and inventory before mark clearing | Allocated, initialized, raw-reserved, marked/live and dead counts/bytes by class; never read an unpublished header |
| Existing dead-record append and final `sweep` free loop | Actual candidate, retained/resurrected, ordinary dispatch, retired and quarantined counts/bytes |
| Span commit/release after each major | Occupied slots, reusable holes, partial/empty spans, committed backing, descriptor/index bytes and slack |

Keep requested, class and backing bytes distinct; boundary live counts are
not ages. Label debug padding without inferring release occupancy. No pointer
registry, callbacks, extra collections or per-object clock. Elapsed decides value.

Completed-state eligibility counters serve only a later `gen_dealloc`
shortcut. This prototype preserves constructor prefixes, every frame state,
weakrefs, origins, observers and existing lifecycle obligations.

## Interfaces replaced by the one storage slice

Admit exact coroutines at construction, with embedded frame slots in the same
allocation. Other families and thread-frame chunks retain their spaces.

| Existing boundary | Integrated replacement and retained obligation |
| --- | --- |
| `_PyObject_MallocWithType` → `PyObject_Malloc` → `_PyObject_MiMalloc` | Select the internal coro span class below existing wrappers; take a free slot bit. Keep `gc_count_alloc` before allocation and requested-byte major debt unchanged |
| `new_reference` / object-start map | Preserve the existing initialized-header publication bit; span allocation/reservation is separate. TRACKED/frame state still governs traversal safety |
| `live_lookup` / page/block lookup and shallow ranges | One checked span owner/domain/slot extent before legacy lookup; exact or conservative-interior rules unchanged. Never shallow-scan a whole span |
| `live_walk_heaps` true-block enumeration | Enumerate owned span occupancy and initialized starts directly; retain per-object marking, classification and dead records |
| `sweep_one`, `PyObject_Free`, `_PyGC_FreeDeadBlock` | Preserve lifecycle/deallocator ordering and deferred frees; final storage commit retires slot masks, clears publication and updates accounting without `mi_free(slot)` |

Use the existing `current_object_heap` typed route, with a dedicated private
tag. `gc_count_alloc` can collect before backend allocation: preserve
`collect_pass`'s save/reset/restore across callbacks and queued work; add no
independent unscoped hint. Admit only known default/debug wrapper chains.
Unknown chains keep ordinary storage through their wrappers, so a wrapper's
unrelated raw allocations cannot accidentally enter the cohort.

Use `_PyMem_NoScanMalloc`/`_PyMem_NoScanFree` for backing, below the admitted
default/debug wrappers. Require the attached thread's NOSCAN heap to match
`_mi_thread_id()` before new backing allocation; otherwise decline admission.
No new shared/private heap or allocator hook is introduced. The existing
NOSCAN exclusion prevents legacy GC inventory/shallow scanning of backing;
the domain span index is the sole object inventory. Cross-thread hole reuse
needs no mimalloc heap operation; backing free uses existing `mi_free`.
Detach uses existing NOSCAN heap transfer/abandon handling; descriptors retain
no thread/heap pointer. Verify live backing survives detach before admission.
Header marks/dead records remain authoritative; metadata must not root payloads.
Check provisional 64 KiB spans/≤4 KiB classes before source freeze. Define raw
extent/stride including CPython debug prefix/tail and per-slot ASan redzones/
poisoning: `_PyMem_SizeClass` omits mimalloc padding, which protects backing,
not each slot. Account raw slots separately from backing commit.

Separate storage ownership from object lookup: return NO_SPAN,
OWNED_VALID_STORAGE or OWNED_INVALID. A reserved allocated slot with no
initialized-header bit still needs owner-aware free/realloc without a header
read. A free slot, bad boundary or span tail must never fall through to
mimalloc. Accept client-object and inner debug-raw-base geometry only at their
respective API boundaries; apply publication bits for GC/object lookup.
Cache descriptors plus integer cursors, not direct payload pointers in scanned
thread state.
Admission may stop for wrappers, detach or shutdown; ownership lookup/free/
quarantine must remain active until the last reserved/live/quarantined slot is
gone. Saved allocator forwarding must still reach these ownership dispatches.

Survivors stay put; partial spans reuse holes. Raw reservations, pending
deaths and quarantine prevent empty-span release. Thread exit relinquishes
allocation caches, not domain ownership. Large/unbound allocations keep the
existing route. Concrete required dispatches:

- `gc_block_object` and debug/frame bounds resolve span ownership before
  `_mi_ptr_page`/usable-size; use raw slot base, debug offset and extent.
- `_PyObject_MiRealloc` keeps a fitting slot or allocates/copies compatible
  storage before retiring the old slot; preserve outer object-map transfer
  and failure ownership. Backend allocation must not collect.
- `quarantine_realloc` uses slot bounds, copies and quarantines the old slot.
  `_PyGC_Quarantine`'s normal/OOM paths and quarantine release/unpoison use
  owner dispatch, never `mi_usable_size(slot)`/`mi_free(slot)`. Preserve the
  existing immediate-free-on-quarantine-OOM outcome at the same safe phase.

Reserve metadata before publication; failed collection clears actual visited
MARKED/SHALLOW/DEAD prefixes across both spaces without allocating.

## Work removed, and work still paid

The prototype removes per-slot allocator calls, free-map reconstruction/
true-block inventory callbacks and individual allocator frees. It retains roots/closure,
initialized-object checks, mark/header work, per-death records, lifecycle
graphs, weakrefs/resurrection and ordinary deallocators. Finish every native
referent reader and diagnostic read before reusing or releasing storage.
Retain completed-coro `gen_dealloc`; its shortcut is a separate later cause.

Historical coroutine exclusive sampled CPU was 12.138% inventory/classification,
9.601% sweep/storage-other and 3.261% native-deallocator descendants. These
are opportunity umbrellas, not removable fractions; one allocator leaf was
1.147%. Mutator allocator share is unmeasured; native-deallocator work stays.
Witness avoided callbacks, slot operations, backing releases and remaining records.

## Finite prototype and decision

Propose 90 minutes for one source prototype, a concrete
30-minute ownership/lookup checkpoint, 30-minute independent source review,
then one 45-minute focused fresh debug/O3 feedback batch. Missing owner,
publication, partial-prefix, OOM or quarantine proof stops the slice. Existing
tests must exercise both constructors, suspended/executing/completed states,
origins, escaped frames, native/interior roots, weakrefs/resurrection, opaque
owners, thread detach and mixed partial/large spans. No skip-global-root
ablation, missing barrier or nursery/cadence change is permitted.
At 90 minutes freeze a complete review candidate or retain the patch with
the concrete unclosed boundary; there is no promised completion or extension.

Temporary source ablation routes the cohort through legacy allocation from
birth; it never calls `mi_free` on span slots. Positive allocation/inventory/
retirement witnesses must disappear in that arm while semantics remain valid.
Compare joint ownership without claiming independent allocation/free effects.
Freeze native/config/tool/library
bindings before one subsequent 45-minute elapsed/RSS comparison on coroutines,
generators, comprehensions and Richards, with raw vectors and predeclared noise
and memory rules. No adaptive extra samples or instruction-counter veto.

Report the specified span counters across fixed repeated batches and
absolute/fork/upstream RSS. Reject if lookup/admission or fragmentation cancels whole-workload gains,
even with a shorter free loop. Review and broad qualification precede landing.
These scopes require new grants; they do not
extend the old 04:18 adoption boundary.

Maintain one storage policy with structural eligibility and no permanent
switch. Old-work avoidance still needs complete barriers/opaque-owner rules;
copying, concurrency, boxing and frame-root changes remain separate.
