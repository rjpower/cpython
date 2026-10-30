# Step 7: a nonmoving young/old collector

Source proposal, 2026-10-29; no implementation, build or additional native
measurement. Inspected primary c466e047e00e and ROOTS c67954f336a9, whose
collector C remains ce2ac. Rebase the design on the reviewed integration
union before implementation. Note 11 §11 remains authoritative: roots are
enumerated each minor, stores remember heap owners, and frame suspension
has an explicit owner handoff. No changed contract is approved here.

## Objective and smallest model

[The four-row attribution](pre-I-shared-cost-attribution.md) found measured
collection fractions of 54%/47%/31% in coroutines/comprehensions/generators,
and no hot collections in winning Richards. This establishes collector cost,
not savings available from generations. Coroutines reported 56.43M candidates,
48.56M collected and 10.88GB freed against a last live gauge of 3.05MB:
per-object death processing can remain dominant; allocation ages were not
measured. Generations chiefly avoid repeated
old-live traversal and enumeration; finalization/order/sweep work needs its
separate source assessment. Do not promise a fraction of those percentages.

Use two nonmoving generations, one survivor promotion and a remembered set
of old owners. A major uses the existing whole-domain mark/death pipeline;
a minor treats old objects as retained and reclaims only unreachable young.
Old-to-young edges seed the young trace. Old objects and mixed-generation
cycles die at a major. No concurrent/incremental trace, moving nursery,
per-type collector fallback, card table or JIT dependency is introduced.

Proposed age/remembered metadata are side bitmaps indexed exactly like the
existing initialized-object map (`pycore_gc_trace.h`, `obmalloc.c`), not new
header fields. Keep MARKED/SHALLOW/DEAD transient; MARKED is also SWEPT on
dead objects and cannot simply become the sticky old bit. Two extra maps
have a worst-case bitmap cost of 2/64 of represented object address space,
plus directories; record actual resident pages/RSS, not just virtual sizes.
Reserve their chunk storage before setting the initialized-object bit.
The present object-map mmap failure is fatal; do not invent a recoverable
allocation contract or allocate Python/remembered-set storage on a barrier.

Creation in `Objects/object.c:new_reference()` clears age/remembered state
before CREATE observation. Free/dead-free clears all maps before reuse.
`PyObject_Realloc` and `_PyObject_GC_Resize` must preserve or transfer age and
remembering on success, preserve old state on failure, and clear the former
address even when it is reused. This concerns existing explicit resize,
not collector movement. Raw PyMem/table blocks are not separate generations.

Minor enumeration must walk `initialized & ~old` bits of the pinned domain,
with allocator block size/debug-offset checks. Merely filtering the existing
`live_walk_heaps()` per-object walk still pays old block enumeration. The
domain/segment membership directory is an actual missing interface: bind it
to `live_domain()`/segment ownership and exited-thread heaps, not guessed
from the current thread. Major enumeration remains the existing heap walk.
The [source ledger](7-generations-domain-ledger.md) now names the actual
reserve/publication, free/reuse, resize and abandoned-heap boundaries. Its
young-start index is a concrete interface proposal; representation and
resident-memory cost remain to be decided before implementation.

## Collection and barrier contracts

1. Pin the same domain as `_PyGC_TraceLiveMark()`. Enumerate every existing
   precise/conservative root class, including static types, immortals,
   registered spans, detached/opaque frames, data segments and queues.
   Their root storage is scanned each time; it needs no heap-store barrier.
   A root pointing at an old object does **not** imply recursively tracing
   the old graph. Its young children must be remembered by their heap owner.
   Static types/immortals retain their explicit immediate-field traversal.
2. Traverse each remembered old owner's strong fields, then close the young
   graph using the existing PyValue-aware traversal. Preserve type/shape,
   managed-slot/dict and non-GC-object edges: HAVE_GC/tracked is not an age
   test. Interior conservative words use the existing object lookup and
   transitive raw-memory rules; they retain objects in place.
3. Construct only young death candidates. Complete the current weakref,
   finalizer/resurrection/destroy-hook and ordered sweep transaction before
   committing promotion/remembered changes. Old objects never acquire DEAD
   because a minor did not visit them. Keep whole-domain pin/routing and
   actual-owner queue rules unchanged.
4. Promote surviving young objects that the mark actually traversed safely,
   or proved leaves. An unsafe SHALLOW construction survivor remains young;
   no tp_traverse is called on uninitialized fields. Keep a remembered owner
   if its conservative/opaque edge still reaches such a young survivor.
   Promotion and clearing remembered bits commit only after success;
   allocation/verification failure retains objects and remembered entries.

