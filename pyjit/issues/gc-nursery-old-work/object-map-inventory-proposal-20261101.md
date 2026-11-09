# Page-scoped object-map inventory

State: review. Source-only proposal, 2026-11-01. No implementation or native run.
Candidate base: nursery-adoption `47ccf35e21a94fde9351d1c2e4571bf207257a4c`.
The adopted architecture remains nonmoving tracing GC; this proposal adds no
global collector option. Automatic cadence has separate evidence and review.

## One hypothesis

Enumerating initialized-object start bits inside allocator-provided object
pages can lower collection elapsed time by removing mimalloc's allocated-block
enumeration, free-list bitmap reconstruction, and per-block callback dispatch.
Keep the existing object classifier, old-root closure, promotion, death array,
lifecycle preparation and individual storage free paths.

The same consumer currently pays allocator enumeration for old seeding,
inventory and promotion. The proposed increment changes their common iterator;
it does not make old scanning disappear or introduce arena-wide reclamation.

## Measured opportunity and limit

The existing frozen coroutine profile attributes 12.1377% of sampled CPU to
object inventory plus dead classification and 10.7488% to roots plus closure.
Its explicit `_mi_heap_area_visit_blocks` leaf example accounts for
95,477,375 / 8,321,607,000 sampled period ns, about 1.1473% of total CPU.
This example proves allocator work exists; it is neither the entire allocator
cost nor a forecast that the whole 12.14% can be removed. Type tests, flag
updates and classification remain. The extra old-seeding/promotion opportunity
is source-derived; this historical profile has no age-specific timing.

Evidence: `build/gc-post-cleanup-performance-v2/phase-analysis/mark-subdivision.json`
SHA256 `25be531bd1dfec26bd8b43fd23df892ba54680db2c872b54be0febf68c637285`,
row `fork-32-coroutines`; neighboring `generational-path-feasibility.md` states
the attribution rules. Its source is historical, not the new candidate.

A complete selected-owner barrier has weaker evidence now: the profile has no
cell-specific old-owner cost. `Include/cpython/cellobject.h` exposes `ob_ref`
and inline stores, so adding one barrier call to `PyCell_Set` cannot establish
a complete native mutation boundary. Even a complete cell barrier would leave
other old owner families scanned. Select the common inventory consumer first.

## Exact algorithm and source boundary

1. Retain `live_walk_heaps` domain iteration, runtime head lock, domain pins,
   OBJECT/GC tags, thread-bound heaps and abandoned subprocess visits. Call
   both `mi_heap_visit_blocks` and `mi_abandoned_visit_blocks` with block visits
   false for these two tags. MEM-page discovery keeps its current behavior.
2. In each page-area callback, enumerate existing map words for
   `[area.blocks, area.blocks + area.committed)`. Check overflow before forming
   endpoints. Never scan reserved slack or the global 48-bit map. Missing map
   chunks mean no initialized objects; read via `_PyGC_ObjectMapChunkOf`, which
   does not allocate, rather than the allocating chunk constructor.
3. Mask first/last words, advance across 32 MiB chunks, use set-bit iteration,
   and validate each address against the canonical block stride and current
   debug object offset. Use `full_block_size` for stride and `block_size` for
   unchanged usable-byte accounting. Reject noncanonical/interior starts before
   reading a header. A singleton page checks its one canonical start directly;
   a huge object must not induce a payload-sized bitmap scan.
4. Pass each validated start to the existing per-object consumer, extracted
   from `live_walk_block` without changing its order or classification rules.
   Replay page visits separately for the existing phases; introduce no page
   directory, per-object registry, new allocation metadata or persistent cursor.

Minimum runtime delta: `Python/gc_trace.c`. Existing map accessors and allocator
APIs suffice; no mimalloc modification is proposed. Tests may require private
declarations in `Include/internal/pycore_gc_trace.h`, a `_testinternalcapi`
bridge, and `Lib/test/test_gc.py`. Record the changed traversal contract in
the issue results and GC documentation. Any extra production metadata or
allocator rewrite requires a new design decision.

## Why the existing APIs are sufficient, and what they do not prove

`Objects/mimalloc/heap.c:574` exports committed = capacity * full block size,
usable size and page start. Its false visitor still collects pending frees
and visits pages, but skips `_mi_heap_area_visit_blocks` and its free-map work.
`Objects/mimalloc/segment.c:1706` does the same for abandoned pages;
`arena-abandon.c:329` retains the subprocess cursor, re-abandonment and locks.
These paths cover huge pages and exited threads without introducing a global
map scan. Preserve all their callbacks and failure returns.

`Include/internal/pycore_gc_trace.h:180` defines initialized-start bits.
`Objects/object.c:new_reference` clears collector flags then sets the bit.
Raw allocator blocks and headers not yet initialized have no bit. The bit
does not authorize traversal of an unfinished payload: retain SHALLOW and
`live_safe_to_traverse`, current-domain/tag validation and static-immortal rules.

