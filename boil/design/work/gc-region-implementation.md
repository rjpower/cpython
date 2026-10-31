# First region backend: implementation cut and decision

2026-10-30. Source plan only; no allocator or collection policy has changed.
Source anchors below refer to integration `9c3c0d574246`. Selective death is
being qualified separately; native-only generator cleanup owns `gc_live.c`
until its source review and composition finish.
This refines [architecture section B](gc-architecture-next.md) and
the [code-organization review](reviews/gc-code-organization-review.md).

**Do the existing-mimalloc cleanup first.** A region can remove repeated
individual storage returns, but it cannot remove required object cleanup,
full marking or the present dead-object journal. The measured parent
comprehension profile puts 600/2463 hot CPU samples (24.36%) in finalizer
preparation, not in storage return. Native-only cleanup addresses that larger
identified boundary. The existing generator deallocator also performs redundant
TRACK/stub/UNTRACK operations: `PyObject_CallFinalizerFromDealloc` already
returns zero unconditionally (`Objects/object.c:188`). Removing those operations
and the stub's unreachable RC body preserves the exported API, initial untrack,
weakref handling and actual frame/origin/name/code cleanup. Compilers already
discard the unreachable body; its deletion has no claimed speedup.

A full-mark region remains a useful next storage experiment, conditional on
coverage and hook costs after that simpler slice. It is not a moving nursery,
a minor collector or a reason to introduce an allocator registry. Retain the
32MiB collection budget. Give the experiment an independently bounded 8MiB
pool; pool exhaustion falls back to mimalloc without forcing collection.
Report this bound separately from future nursery sizing.

## Exact first objects and allocation route

Admit exact `PyCell_Type` and nonempty exact `PyTuple_Type` only. Empty tuples,
tuple subclasses and oversized tuples use the current path. Start with 64KiB region pages and a maximum raw request of one quarter-page
(16KiB), keeping a large admitted tuple from consuming most of one page. These
are bounded experimental storage choices, not public allocation limits. Do not add lists,
dictionaries, generators, side buffers or native extension instances yet.

The concrete producers are `PyCell_New` (`Objects/cellobject.c:11`) and
`tuple_alloc` (`Objects/tupleobject.c:34`). Both reach
`_PyObject_GC_New/Var` (`Python/gc_live.c:2622,2641`) through `gc_alloc`.
`_PyObject_MallocWithType` (`Include/internal/pycore_object_alloc.h`) currently
selects a mimalloc heap around **the public allocator**, rather than carrying
type information through an allocation protocol. A new bump inside that
helper would bypass public accounting, custom allocator wrappers and
tracemalloc. That is not an acceptable implementation.

Add one private typed allocation entry in `Objects/obmalloc.c`, called by this
existing typed helper. Factor the current request validation, logical charging
and statistics so a request is charged exactly once, including fallback.
`gc_count_alloc` (`obmalloc.c:1227`) can collect before storage is obtained;
recheck allocator/tracer eligibility after that call. Do not use an ambient
"next allocation is a tuple" TLS token: a nested wrapper allocation could
consume it.

Initial admission recognizes only the default object mimalloc allocator and
its standard debug wrapper. Unknown custom wrappers and active tracemalloc
take the unchanged public path. In particular, `_PyMem_MimallocEnabled`
(`obmalloc.c:959`) alone does not establish the complete outer wrapper chain.
Use the existing debug allocation implementation with an explicit backend
context, rather than duplicating its padding, API-byte and fill conventions.
Tracemalloc installs object allocator wrappers (`Python/tracemalloc.c:885–893`)
and a reference tracer: bypassing those would lose required observable events.
This first fallback is compatibility, not a claim that traced allocations were
region tested.

## Storage metadata and construction publication

Keep private, off-heap page descriptors and allocation records containing raw
start, raw extent, object offset and live/retired state. Obtain payload pages
from anonymous mappings, not an object or PyMem carrier that the current heap
walker could scan a second time. Metadata is not a strong-root table. Store
the owning main-interpreter identity explicitly; the allocating thread is not
the lifetime owner. Keep one bounded range directory; do not forge mimalloc
segments or pass region pointers to `mi_*` APIs.

