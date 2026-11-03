# Independent stable-storage mechanism review

2026-11-01. **ACCEPT the exact design for a bounded, ownership-complete source
attempt.** No runtime implementation, native feedback, performance or landing
is accepted here. Root owns any source grant. This review used light source
and document reads while the separately authorized stats batch ran; it started
no target, compiler, workload, helper or bulk hash operation.

Reviewed final plan: `stable-storage-mechanism-plan.md`, SHA256
`f97c3de4be222e304de5514c05825bdd0d4ba2d6d7ae79102ef9f3781911b5f0`.
Author final-v2 receipt SHA256:
`b5a79817855ee9d92d5cb5b13258e2c965bf2dfc4866cfe62169827935982822`.
The prior `7be69c04…` plan used a different backing policy and is superseded;
this disposition does not accept by near identity. The earlier storage
hypothesis remains historical input, including its broader cheap-death idea.

## Why this is an executable mechanism

The candidate changes allocation, object lookup/inventory and final storage
return together for exact native coroutines and their embedded frames. It
keeps every ordinary gen_dealloc, dead record, lifecycle graph, weakref,
resurrection, constructor prefix and frame state. Therefore completed-state
cheap-death eligibility is **not** an admission or prerequisite census. Actual
family/size/live/dead/occupancy facts can be measured as prototype witnesses.
An eligibility census is appropriate for a separate deallocator shortcut,
which is explicitly absent here.

The historical construction/death consistency and sampled categories nominate
the family, not the admitted fraction or promised speedup. The source prototype
must report logical requested bytes, raw stride, backing/metadata bytes and
retained slack separately. Global lookup overhead and nonmoving fragmentation
are package costs even if its free loop improves.

The final backing choice is concrete and source supported:
`Objects/obmalloc.c:_PyMem_NoScanMalloc` calls
`mi_heap_malloc(_PyMem_NoScanHeap(), size)`; NoScanFree calls mi_free. Existing
NOSCAN blocks are excluded by trace tag offsets and ordinary object inventory.
The new domain span inventory must therefore be their sole object enumeration,
including failed-mark cleanup. The explicit attached-thread/thread-id gate is
necessary: GIL serialization alone does not make a mimalloc heap cross-thread
allocatable. Domain descriptors outlive thread caches; backing survives the
existing transfer/abandon path and cross-thread free without retaining a dead
thread's heap pointer.

## Five-interface closure required in the actual source

| Interface | Required proof before native readiness |
|---|---|
| Typed allocation/wrappers | Preserve requested-byte debt before allocation and all known default/debug wrapper calls. Clear/reset the route across collection callbacks/queue work. Unknown wrappers and unbound/oversize cases decline future admission without disabling ownership of existing slots. |
| Reservation/publication | Reserve allocation and metadata first; existing object-start publication remains separate from traversal-safe constructor/frame state. Free an allocated unpublished reservation without reading its header. |
| Lookup/bounds | Resolve span membership before `_mi_ptr_page`, block geometry or usable-size. NO_SPAN differs from OWNED_INVALID; free slots, tails and bad boundaries cannot fall through to mimalloc. Raw debug bases and client objects have distinct accepted offsets. Shallow scans stop at one slot. |
| Inventory/rollback | Enumerate initialized occupied slots and retain existing classification/dead records. Metadata/cache pointers do not root payloads. A nonallocating fallback clears the actual visited MARKED/SHALLOW/DEAD prefix in both spaces after a real injected failure. Legacy mimalloc enumeration alone cannot do this. |
| Retirement/quarantine | Finish every referent-reader/deallocator phase before reuse. PyObject_Free/FreeDeadBlock and debug wrappers reach span-aware raw retirement. MiRealloc, quarantine_realloc, quarantine normal/OOM and release/unpoison must never send a slot to mi_usable_size, mi_heap_realloc or mi_free. Quarantine retains allocation/descriptor ownership until release. |

These obligations match concrete current hazards: gc_block_object resolves a
mimalloc page first; quarantine_realloc copies using mi_usable_size; quarantine
allocation failure directly mi_frees; quarantine release obtains usable-size
for ASan then mi_frees. The source must replace those assumptions, not merely
add a new allocation entry. Admission stopping during wrappers, detach or
shutdown must leave existing-slot ownership dispatch active.

## Mandatory 30-minute source checkpoint

One representation choice remains intentionally for the source attempt.
The plan says the existing current_object_heap route uses a “dedicated private
tag,” while backing uses existing NOSCAN and no new heap. Today
current_object_heap is a real mi_heap_t pointer, with no separate type hint.
The implementation must choose and close an explicit route representation:
a marker must never reach a real heap dereference/API, or one scoped typed
field must be saved/reset/restored at every actual reader/reentrant boundary.
Do not assume that an undeclared private heap or context already exists.
This is a source-checkpoint obligation, not implemented readiness.

At the same checkpoint:

- A declined admission must allocate through the ordinary GC-visible object
  heap; it cannot strand a coroutine in unregistered NOSCAN backing.
- NoScanMalloc does not promise 64 KiB alignment. Resolve actual base/extent
  geometry, or retain the original backing base when aligning within a larger
  allocation. NoScanFree receives only that real backing allocation.
- Define stride/debug prefix/tail/ASan redzones explicitly. `_PyMem_SizeClass`
  omits mimalloc padding; backing guards do not guard every slot.
- Pin empty-span release, index removal and descriptor lifetime after the
  last live/reserved/death-pending/quarantined slot, without further fallible
  work inside irreversible retirement.

An unresolved boundary remains an explicit blocker to native readiness; report
it at the checkpoint rather than bypass it to make the source appear complete.
There is no debug-only bypass, opaque audit exemption or native retry grant.

## Feasibility and decision

Ninety minutes is reasonable for a **bounded source attempt**, not a promise
that this cross-cutting allocator change becomes native-ready. The 30-minute
closure checkpoint should expose an unsuitable route/index/geometry early.
At the ceiling, freeze a complete review candidate or preserve the incomplete
patch and concrete missing boundary; do not extend silently or launch partial
ownership code. Exact-source review and strict build/root/linked audits still
precede native work.

The proposed debug/O3 cohort must witness both constructors, interior/native
roots, all lifecycle states, mixed live/dead spans, thread detach, actual
quarantine, reserve/index failure and a genuinely visited failed-mark prefix.
The from-birth ordinary-allocation source ablation is coherent: its positive
span witnesses must disappear without ever freeing a span slot as a legacy
block. It measures the combined ownership mechanism, not independent free and
allocation contributions.

Actual elapsed and absolute/parent/upstream memory decide after complete
behavior and ownership evidence, with fixed exposure and no adaptive samples.
Source counters do not prove cycles; static shorter paths do not override a
loss. Full qualification still precedes integration. These new scopes neither
reactivate cadence/adoption nor extend the old 04:18 boundary. No additional
design blocker remains on the exact final plan.
