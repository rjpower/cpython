# Object-map inventory design review

Decision: **ACCEPT the design for a bounded source prototype**. Reviewed on
2026-11-01 at 00:53 UTC. The coordinator must grant implementation separately.
This is neither exact-source acceptance nor permission for a compiler, target,
correctness batch or performance campaign.

The frozen proposal is [object-map-inventory-proposal-20261101.md](../object-map-inventory-proposal-20261101.md),
SHA-256 `1411d4567bc1a85f124b6631a570a15e016b0ae6a19c061ada83957551bbbfe2`.
I reviewed the actual clean candidate
`47ccf35e21a94fde9351d1c2e4571bf207257a4c` in
`build/worktrees/nursery-adoption`, rather than assuming the primary worktree's
free/diagnostic paths are identical. The proposal and source-evidence hashes,
eleven candidate source files and historical profile are bound in
`build/gc-structural-design-review-20261101-v1/receipt.json`, SHA-256
`2526ea25ebeff2794140c8faf4d40bbd3c7fe6edccbb2121a3c376c93e70ff96`.

The existing allocator APIs support this increment. Heap and abandoned-page
visitors collect pending frees and expose page start, committed capacity, full
stride and usable block size before the optional allocated-block walk. The
cached `mi_page_start` is asserted equal to the canonical start used by that
walk. Suppressing block visits therefore removes the free-list bitmap
reconstruction and allocator's indirect callback per allocated block while
retaining page discovery and locking. A singleton, including a huge object,
can be checked with one canonical map lookup. General pages must use committed
capacity, never reserved slack, and mask word/chunk boundaries before set-bit
enumeration. A map bit outside canonical block starts cannot authorize a
header read.

The source lifetime boundary is coherent. `new_reference` sets membership
after initializing the header; public object free and dead-block free clear
membership before backing free. Successful object realloc updates membership
without an intervening Python callback or allocation, while failure retains
the old mapping. Quarantine expiry frees already-retired storage. The new
iterator must preserve these points and existing SHALLOW/constructor checks:
an initialized header does not prove that its payload is ready to traverse.
Raw MEM/NOSCAN discovery, static objects and current domain/page validation
keep their present contracts. This review does not widen the supported native
allocation API boundary.

The common iterator reaches old seeding, classification, promotion and heap
object listing. Preserve each consumer's ordering, usable-byte accounting,
failure return and mark/death unwind. Keep domain pins, head-lock scope, thread
heaps, subprocess-tag filtering and abandoned cursor cleanup. No descriptor
may survive lifecycle callbacks. Nonallocating map reads are sufficient for
production enumeration; the existing mark journal and dead-array allocation
can still fail and must retain their current behavior. The proposed source
boundary introduces no new barrier, allocator metadata, bulk destructor or
alternate collection policy.

Two concrete implementation obligations need explicit coverage:

- Separate object-route selection from the visitor's block-visit argument.
  Current `live_walk_heaps` uses `all = !w->pages` both to select OBJECT/GC loops
  and to request allocated-block visits. Setting that variable false would
  silently skip those loops. MEM page discovery must remain unchanged.
- Build both comparison buffers before the stale-start fixture mutates any
  membership. Retain a live object in the same size class/page, free the
  nominated slot, and permit no object allocation, callback or collection
  before address-only enumeration. Assert that the allocator reference
  excludes that exact address before invoking any header consumer; restore
  the injected bit before return. The author acknowledged this construction.

The two iterators share the membership gate, so equality alone cannot detect
an omitted publication bit. The proposal correctly requires separate known
live/raw/freed membership assertions, in addition to exact address/size/tag
sets. Preserve the omitted-word negative with the real map unchanged, and
intercept the stale-start mismatch before header traversal. Actual debug
offsets, partial/full/singleton pages, huge objects, thread abandonment,
realloc outcomes, quarantine, reuse and comparator OOM remain source/native
acceptance obligations. These are proposed checks, not executed behavior.

The historical arithmetic independently matches: inventory plus
classification is 12.1377% of sampled CPU, roots plus closure 10.7488%, and
one explicit allocator leaf example 1.1473%. The first figure is an umbrella,
and the last is an example rather than the exclusive allocator total. Old
seeding/promotion offer a source-derived opportunity without age-specific
timing. None predicts elapsed improvement. The proposed elapsed-first
comparison, raw vectors, retained-liveness/RSS and pause observations must be
fixed and reviewed before scores; callback counts cannot accept this change.

No design blocker remains. A missing publication/free proof, an unsafe stale
fixture, unbounded huge-page work or changed lifecycle/OOM semantics should
reject the source increment. The proposed finite source/review/debug stages
and overall 04:18:11 UTC boundary remain held; this review does not extend
them. Only host source inspection, Git identity checks and historical JSON
arithmetic ran here. Existing cost bindings and all native products remain
unchanged.
