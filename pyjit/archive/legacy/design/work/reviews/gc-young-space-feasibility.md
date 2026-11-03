# Nonmoving young mimalloc minor: bounded source feasibility

**A useful nonmoving minor can reuse this collector, without another identity registry or tracing engine. It is a credible first generational implementation, not a free consequence of mimalloc heaps.** It avoids relocation's largest additional proof: updating every live alias. It still needs the same age boundary, remembered edges, safe publication, native-owner scanning and major policy as copying. Do not require both complete implementations in sequence; choose one age-domain consumer after the concrete transfer interface is reviewed.

This audit leaves the completed performance review CLEAR and changes no runtime. The fresh profiles support young-death work, but do not identify the allocation-family contribution precisely enough to promise a cell/tuple-only speedup. Generator/coroutine bodies use typed variable GC allocation (`Objects/genobject.c`, `make_gen`); nonmoving collection could admit them using existing traversal without first making their borrowed aliases writable. Active frame storage also includes thread-owned storage, so routing generator objects is not equivalent to routing every frame allocation.

## What the vendored allocator actually supplies

`mi_heap_collect()` (`Objects/mimalloc/heap.c:131–201`) drains delayed frees, retired/empty pages and allocator caches. It does not discover Python garbage. `mi_heap_destroy()` is unsuitable for survivor pages and is not a Python lifetime phase.

`mi_heap_delete()` (`heap.c:491–508`) preserves allocated blocks, but transfers pages **only to its thread's backing heap**, and only when tag and arena match. Otherwise it abandons them. CPython makes MEM/tag0 the backing heap and embeds OBJECT/tag1 and GC/tag2 heaps in each thread (`Python/pystate.c:2928–2951`; `Include/internal/pycore_mimalloc.h:12–18,75–81`). Therefore deleting a young object heap is not an existing operation for promoting survivors into CPython's old object heap. `mi_heap_new_ex()` also uses the calling OS thread's default backing TLD (`heap.c:258–264`); it must not accidentally create a separate allocation/subprocess domain.

There is reusable machinery: private `mi_heap_absorb()` transfers queues and drains delayed frees (`heap.c:451–477`). However `_mi_page_queue_append()` (`Objects/mimalloc/page-queue.c:395–425`) updates `xheap` without retagging. General `mi_page_set_heap()` does retag (`Include/internal/mimalloc/mimalloc/internal.h:678`), and abandoned reclamation chooses a matching tag (`Objects/mimalloc/segment.c:1263–1271`). A narrowly specified young→old page-transfer operation must preserve object-kind offsets, update generation classification, obey delayed-free ownership, and never reclaim old pages into the next young epoch. Public delete/collect alone cannot implement that contract.

## Same engine, smaller mark domain

Use existing page metadata for age membership: for example, young OBJECT/GC tags with the same object offsets, transferred/retagged to their old counterparts. This is a finite extension, not a chosen implementation yet. Heap-pointer identity alone is insufficient after thread exit, when page ownership becomes NULL and reclamation can change heaps. Tags or another page-resident age state must survive that transition.

The minimum collector changes are explicit:

* Route admitted typed allocations in `pycore_object_alloc.h:18–68`; retain custom/debug/tracemalloc wrappers and allocation charging in `Objects/obmalloc.c:1225–1250`. Collection/queue allocations currently override `current_object_heap` (`gc_live.c:1976–1993`); choose their generation deliberately.
* Extend lookup and enumeration together. `live_page_of/live_lookup` check interpreter subprocess, page shape, kind offset and object-map membership (`gc_trace.c:1763–1853`). `live_walk_heaps` currently visits fixed OBJECT/GC tags and abandoned pages (`2293–2337`); `live_build_pages` initializes their offsets (`1724–1730`). Teach these paths age membership without removing current-domain/interior/initialization checks.
* Seed young tracing from ordinary roots, dirty old core owners and all allocated stable native owners, retaining registered spans and native-memory range protocols. Stop graph descent at clean old core objects. Existing direct managed loops and `tp_traverse` adapter remain the one tracing implementation. Scanning every old object would avoid barriers but lose the intended minor-collection bound.
* Produce dead records only for the young domain, then reuse current weakref/finalizer retention, preparation and sweep. These already use dead flags to delimit retention (`gc_live.c:637–679,1838–1909`). Promotion and remembered metadata must be committed before callback reentry. Allocation failure retains everything or falls back to a full collection before mutation; do not partially promote/reclaim.

No generational barrier exists today. Core container/type/shape stores need nonallocating dirty-owner recording with a fail-closed overflow path. Active frame arrays are rescanned roots; an old suspended generator must become dirty before resumed execution can publish young references (`genobject.c:261–292`). This can avoid a barrier on every register write, but requires all resume/throw/close entry paths. New old objects must publish their young edges transactionally. SHALLOW/under-construction survivors cannot simply become clean old objects; retain their protected classification until traversal is safe.

Thread exit currently abandons every heap (`pystate.c:2959–2973`). Young tags must remain enumerable, or exit must safely promote them and establish outgoing-edge obligations. Periodic majors must reclaim old cycles even with zero promotion. Partially live promoted pages retain slack; unlike copying, nonmoving promotion neither compacts survivors nor provides a bump-allocation fast path.