Before advancing a page cursor, reserve the record and the object-start-map
chunks for the prospective address. Existing `_PyGC_ObjectMapChunk`
(`obmalloc.c:1263`) maps lazily and fatally: `_Py_NewReference` cannot report
failure after allocation. Add a fallible private reserve operation for this
new producer, retaining the current fatal contract for old callers. Descriptor,
map or page OOM leaves no published object and no changed cursor. An ordinary
region resource failure may fall back to mimalloc; failure of that allocation
reports the existing MemoryError. Logical charging remains the normal gross
request accounting even when allocation fails.

The object-start bit means an initialized object, not merely reserved storage.
`Objects/object.c:2694` sets that bit and invokes the CREATE reftracer.
`_PyObject_InitVar` currently sets size after its ordinary initialization call;
`tuple_alloc` also promises uninitialized items, and all GC-type objects remain
visible to this collector even when untracked. Therefore do not infer readiness
from TRACK alone. For the two admitted layouts, initialize the cell value or
all tuple value slots, type, tuple size and cached hash before publication or
any possible callback. Zeroing tuple slots adds writes; measure them rather
than promising a pure bump-allocation win. Never expose an interior pointer
into a partially initialized tuple to a traversal that trusts its size.

No collection, allocation, detachment or callback may intervene between the
final metadata reservation and this infallible initialization/publication
sequence. Root the type using the existing span. This is the smallest actual
consumer of a construction-publication boundary; it does not require a generic
constructor framework or changing every existing allocator.

## Lookup, full enumeration and reclamation

Provide a narrow internal space lookup returning object start, raw block,
extent, owner and initialized/retired state. Region range membership must be
checked before the current mimalloc-only accesses:

- `gc_block_object` and `_PyObject_CheckObjectHeap`
  (`Objects/obmalloc.c:1294,980`);
- `live_page_of/live_lookup` and `_PyGC_LiveOwner`
  (`Python/gc_trace.c:1729,1800,1705`);
- debug offsets, allocator-size queries and quarantine paths.

Exact roots accept initialized object starts only. Conservative interior roots
may resolve addresses inside an active object's allocation extent, preserving
the current conservative padding/block behavior; never resolve unallocated
page tail or retired records. Such retention is not evidence that every C
pointer is writable or that the object can move.

Extract the object-classification body of `live_walk_block`
(`gc_trace.c:2106`) into a shared helper taking `(object, raw block, extent,
owner)`. Keep the mimalloc block adapter and add a region-record adapter to
`live_walk_heaps` (`2206`). Every full mark, object-list diagnostic and owner
query must see the same objects exactly once, including allocation by an
exited thread. Preserve kind classification, marks/SHALLOW handling, dead-array
OOM rollback and byte accounting. The first region still pays per-object
classification and dead journaling; it does not solve old-live enumeration.

Continue ordinary `tp_dealloc` work before storage retirement. Region
`PyObject_Free` and `_PyGC_FreeDeadBlock` (`obmalloc.c:1674,1701`) clear object
identity and retire a record. Preserve logical allocator/free statistics and
the current outer free wrappers. The inner default free dispatch recognizes
region storage before mimalloc; installation of a later wrapper must not make
an already allocated region object unfreeable. Do not bypass wrappers merely
because the physical page is retained.

No holes are reused in this first backend. A page resets only when every
record is retired, no collector dead/block array still borrows it, and debug
quarantine permits reuse. Add the collector's storage-completion call after
dead-array use/free, not during the first `_PyGC_FreeDeadBlock`. Retain survivor
pages: one long-lived cell can strand a page. The fixed pool bounds that slack;
report occupied pages, active bytes, retired bytes, metadata and fallback
requests. These are finite backend accounting fields, not another profiling
service.

## Manual free, resize and debug modes

`_PyStolenTuple_Free` (`tupleobject.c:230`) and ordinary cell/tuple deallocation
must use the same retirement path; neither may call `mi_free` on a region
interior. `_PyTuple_Resize` (`992`) includes shrink, empty conversion, failure
and untracked temporary cases. Initially, resize always escapes to mimalloc:
allocate the replacement, copy the smaller recorded extent, then retire the old
record only on success. Preserve the existing caller's size/map/weakref and
error ownership behavior. Do not reset a page while a resize caller still
holds its old pointer. Debug realloc mutates headers, so extent information
must come from the allocation record, not a temporarily damaged object header.

Preserve debug prefixes, forbidden-byte checks and clean/dead fills. ASan must
poison retired object ranges and unused page space and unpoison only valid
new allocations. Existing kill-mode quarantine retains storage across later
collections: page reset must wait for the same interval. Disabling new region
admission under kill mode is insufficient if old region objects survive a
mode change. If those existing debug/quarantine contracts cannot be retained
with the narrow dispatcher, keep region admission disabled in the candidate
until they are implemented; do not call a default-only prototype qualified.

