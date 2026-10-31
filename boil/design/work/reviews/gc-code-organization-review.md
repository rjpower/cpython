# GC code organization and root contracts

Source review at primary `9c3c0d574246`, 2026-10-30. No runtime edit, build,
native test or timing. This supplements [architecture §7](../gc-architecture-next.md#7-code-design-and-the-interfaces-we-will-improve).

The current root mechanism is small and useful. Keep its two lifetime models,
the common traversal wrapper, contiguous scratch arrays and rooted callback
queue. The main organization problem is that root enumeration, allocator
interpretation, marking and historical verification share one large interface
and implementation. Introduce writable locations and space access with their
first real nursery consumers; do not precede that work with a mechanical split,
another root registry, or a generic cleanup framework.

There is no `pycore_gc_roots.h` or `gc_roots.c` today. The actual files are
`pycore_gc_trace.h`/`gc_trace.c`, plus value-span and stack-walk interfaces.
The trace header's opening describes a verifier running under reference
counting and freeing nothing. That is historical prose, not the live contract.
Its mark-result API and implementation now drive reclamation.

## Actual interface map

| Responsibility | Current source | Contract to preserve |
| --- | --- | --- |
| Temporary explicit roots | [`_PyValueRoot`](../../../../Include/internal/pycore_structs.h#L65), [value helpers](../../../../Include/internal/pycore_value.h#L334), thread chain in [`gc_trace.value_roots`](../../../../Include/internal/pycore_tstate.h#L78) | Caller-owned LIFO records; only initialized `base[0:n]` is visible; no allocation on push/pop. |
| Registered non-object storage | [`_PyGCRoot`](../../../../Include/internal/pycore_gc_trace.h#L72), [add/remove and public wrappers](../../../../Python/gc_trace.c#L4469) | Caller owns node/array for the entire registration; head lock protects list membership. Internal registration is allocation-free; public single-slot registration allocates and can fail. |
| Root enumeration | [`_PyGC_TraceVisitFrames`](../../../../Python/gc_trace.c#L1418), [thread state](../../../../Python/gc_trace.c#L1470), [interpreter state](../../../../Python/gc_trace.c#L1503), [collector roots](../../../../Python/gc_live.c#L278) | Frames, spans, persistent runtime fields, registered arrays and queued work have explicit visits. Conservative fallback remains separate. |
| Object edges | [`_PyGC_TraceTraverse`](../../../../Python/gc_trace.c#L959) | Includes static types, heap-type edges for non-GC instances and runtime-owned attributes, as well as `tp_traverse`. Calling only `tp_traverse` is insufficient. |
| Native frame precision | [`pycore_gc_walk.h`](../../../../Include/internal/pycore_gc_walk.h#L11), `gc_walk.c` | Source rooting promise plus linked code-range/CFI classification; opaque frames retain conservative scanning. |
| Object identity/storage | [typed allocation](../../../../Include/internal/pycore_object_alloc.h#L16), [`new_reference`](../../../../Objects/object.c#L2694), [bitmap](../../../../Include/internal/pycore_gc_trace.h#L188), [lookup](../../../../Python/gc_trace.c#L1729) | Heap/tag/debug-offset interpretation and a published start bit distinguish objects from raw/free blocks. Free/reuse/realloc must maintain that identity. |
| Mark result/lifecycle | [`_PyGCMarkResult`](../../../../Include/internal/pycore_gc_trace.h#L314), [mark entry](../../../../Python/gc_trace.c#L3307), [collection](../../../../Python/gc_live.c#L1731) | Complete dead inventory, per-record/aggregate facts, domain lifetime, safe retention on preparation failure. |

## Adding a root safely

A temporary C borrow should first use an existing strong frame slot or traced
owner if that owner actually survives and retains the field across reentry.
Otherwise use the existing span primitives. This schematic uses actual APIs;
`work` denotes the caller's operation, not a new helper interface:

```c
PyValue slots[1];
_PyValueRoot span;
_PyThreadState_PushValueSpan(tstate, &span, slots);
_PyValueSpan_AppendObject(&span, owner);
int status = work(owner);  /* may collect; owner is anchored */
_PyThreadState_PopValueRoot(tstate, &span);
return status;
```

Link before the collecting call and pop on every success/error exit, after
the last use. Push/pop order is checked. The caller supplies capacity;
append stores the value before increasing `n`. An object out-parameter can
write directly into an initialized raw anchor using
[`_PyValueSpan_ObjectSlot`](../../../../Include/internal/pycore_value.h#L392).
After successful backing-buffer realloc, call `_PyValueSpan_Rebase` before
another collecting call; failure keeps the old buffer/span. No registration
may outlive its stack record or storage.

Use `AppendObject` for a raw box or an object whose storage is borrowed.
Canonicalizing an exact numeric box can yield an inline scalar and preserve
the value while losing the storage anchor. Raw anchoring spans must not be
“normalized” into canonical semantic values. `gen_clear_frame` is a compact
[real scoped-root example](../../../../Objects/genobject.c#L155).

For non-object storage retained beyond one C activation, initialize its slots,
then register the array with an embedded `_PyGCRoot`. Remove the registration
before freeing/moving the node or array. Internal add/remove do not allocate;
the public `PyGC_AddRoot(&variable)` can fail and needs an already valid
lifetime through registration. The [SAVEALL publication path](../../../../Python/gc_live.c#L1831)
shows a fixed array protected across Python allocation, with common-path
removal. Removal can occur without an attached thread; adding requires one.
The list lock does not authorize racing writes into the registered payload.

A heap field is an edge, not a reason to add another global root. Visit it
through its owner, including separately allocated arrays. Add persistent
thread/interpreter fields to their corresponding visitors. Weak caches need
invalidation semantics, not accidental strong visits: current marking
explicitly [excludes borrowed caches](../../../../Python/gc_trace.c#L1644).

### Precision is not relocation

`Py_GC_PRECISE` permits the walker to omit conservative words in that C frame.
It promises that live objects are anchored elsewhere. It does not describe
the writable location of every C local, authorize rewriting arbitrary pointer
words, or prove that a caller reloads a moved pointer after a safepoint.
The current span visitor passes child values to `visitproc`; it does not
update the original C alias. An address in a registered array is physically
writable, but its current API is still a lifetime contract, not a promise
that all aliases obey movement.

[`root_check_c`](../../../../Tools/cases_generator/root_check_c.py#L1)
checks a supported C subset using dataflow, liveness and call effects. It
distinguishes strong mutable fields across allocation-only calls from immutable
edges across reentry. Unknown calls are conservative. The
[shared effects table](../../../../Tools/cases_generator/call_effects.py#L1)
is a useful existing concept; keep one authority. Its allocation-only category
depends on [`_PyGC_LiveCollectHere`](../../../../Python/gc_live.c#L2022)
deferring Python delivery. Do not weaken it to accommodate a new path.

The [linked inline audit](../../../../Tools/build/gc_inline_audit.py#L1)
checks compiler inlining/outlining against the source promise. Retain actual
backend/CFI/range verification and runtime lifetime controls. Neither the
source checker nor a “precise” section is a writable-root map. Any future
relocation checker needs an additional reload/pin proof, not an annotation
rename or blanket noncollecting whitelist.

## Data structures, ownership and failure

| Structure | Owner/lifetime and failure rule |
| --- | --- |
| `addrset` / `rangelist` | [`gc_trace.c:111`](../../../../Python/gc_trace.c#L111): private open-addressed sets, at most half full, and sorted range vectors with binary search. Useful for diagnostics, mem-carrier deduplication and snapshots; no hash entry is required for each ordinary live mark. |
| Initialized-object bitmap | Sparse anonymous mappings; one bit per eight address bytes. [`ObjectMapChunk`](../../../../Objects/obmalloc.c#L1263) currently treats mapping failure as fatal. It is not a fallible nursery reservation API. |
| Mark stack / carrier ranges | [`trace_t`](../../../../Python/gc_trace.c#L411) owns libc scratch; push/growth records `oom`. [Finalization](../../../../Python/gc_trace.c#L2403) releases it. Object headers carry MARKED/SHALLOW; the verifier additionally owns sets/provenance. |
| Dead array | The mark allocates/grows a contiguous `_PyGCDeadObject` array; the caller owns/free it. [Failed discovery](../../../../Python/gc_trace.c#L3384) clears partial DEAD flags and returns no usable dead set. Domain pins still need caller cleanup. |
| Retention/order work | [`objstack`](../../../../Python/gc_live.c#L586) has sticky failure and bounded growth. [Ordering scratch](../../../../Python/gc_live.c#L1189) is separate, with its own allocation failures and cleanup. Do not replace these with one container unless failure/ownership contracts match. |
| Callback batches/queue | [`call_new`/batch/splice](../../../../Python/gc_live.c#L310): private libc nodes; a batch transfers ownership to the interpreter queue. [`run_queue_ex`](../../../../Python/gc_live.c#L510) publishes active op/callback/exception slots before unlinking/freeing the node. Thread yielding or recursive collection must not create a root gap. |
| Debug output | [`debug_output`](../../../../Python/gc_live.c#L1440) owns bytes, not borrowed objects. Its Python writer runs after dead/garbage snapshot use. OOM preserves ordered prefix/fallback behavior. |
| Weakref heads / walk cache | Weakref hash entries deliberately have stable head-slot addresses; the frame cache is a separate open-addressed map with loader/range invalidation. Keep both distinct from allocation ownership and generations. |

Collector-private libc storage avoids recursive managed allocation and accidental
conservative roots. That is a specific implementation contract, not permission
to classify arbitrary RAW allocators or callbacks as harmless. Keep discovery
OOM, optional diagnostic truncation, irreversible callback publication and
allocation-metadata failure distinct. They do not share one rollback rule.

## Five shared concepts, with their first real consumers

1. **Writable reference locations.** Introduce a narrow internal visitor for
   actual `PyValue *` and `PyObject **` slots, alongside the existing value-only
   `visitproc`. Start with thread value spans and the first admitted cell/tuple
   layout. Marking can adapt locations to the existing value visitor, so it
   remains useful before movement. Preserve raw-anchor representation; classify
   weak/readonly/ambiguous edges separately. Every incoming alias must reload
   or pin, with the complete pin set known before relocation.
2. **Space lookup/enumeration.** The region allocator is the concrete second
   consumer. Extract the exact/interior lookup and enumeration boundary from
   `live_page_of`/`live_block_of`/`live_lookup` and heap walking. Return validated
   object start, allocation extent, owning space and construction/pin facts;
   retain direct mimalloc fast paths. Include owner queries, debug offsets,
   exited heaps, manual free/realloc and address reuse. Do not forge mimalloc
   page structures or create an independent live-native-object registry.
3. **Construction publication.** `new_reference` clears flags, sets the start
   bit and emits CREATE; that is not proof that every payload field is ready
   for typed traversal. Current [SHALLOW handling](../../../../Python/gc_trace.c#L1870)
   preserves unsafe constructor storage and scans its words. The first region
   constructors need explicit reserve/publish/ready contracts. Reserve metadata
   while failure remains returnable; do not reuse today's fatal map extension
   as relocation OOM handling. Pinning/page retirement does not justify age
   promotion; initialized-prefix and remembered-state commit precede reentry.
4. **Lifecycle facts.** Keep actual work in existing dead-record kinds and
   aggregate bits. The selective branch refines their meaning; update old
   type-capability comments when integrating it. Native cleanup eligibility
   belongs beside the operation being shared (`gen_close`), not in a new
   registry of arbitrary type cleanup callbacks. Facts can select a stage;
   dynamic DEAD/readiness/observer guards still need revalidation.
5. **Preparation and commit.** Name the boundary at the new native-clear and
   relocation consumers. All storage preparation and cohort validation finish
   before mutation; commit has no allocation, Python execution or detachment.
   Existing callback publication can already be partial, so preserve its
   delivery contract rather than inventing universal rollback. A small local
   prepared-state record can own existing scratch; it need not become a
   reusable transaction framework.

## Organization and implementation ownership

Keep the current native-cleanup patch in its five reviewed code/test paths.
Its shared generator predicate/clear and `gc_live.c` orchestration are the
next small source consumer. Avoid touching marking layout at the same time.

Then assign these finite slices with one owner per shared interface:

| Slice | Owned files/boundary | Reviewable endpoint |
| --- | --- | --- |
| Space/region backend | `pycore_object_alloc.h`, `obmalloc.c`, `object.c`, allocator-facing `gc_trace.c` functions; layout constructors by agreement | Existing full mark sees both spaces; publication/free/reuse/OOM and interior lookup agree. No minors yet. |
| Slot visitors/root consumers | `pycore_value.h`, `pycore_structs.h`, root declarations/enumeration in trace header/C, first admitted layout | Marking through slot adapters agrees; alias/reload/pin and initialized-span rules are explicit. |
| Core stores/remembering | Container/frame/type/handler owners and generators, after shared visitor/space signatures freeze | Actual owner stores/constructor promotion close the old-to-young ledger; generated products follow handwritten inputs. |
| Minor/relocation orchestration | `gc_live.c` plus narrow mark/space adapters, after preceding contracts | Independent full young-liveness oracle, complete pin discovery, precommit OOM and finite major debt. |

`gc_trace.c` is overcoupled: it includes root enumeration, allocator maps,
marking, historical verifier algorithms, live retain/kill provenance, test
fixtures and diagnostics. `trace_t` combines these states. Extract a root
header/visitor family when the slot visitor has two consumers; extract heap
access when the region backend exists. Keep unwind/platform classification in
`gc_walk.c`. Later move verifier-only diagnostics/fixtures if doing so removes
dependencies from the live mark context. A split must preserve shared root
and object-edge semantics, not fork two independently maintained mark engines.

`gc_live.c` usefully owns scheduling, lifecycle selection, resource ordering and
reentry policy, but preparation and irreversible work are interleaved enough
that a new path is easy to place too early. Improve those boundaries locally
as native cleanup and relocation arrive. Do not first build a scheduler, generic
cleanup-slot protocol, allocator plugin system, new root manager or universal
graph/container library. Existing arrays, bitmaps and short linked queues fit
their jobs. The missing shared concepts are location, space, publication and
commit contracts, with explicit consumers and ownership—not more registries.