`Objects/obmalloc.c:PyObject_Realloc` preserves the old mapping on NULL and
updates it after successful realloc, including debug offsets and valid
same-address results. `PyObject_Free` and `_PyGC_FreeDeadBlock` clear the bit
before the backing free. Quarantine receives storage through those paths;
expiry frees storage whose object membership has already ended. Raw MEM,
NOSCAN and allocator callback frees are not additional initialized-object
publication paths. Do not move map writes or diagnostic retirement hooks.

This promotes the existing start-map invariant from a per-block header gate
to an enumeration authority. A stale canonical bit in an actually freed slot
cannot be proved safe from the bitmap itself. A complete supported-path
map-write/free audit and independent allocator-set oracle are acceptance
requirements, not an assertion that page membership alone fixes stale bits.

## Obligations

- Allocation/publication: no new bit writes or collection opportunities;
  constructor readiness remains the existing tracking/SHALLOW contract.
- Mutation/native owners: no write barrier is added. Marking still reaches
  every old owner and its closure. PyMem, opaque C roots, detach records,
  weak/borrowed aliases and native resource ownership retain their contracts.
- Free/realloc: clear/update membership at the existing points, preserve
  failure and same-address provenance, and never inspect a freed old header.
- OOM: bitmap iteration allocates nothing. Existing mark journal, dead-array
  reservation, fail-closed unwind, debt retry and lifecycle preparation remain.
  Test-only comparison storage is libc scratch, never a root; reserve it before
  changing mark/death flags and fail closed if it cannot be built.
- Callbacks/reentrancy: no Python callback, detach or user allocation is added
  inside page enumeration. Preserve current head/abandoned lock nesting and
  collector claim semantics. Lifecycle callbacks run in their existing phase;
  do not keep a cursor across them or reuse stale page descriptors afterward.
- Weakrefs/finalization: actual kind classification, resurrection revalidation,
  queue pins, buffer releases and native deallocators are unchanged. No bulk
  free shortcut is authorized.

## Positive and causal checks

Before destructive work, a test-only collector entry gathers candidate starts
and the current allocator true-block starts independently into libc arrays,
sorts address + usable size + tag records, and requires exact equality. The
reference retains its current map gate. Cover debug offset zero/nonzero,
partial/full/singleton/huge pages, OBJECT and GC tags, multiple threads and
abandoned heaps, raw uninitialized blocks, initialized partial constructors,
explicit free, failed/in-place/moving realloc, quarantine and address reuse.
Fixtures separately assert known initialized objects are present and known
raw/freed objects have clear bits; both iterators share the map, so equality
alone cannot prove that publication invariant.

Two temporary test-source negatives must be rejected before header traversal:
(a) skip one known populated candidate bitmap word while leaving the map and
reference unchanged; (b) add a stale start at a freed, unreused controlled slot
for candidate enumeration, with the reference allocator set excluding it.
Restore the real map before leaving the fixture if it was modified. This is
test-owned fault injection, not a production policy switch. Also nominate
wrong debug offset/boundary clipping and require a set mismatch, not a crash.

Normal collection witnesses must show page callbacks positive and allocator
true-block callbacks zero for the new object route, with identical object and
death-byte accounting. Run actual minor/major, OOM, retain/KILL, native-owner,
weakref/resurrection, generator and stale-root cases using existing helpers.
Source controls must remain temporary ablations and separate products.

## Finite implementation and decision

Propose 60 minutes source with a concrete 20-minute checkpoint, 20 minutes
independent exact-source review, then one 45-minute focused fresh-debug batch.
Stop first unexpected failure and preserve it. Missing lifetime proof or
unbounded huge/sparse bitmap work rejects the source increment before timing.
This estimate fits only if root grants it before the overall 04:18:11 UTC
deadline; this proposal itself grants no implementation or native authority.

After correctness and causal controls, compare matched O3 parent/candidate at
fixed major debt on coroutines, generators, comprehensions and Richards using
the established elapsed-first protocol. Require GC-workload geometric mean
improvement in both fixed rounds and no unexplained >2% repeated control
regression. Record peak RSS, retained live bounds and pauses; a >5% RSS change
needs an explicit bounded time/space decision. Instructions are unavailable
on this host and remain a declared diagnostic gap. Do not substitute callback
counts or code size for elapsed benefit. Reject a losing increment without
reintroducing collector optionality.

## Relevant collector comparisons

The useful arena idea is making existing object metadata an efficient scan
index. This experiment does not build the LuaJIT arena proposal or claim its
barriers exist. The original proposal URL was unavailable during this audit;
the [official LuaJIT 2.1 source](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_gc.c)
uses object links and color marking, so it is not evidence that the proposed
arena design is the shipped implementation.

[Immix's original paper](https://www.mmtk.io/assets/pubs/immix-pldi-2008.pdf)
reclaims blocks and finer lines and supports opportunistic defragmentation.
That allocator/relocation scope is larger than consuming Boil's existing start
map. Borrow the metadata-locality question; do not infer a copying proof or
skip CPython's object-specific death work.