Promotion evidence must be captured per object before `live_walk_block`
clears MARKED/SHALLOW. The present aggregate `nshallow` also counts marks
later upgraded by a precise visit, so it cannot determine survivor promotion.
Specify staging storage and rollback together with the young index; neither
cleared header bits nor a NULL `tp_traverse` alone prove safe promotion.

The current collection unpins its domain and can call a Python debug writer
before sweep, earlier than stop callbacks or queued Python. Thus a metadata
commit placed only after sweep/before stop is insufficient. Activation waits
for the existing no-Python-dead-snapshot repair or a reviewed staging proof
covering that earlier reentry and allocation-failure boundary. Preserve every
barrier write made after the commit; never clear it from an older snapshot.
This records a source dependency, without authorizing Queue2/S3 changes.

The proposed owner-aware store operation is new, not an existing API: under
the owning domain's GIL, remember the old owner before publishing a strong
heap pointer/PyValue. It is nonallocating, noncollecting and idempotent.
For a single store, NULL/immediates and proven old targets need no entry;
bulk replacement can remember the owner once without inspecting every item.
The check occurs **after** any allocating conversion, before the store,
with no intervening safepoint. Deletion alone needs no insertion barrier.
No generic INCREF/DECREF or Py_SETREF rewrite supplies this information:
the former are lifetime operations; Py_SETREF knows an lvalue, not its owner.

Constructors can collect between initial stores. “Brand new” is not a
barrier exemption: a safely traced constructor result might already be old.
Require barriers on its subsequent stores unless the exact region proves
no collection before publication. Old-owner traversal must be safe whenever
allocation is allowed, as full tracing already requires. The SHALLOW rule
does not prove all existing constructors satisfy that obligation; close the
constructor/traversal ledger before enabling minors. Unsupported raw fields
or foreign-owned buffers are blockers, not permission to guess their owner.

Keep note 11's frame rule: running chain/data-stack slots are visited fully,
including an old resumed generator before its next yield, so those slot
stores need no heap barrier. Remember the owning generator/coroutine before
unhooking at yield/return; cover exception, close/throw and async paths.
`Python/frame.c:take_ownership()` must remember the frame owner immediately
after copying slots and before its allocating injection/back-frame work;
later f_back/header stores need their ordinary barrier. External edits to
suspended locals use the heap owner. Reconcile these boundaries with H2
and regenerate handlers from `Python/bytecodes.c`, never patch generated
handlers independently.

## Store inventory and current coverage gaps

This is the complete set of coverage obligations, with concrete current
bypasses. It is **not** a completed assignment-by-assignment audit. Before
activation, one owner/field/store ledger must close every tp_traverse edge,
extra type/dict/shape edge, raw backing-store publisher and root-only field.

| Family / existing source | Required owner boundary and bypasses |
| --- | --- |
| Lists/tuples: cpython headers, listobject.c/tupleobject.c | SET_VALUE/SET_ITEM, append-resize, slice/extend/repeat, raw ob_item writes and memcpy/memmove; remember the list for external item arrays. Tuple construction/resize needs the same promotion rule. |
| Dicts/sets: dictobject.c/setobject.c | Insert/replace, split values, merges, resize/table swaps and bulk copies. STORE_KEY/STORE_VALUE currently lack owner; a low-level table address alone is insufficient. Shared keys need the type/dict owners whose traversal exposes them. |
| Instances/types: shapeobject.c/typeobject.c/object.c/structmember.c | Shape slots/transitions/dict-mode conversion, managed/offset dicts, member stores, __class__/Py_SET_TYPE, type attributes/MRO/bases and shape-owned references. Preserve flags when changing the header shape. |
| Fast handlers: bytecodes.c and generators | Raw STORE_SUBSCR_LIST_INT, STORE_ATTR_SHAPE/TRANSITION/POLY/WITH_HINT/SLOT bypass central setters. STORE_GLOBAL/NAME must reach barriered dict paths; STORE_DEREF must reach the cell barrier. Regenerate all emitted variants. |
| Cells/functions/code/descriptors | PyCell_SET_VALUE; function defaults/closure/annotations/typeparams; cached code fields, monitoring/feedback references, methods/properties and descriptors. Audit funcobject.c/codeobject.c/methodobject.c/descrobject.c, not only Python attribute assignment. H3 pure-data arrays contain no hidden Python roots by assertion, not by name. |
| Frames/generators/exceptions/context | owned_slots handoffs, frame headers/locals proxies, gi_exc_state, async awaitables, traceback/exception links and context HAMT/state. Running frame roots and suspended heap fields are distinct. |
| Native containers and modules | deque blocks, functools caches/partials, asyncio tasks/futures/callbacks, weakref callback fields, module state and each extension's own traced pointers. _collectionsmodule.c/_functoolsmodule.c/_asynciomodule.c contain independent mutations. |
| Roots/queues/foreign memory/ports | Registered raw roots/spans and interpreter roots are scanned directly; a heap object's PyMem/malloc tables still need their logical owner. Queue op/callback roots remain explicit. Port APIs/macros and C/C++ direct fields need rebuild and migration; opaque frames do not cover old heap mutation. |