Full-only accounting also needs a finite change: `record_stats` writes generation2, and `collect` derives heap size/next allowance from the full mark result (`gc_live.c:1617,1918–1925`). A young-only result cannot replace global live-byte totals or masquerade as a full collection.

## Engineering decision

Compared with first copying cells/tuples, this avoids writable C-local conversion, derived-pointer pinning, forwarding, identity/hash/weakref relocation, and destination reservation/rollback. It retains existing `mi_free`, realloc, interior lookup and native address stability. The hard common work is barriers, constructor prefixes, age-aware enumeration and native scanning; the allocator-specific blocker is safe page transfer/reclamation, not a missing tracing algorithm.

**Prefer the nonmoving age-domain cut if that transfer remains a small internal extension and it admits meaningful generator/coroutine traffic.** First compare its minor live set against the existing full trace, with missing-barrier, old-native→young, resumed-generator, constructor/OOM, thread-exit/reclaim and zero-promotion-major controls. Keep the major engine authoritative. Add copying later only if survivor slack, allocation locality or per-dead-object teardown remains material. Conversely, if page transfer requires a second persistent inventory or invasive allocator ownership changes, proceed directly with the approved copying-space consumer; do not make a nonmoving detour mandatory.

## Captured inventory-versus-tracing check

This additional check narrows the conditional recommendation above: **the current evidence does not justify making nonmoving generations a mandatory first performance milestone.** It reparses the four already frozen perf-script texts, without invoking perf or another native program. Exact counts, periods and example stacks are in [mark-subdivision.json](../../../../build/gc-post-cleanup-performance-v2/phase-analysis/mark-subdivision.json), SHA `25be531bd1dfec26bd8b43fd23df892ba54680db2c872b54be0febf68c637285`.

Within the existing exclusive `gc_mark` bucket, explicit `live_mark_roots` ancestry proves roots plus transitive closure. `_mi_heap_area_visit_blocks` together with `live_walk_heaps` proves allocated-object inventory/classification: `live_build_pages` passes `visit_blocks=false`, and that allocator helper is called only for true block visits (`gc_trace.c:2293–2349,3441–3469`; `Objects/mimalloc/heap.c:733–754`). An unqualified `live_walk_block` leaf does not suffice. Explicit page-directory/cache frames receive their own categories; the remainder stays ambiguous.

| Exclusive sampled CPU % | Coroutines | Comprehensions | Generators | Richards |
| --- | ---: | ---: | ---: | ---: |
| Roots and transitive closure | 10.749 | 5.960 | 11.550 | 0 |
| Object inventory and dead classification | 12.138 | 7.276 | 4.458 | 0 |
| MEM page-directory construction | 0.060 | 0 | 0 | 0 |
| Weak-cache cleanup | 0.060 | 0.155 | 0 | 0 |
| Ambiguous/other mark work | 0.242 | 0.232 | 0 | 0 |
| Total existing mark category | 23.249 | 13.622 | 16.008 | 0 |

The two main coroutine categories contain178 and201 samples respectively; comprehension categories77/94, generator categories114/44. Every mark period and sample reconciles to the prior frozen analysis. This is evidence about code paths, not allocation ages or exact elapsed fractions. The independently timed coroutine root-closure/hot diagnostic10.95% is consistent in scale with the10.749% sampled subtree, but uses different boundaries and clocks; their difference is not an exact inventory timer.

A nonmoving young collector can avoid old graph traversal and old block inventory. It cannot avoid enumerating its admitted young allocations, constructing their dead records, or sweeping each young death using this engine unchanged. The measured inventory plus sweep-loop buckets already total21.739% for coroutines,12.539% for comprehensions and8.409% for generators, before the separately classified native-deallocator work. If those blocks predominantly die young, changing the age boundary retains much of that work. The48.56million coroutine deaths over301 hot collections establish high death volume, **not** their ages or a reclaimable nursery layout.

An intentionally loose bound is that eliminating all measured root/closure work would remove at most10.749/5.960/11.550 percentage points from those sampled CPU totals. A real minor must still scan roots, traverse young survivors and scan stable native owners, so its saving is smaller. Additional savings from skipping old inventory are possible but unquantified: these captures have no age classification. Do not assign the entire mark bucket to avoidable old-live tracing, or promise generational savings from the death count alone.

Consequently, nonmoving age separation is a lower relocation-risk correctness path, not yet the demonstrated largest performance path. To remove the measured per-death inventory component, the design needs bulk reclamation for safely admitted layouts, or another concrete reduction of per-object journal/sweep work; generations alone do not deliver that. Bulk reclamation cannot skip generator frame/resource cleanup merely because an object is dead, and a copying cell/tuple-only slice may initially cover too little of this traffic. Prefer one nursery milestone that couples its age/barrier contract to meaningful allocation and dead-storage coverage. Choose nonmoving first only with a finite source-derived coverage/old-work rationale, rather than adding a second complete qualification ladder as a compulsory detour.
