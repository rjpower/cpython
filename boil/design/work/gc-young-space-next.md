# Next collector milestone: common young allocation and reclamation

Decision 2026-10-30 after the landed cleanup/scanner measurement. Source/native
0b15 and integrationaee46 remain the qualified default; **no young collector
is implemented or enabled yet**. This replaces both a required cell/tuple
moving-first proof and a required nonmoving-generation detour. Build one useful
young-space path in the existing collector, then add relocation where it earns
its contracts. The [performance report](gc-post-cleanup-performance.md) gives
the measured baseline and limits.

## Why this cut

The combined cleanup yields1.417× coroutine and1.621× comprehension throughput
against the parent. Full29 is still0.871× upstream. Captured coroutine CPU
splits marking into10.75% roots/closure and12.14% heap inventory/dead
classification, followed by9.60% sweep work. Comprehensions has5.96% closure,
7.28% inventory and5.26% sweep; generators11.55/4.46/3.95%.

A young-only mark/sweep still enumerates its young corpses and builds their
dead records. It can save old work, but these data do not establish how much
inventory was old. The48.6M coroutine deaths in50 hot calls are not an age
histogram. A new allocator under unchanged full-heap inventory also misses
the structural gain. The milestone must demonstrate useful allocation and
dead-storage coverage, including a route that avoids ordinary per-corpse
journal/dispatch for safely recyclable storage.

`make_gen` allocates exact generator/coroutine plus embedded frame together
(`Objects/genobject.c`). The recursive coroutine/generator kernels create
these objects in hot work. A nursery that admits only cells/tuples while
leaving those allocations on the full-major path is a relocation proof, not
an evidenced broad win. Keep their addresses stable in the first useful cohort;
copying is optional until writable-alias contracts are complete.

## Implementation sequence

**Y1: close the space and lifecycle contract, then implement a complete
experimental path in an isolated branch.** Admit exact cells/tuples and exact
generator/coroutine allocations only where the descriptor covers the complete
allocation, initialization and cleanup. Unknown, resource-bearing, large and
custom-allocator cases retain stable mimalloc storage. Use space descriptors,
compact page/start/mark metadata and the existing authoritative object-start
map. Do not add a second identity registry or a 24–32-byte record to every 16-byte
cell. Start with an8 MiB experimental capacity; it is not an optimum or the 32 MiB
major floor. Charge page slack, side metadata and retained survivors explicitly.

The descriptor states allocation extent, initialized traversal prefix,
side-storage ownership, remaining lifecycle obligations and observer/free-hook
requirements. Preserve public allocator/debug/tracemalloc accounting. Prepare
space/map capacity before publication; collecting allocation can reenter before
storage exists. Keep SHALLOW objects protected until traversal is complete.

Generator lifecycle transitions must keep a compact pending-work index honest.
Reserve and record membership before publication or the transition that needs
it; remove membership on actual completion/free and before address reuse.
Allocation failure must retain the object or select safe ordinary processing,
never silently omit a pending entry. Epoch/address validation must prevent a
reused block from inheriting stale membership.
Completed, FRAME_CLEARED objects with no escaped frame, exception state, actual
weakrefs/hooks or remaining native/observer obligations are candidates for pure
storage discard. Suspended genexprs need the existing native-clear proof first.
Never infer readiness merely from the type or remove the required weakref,
finally, buffer or error behavior. The index is not a strong root. Ordinary
completed corpses should not require building and dispatching a full dead record
just to drop non-RC fields and return storage. Collector-owned graph/journal
reads and required native work finish before any page release.

**Y2: make age boundaries complete in that same path.** Core old-owner stores
need nonallocating barriers for list/dict keys and values, cells, shapes/type
fields and runtime-owned references. Active frame arrays remain roots; do not
push roots for each bytecode store. Resuming/publishing an old generator must
remember its initialized frame prefix before it can retain a young child across
suspension. Constructors, array reallocations, error exits and reentrant calls
need the same owner contract as ordinary stores.

Scan allocated old native owners at each minor, using authoritative space
metadata, so an unported native strong field retains its young child. This
index is not a root at major collection. An old native→old core→young chain
still requires the core owner's barrier. Native traversal remains a compatibility
adapter. A per-minor census of every old managed object would defeat this cut;
separate the native-owner boundary from the remembered core set.

Reuse the existing mark engine, lifecycle/dead-work machinery for exceptional
cohorts, queue policy and major collector. Minor liveness is confined to the
selected young domain plus roots/boundary edges. Every retained alias remains
valid. Promote survivors in place first where that is the simplest complete
contract; charge partial-page slack. Copy managed survivors later within this
same space protocol, after all readonly/interior/C aliases are pinned or reload
through writable slots. No new copying backend is required merely to prove age.