Shared/backing storage must have a sound owner relation, including aliases;
remembering a newly allocated view does not cover an old owner. Weak referents,
borrowed caches and weak task lists must not become strong remembered roots.
Detached mutation/FT needs its own synchronization proof; this proposal enables
the existing GIL collector only and makes no free-threaded barrier claim.

Owner remembering is the authoritative starting choice. A card set could
bound large-owner rescans, but needs a card-to-logical-owner/traversal mapping
for external/shared tables that does not exist. A target-only insertion set
avoids owner rescans and unsafe owner traversal, but retains overwritten/deleted
young targets as false roots, promotes their young graphs, and raises survival,
old-death and RSS cost. Its per-epoch retention is at most the epoch's young
heap, but repeated promotion is unbounded without an independent major budget.
It is a design alternative requiring a coordinator ruling, not a fallback or
an adopted deviation from note 11. Start without cards/target-only layering.

## Lifecycle, validation and implementation order

The minor must reuse `clear_callback_weakrefs`, `handle_finalizers`,
`queue_destroy_hooks`, routing and `sweep` for its young candidates, preserving
once-only flags, legacy-cycle/DEBUG_SAVEALL handling, resurrection closure and
non-GC re-finalization bookkeeping. Queue running/current work stays rooted;
Python hooks still run only in the existing permitted context. A major must
recompute all live/death flags normally, not interpret sticky age as liveness.
Minor gc.get_stats/get_objects/counts/callback-generation semantics and freeze
are changed interfaces: define young=0, old=2, empty compatibility generation1
before coding; keep explicit full collection/shutdown as majors.

Keep separate young-allocation and major-debt budgets. Promoted bytes alone
cannot schedule majors: an old unreachable cycle can persist while promotion
is zero. Require a finite allocation-since-major budget as well as promoted-byte
pressure; declare both from memory headroom/cadence policy before activation.
Existing stress controls must exercise the declared collection kind, not
silently become a minor-only test. No numeric policy is inferred from these
four profiles; old-death latency, peak RSS and per-major pause are acceptance
constraints. This missing policy is an activation prerequisite.

1. Close source/store/constructor/domain interfaces on the actual ROOTS+3a
   union; independently review root/frame/queue ownership and the API/ports
   changes. Keep minors disabled until all compiled mutators are covered;
   no unknown-type always-dirty compatibility layer. Preserve inherited HTTP
   and queue-progress debt; scoped incremental ROOTS landing does not close
   the final GC/performance correctness checkpoint.
2. Implement age/enumeration and owner barriers with the existing full mark
   as an independent oracle before reclamation. In debug verification, full
   tracing with age/remembering ignored must reach no young object omitted
   by the minor. Use separate mark state, not A's overwritten marks/dead list.
   OOM/incomplete oracle means no reclaimed objects and no clean receipt.
3. Demonstrate named barrier omissions before sweep: old list/dict/cell/shape
   and raw specialized store; native module/port field; suspended generator
   and frame ownership transfer. Scrub incidental conservative roots. Missing
   registration must fail the minor/full young-reachability assertion, not
   crash later. Positives cover collection while the old generator runs,
   construction promotion/SHALLOW-to-precise upgrade, resize/reuse, bulk/shared
   storage, weak callbacks, resurrection/finalizers and failed transactions.
4. Qualify the coherent source, then compare the same release cohorts and
   workloads: wall time, candidate/death counts, survivors/promoted bytes,
   roots/mark/death-pipeline cost, barrier-heavy controls, minor/major pauses
   and peak RSS. Add only the missing generation counters needed for that
   decision. Keep generations only with useful shared throughput/pause benefit
   and acceptable retention; reject a design that merely moves nursery-death
   work into more expensive majors or taxes winning object/call controls.

The artifact establishes a reviewable direction and the missing proofs; it
does not clear implementation, claim complete mutator coverage or supersede
note 11. The first source deliverable is the closed owner/store/constructor
ledger, not a speculative young sweep over the present unbarriered tree.