## Ownership, OOM and the minimum patch

The supported configuration is one main interpreter, ordinary threads and the
GIL. No region mutation detaches. Thread exit cannot free pages containing live
objects. Interpreter shutdown also cannot unmap pages still named by late
cleanup. Keep the bounded pool/range directory alive until the existing late
free paths finish; repeated runtime initialization needs an explicit terminal
ownership point before implementation. Process-exit retention of a bounded
pool is not a solution to repeated-init leaks.

| File owner | Minimum implementation responsibility |
|---|---|
| Allocator lane: `Objects/obmalloc.c`, `pycore_object_alloc.h` | Typed admission, common charging, metadata/reserve, range lookup, debug/free/realloc dispatch, reset and OOM |
| Allocator lane: `Objects/cellobject.c`, `Objects/tupleobject.c`, `pycore_object.h` if needed | Exact-layout construction/publication; audit all tuple producer and stolen/resize routes |
| Collector lane: `Python/gc_trace.c`, existing `pycore_gc_trace.h` | Shared classification adapter, lookup, owner and full enumeration |
| Collector lane: `Python/gc_live.c`, `Include/internal/pycore_interp_structs.h` | Infallible publication connection, post-journal reset boundary and final lifetime ownership |
| Tests | Existing cell/tuple/GC/tracemalloc/allocator seams; focused external lifetime, resize and hook checks |

Keep the initial backend in obmalloc rather than adding a family of allocator
files or a plugin vtable. Merge the native-cleanup owner first; coordinate the
few `gc_live.c` hooks instead of concurrently rewriting collection. The hard
current contracts are fatal lazy map growth, mimalloc-only address decoding,
pre-payload object publication and public allocator wrappers. None is solved
by swapping `malloc` for a bump pointer.

Before enabling this backend, require meaningful tests for exact versus
subclass/empty admission; tuple stealing and all resize outcomes; same-thread
and cross-thread lifetime; exact/interior/stale address lookup; hooks installed
before and after allocation; tracemalloc fallback; metadata/page OOM before
publication; survivors across reset; debug/ASan/quarantine mode transitions;
and repeated initialization ownership. Run an independent ordinary full-mark
omission oracle on mixed-space graphs. Existing suites then qualify the actual
union. This packet has not implemented a backend or run native validation; these
checks define the next implementation boundary.

Proceed only if post-cleanup profiles still identify common storage return or
lookup as a material cost and exact cells/tuples cover enough bytes to justify
this cut. Compare total elapsed, instructions, RSS/slack and pause maxima with
the same budget; a faster allocator with more full-heap work is not automatically
a useful collector improvement.

## Review35: what to keep from the current root/traverse structures

`_PyGCRoot` is an interpreter-local, sparse, durable registration list
(`pycore_gc_trace.h:72`, `gc_trace.c:4469`). Its intrusive add/remove are O(1),
address-stable and allocation-free when the caller owns the node. The public
one-slot wrapper allocates a node and searches by slot on removal. A shadow
stack cannot replace arbitrary-lifetime removal cleanly. Keep the intrusive
durable list; prefer caller-owned handles in hot internal paths. C locals
already use a per-thread LIFO `_PyValueRoot`/span stack
(`pycore_value.h:334`); chunking that storage is a separate measured allocation
question, not justification for a second universal registry.

Frame registers are roots already: `visit_frame` (`gc_trace.c:1377`) calls
`_PyFrame_Traverse` (`Python/frame.c:12`) over frame storage at collection.
The evaluator does not permute a root list for every bytecode. A moving
nursery needs writable locations and correct publication at safepoints, not
re-registering every register after each instruction.

For common exact managed layouts, dispatch once per object to a direct
writable slot loop: cell value, tuple value array, then other proven layouts.
This can remove callback-per-edge overhead while sharing mark/update logic.
Keep `tp_traverse` as the pinned-native compatibility adapter; its callback
reports referents, not necessarily mutable field addresses. Replacing every
extension traversal now would expand the porting boundary without evidence.
The measured mark samples include lookup, graph work and memory access; they
do not isolate traversal dispatch or assign a per-edge cost. Introduce slot descriptions with the first direct managed scanner, so marking
already consumes them before copying. Preserve a shared field definition rather
than mechanically replacing every native callback.