**Y3: prove safe bulk reclamation and measure the complete consumer before
changing the default.** A no-callback/no-observer fast cohort can recycle wholly
dead, fully processed pages or blocks after required lifecycle work. Mixed,
partly initialized, observer-active and exceptional cohorts use the existing
safe processing. No fast path may bypass map invalidation, accounting, debug
fills/quarantine, custom free hooks or reference-tracer notifications. A route
that still visits every dead object may be useful evidence, but it is not the
claimed bulk-reclamation win.

## Allocator decision and major policy

Vendored mimalloc's public `mi_heap_delete` is not promotion into CPython's
old object heap: it transfers only to a compatible thread backing heap, while
CPython's backing MEM tag differs from its object/GC tags. `mi_heap_collect`
only processes allocator state. Private page absorption changes heap ownership
without retagging pages. Use a narrow, explicit compatible page-transfer/reclaim
boundary or a young-page descriptor retained at its address; preserve object/GC
kind tags, interpreter/thread ownership, remote/deferred frees and thread-exit
adoption. Do not treat abandonment as successful promotion.

The first code review must close that boundary with a real survivor-promotion,
remote-free/thread-exit and major-enumeration path. If it requires a second
object inventory or invasive allocator ownership system, revise the backend
before expanding admission. The existing region packet supplies layout and
allocator-wrapper constraints, not an obligation to ship a separate experiment.

Young capacity, stable/promotion pressure and major allocation debt are separate
quantities. Retain the 32 MiB major floor. A new minor must not reset or hide major
debt. Charge stable allocation and promotion, and impose a finite major allowance
for zero-promotion traffic so old cycles/resources are eventually processed.
Record the experimental debt formula and constants before launching the prototype;
do not silently tune them per workload. This is a policy decision in Y1's source
review, not an unspecified production default. No age or survival distribution
has yet been measured to justify one.

## Landing evidence and ports fixtures

Land one complete experimental consumer, not inactive metadata followed by
successive full campaigns. Independent source review covers the combined space,
barrier, publication and lifecycle protocol before its finite gates.

- User-visible checks preserve ordinary results/data/errors through minors and
  promotion; old-core and native-owner edges; generator suspension/resumption;
  weakrefs/resurrection/finally; buffers and escaped frames; and allocating
  constructors. Check live payloads before/after removing owners and bounded
  retained memory. Preserve current queue/OOM behavior.
- Critical units check mark-domain fences, missing-barrier negatives, page/map
  reuse, construction prefixes, promotion and irreversible-commit OOM boundaries.
  Compare minor retention with an independent full trace under the same declared
  conservative native boundary, allowing intentional floating garbage.
- Memory checks include one survivor per page, zero-promotion old cycles,
  thread exit, live/young/promoted bytes, pending native work, metadata and page
  slack, RSS and pauses. These expose space/coverage costs, not private counter
  values substituted for a data oracle.
- Reuse the landed paired-four and four hot profiles with matched build policy
  and untouched kernels. Report eligible bytes, actual young death/storage
  coverage, old/native scan work and required teardown. If the prototype merely
  shifts the same journal work, retain that result and change the backend;
  do not promote a proof of movement or age into a performance claim.

The small native-owner/local/core-store/weakref/buffer fixtures become the first
ports-tree contract examples. Broader Cython/cffi/NumPy migration follows an
actual settled interface. Interpreter value/frame/call work can proceed alongside
this collector work after its separate bounded source proposal; the soft2×
guidance creates no opcode quota or aggregate landing threshold.

## Inspiration and retained source reviews

Ruby's write-barrier protection split offers a useful compatibility model;
its unprotected native fields require conservative generational treatment.
The exact Ruby rules differ from this proposed owner scan.
[Ruby extension guide](https://docs.ruby-lang.org/en/3.4/extension_rdoc.html),
[CRuby memory-management practitioner report](https://www.steveblackburn.org/pubs/papers/ruby-ismm-2025.pdf).
RPython's hybrid collector separates generations from nonmoving external
storage, illustrating that stability and age are independent choices.
[RPython GC documentation](https://rpython.readthedocs.io/en/latest/garbage_collection.html).
These are design comparisons, not evidence of this fork's speedup.

The retained source/coverage reviews and captured mark subdivision are linked
from the [performance report](gc-post-cleanup-performance.md). They explain why
neither mandatory moving-first nor mandatory nonmoving-first follows from the
new measurements. One useful common-young allocation/reclamation contract is
the required outcome; relocation is a later capability within it.
