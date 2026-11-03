# Pinned domain and young enumeration: source ledger

Source-only proposal, not minor-collection authorization. Inspected clean
e9a1bcc65cf50c9316fc375500bcdaf4fb3a117b against frozen f2a5b3ca33d1.
The ten files hashed below are byte-identical between those commits. No
allocator/collector source or configuration changed; no new probe ran.

## Existing contracts and owners

| Boundary | Current caller/owner and behavior | Generations obligation |
| --- | --- | --- |
| Segment reservation | Objects/mimalloc/segment.c:871 calls `_mi_gc_segmap_reserve`; segment-map.c:173 reserves lookup leaves before publication. Failure frees the proposed segment and returns NULL. Publish after initialized metadata is at segment.c:989; removal precedes release at :420. | Reserve required young-index storage before an object can be published; segment reuse must not inherit age/remembered state. Allocation failure can remain an ordinary allocator failure at this boundary. |
| Object publication | Objects/object.c:2694 `new_reference` resets collector header flags, sets the object-start bit, then calls the ref tracer. pycore_gc_trace.h:231 supplies the bit set. `_Py_NewReference` has a void interface. | Set fresh-young membership before any publication/ref-tracer reentry. A fallible age registration inserted here cannot silently omit the object or report MemoryError through this interface. |
| Object-map allocation | obmalloc.c:1248 `object_map_reserve` uses anonymous mmap; :1263 installs global/chunk mappings with CAS. Mapping failure is fatal; racing unused mappings are unmapped. Map storage is outside conservative writable-data roots. | This is a fatal existing initialization contract, distinct from fallible segment reservation. A new index needs one explicit reserve/failure policy, not an assumed infallible growable vector at publication. |
| Free and address reuse | obmalloc.c:1675 `PyObject_Free` and :1701 `_PyGC_FreeDeadBlock` clear initialized-object membership before actual free, with sweep deferrals honored. | Clear young and remembered membership at the same actual-free boundary; same-address reuse must be fresh, not evidence of survivor age. Index addresses are nonowning metadata, not roots. |
| Resize | obmalloc.c:1642 `PyObject_Realloc` can collect first. It captures initialized-object identity, leaves old membership on NULL, and transfers the map bit after successful realloc. | Capture age before realloc; failure preserves old age/index. Successful move transfers age and index exactly once, including in-place resize and too-small result rules. Calling generic fresh-young registration would rejuvenate old objects and lose remembered-owner state. Store/resize closure remains separately required. |
| Domain capture | gc_trace.c:1668 `live_domain` includes either one interpreter or all main-allocator-sharing interpreters; max64 overflow fails. It captures each mimalloc subproc, bounds and per-tag debug offset. :3334 holds HEAD_LOCK and pins each captured interpreter before tracing. | Young/remembered enumeration uses this exact captured domain, not only current thread/current interpreter. No fallback to a partial domain on overflow. |
| Thread exit and reuse | pystate.c:2918 binds tagged heaps to the interpreter subproc; :2954 abandons all heaps before thread-state destruction. pycore_mimalloc.h:65 documents within-subproc reclamation. | Age metadata cannot be owned solely by a live tstate or vanish on heap abandonment. Track allocator storage by subproc/segment lifetime; reclamation preserves surviving-object membership. |
| Lookup/reconstruction | gc_trace.c:1729 `live_page_of` checks segment/subproc/tag/free-span membership; :1784 `live_block_of` checks page capacity; :1801 `live_lookup` adds debug offset and requires the start bit. Debug offset is `2*sizeof(size_t)` when debug hooks are enabled, not merely Py_DEBUG. | Reuse these checks to resolve indexed object addresses and block sizes. Do not infer object start from block contents or assume release offsets in Og/ASan/debug-allocator modes. Huge segments and object-vs-GC tags remain covered; MEM/NOSCAN are not object enumerators. |

## Proposed interface boundary

Keep `live_walk_heaps` (gc_trace.c:2204) as the full collector's enumeration.
It currently asks mimalloc for **every** active and abandoned OBJECT/GC block.
Adding an age predicate inside its callback still enumerates old blocks.
`live_build_pages` separately visits MEM **pages**, not object blocks; removing
that preparation is not implied by a young-object index.

The smallest useful minor interface is a nonowning young-start index with an
enumerator over only nonempty young words/chunks for the captured subprocs.
For example, a parallel bitset plus a nonempty-word summary can enumerate
young starts without visiting old object blocks or scanning the full global
object-start map. Representation and resident-memory cost need approval;
this is an interface proposal, not an approved bitmap layout.

Allocator/object owner supplies reserve-before-publication, fresh-init,
actual-free and identity-preserving resize transfer. Collector owner supplies
`visit_young(captured_domain, visitor)` using the existing validated address
lookup, and stages promotion/removal until the collection is committed.
Enumeration failure means keep everything and leave age/index state valid,
as the full mark's dead-list/OOM rollback does (gc_trace.c:3386). SHALLOW is
not promotion evidence: unsafe-to-traverse survivors stay young until an
independent safe traversal proves their outgoing-edge obligations.

The index must survive tstate exit and abandoned-segment reclamation, reject
foreign subprocs, and clear before address/segment reuse. It must not keep
Python objects alive through conservative scans. None of these are supplied
by the current initialized-object map alone.

## Reentry and pin limits: activation prerequisite

The existing pin protects interpreter ownership/routing, not an arbitrary
borrowed segment pointer across Python reentry. `collect()` unpins at
gc_live.c:1530, **before** sweep. DEBUG_COLLECTABLE then calls `debug_cycle`
at :1555 before sweep(:1559); its output can invoke Python. Thus committing
age/remembered metadata only after sweep and before the stop callback(:1610)
is too late on current source. New allocations/barrier writes and failure
rollback could otherwise be overwritten by a stale collection snapshot.

Activation must wait for the existing no-Python-dead-snapshot repair, or
supply a separately reviewed staging proof covering every allowed reentry,
owner lifetime and OOM path before that earliest debug write. This ledger
does not implement or waive S3/Queue2 work. Captured-domain pin/unpin and
post-callback mutation epochs must be explicit in that proof; a stop-callback
ordering rule alone is insufficient.

## Inspected source identities (SHA256)

```
643dff924abc57de6b63cab76c9b323d40fd06514f87914a3a691e8b3d134815 Objects/object.c
18c4fa46ab5c137d7b2ddaa350172cf1a1483b636e4b00d781efd4000dbb8bf6 Objects/obmalloc.c
a992e22b36eb4c31031cddf1d92101c2cb91e6363de376cd678528648cccaa01 Python/gc_trace.c
0833204bbf7c316aac1f7a3de48a1ae8548a85a041144b35379a9aa1cd11a83f Python/gc_live.c
e5c5634e9719406f1dd43aae8be0eb317b35dbf482f7b12d4307884f229b572e Python/pystate.c
63bfca0186b99b9e8a9fb6a16f44eab0021b43adcaf63fe3d919a2fb583a57a9 Include/internal/pycore_gc_trace.h
5f0e659a658e9a1009ad3e83d464d1f2643109badb6da57c0f2d0615921903e3 Include/internal/pycore_mimalloc.h
1f3af406eb49f37cd3f3a75a3163a12dfb490a9ccbcc8a649760c3a48ef3051b Objects/mimalloc/segment-map.c
935263e4584c07ed3fb9063ff17c91e7ad222fbb93f08a5656d0bdb8dc2c2fdc Objects/mimalloc/segment.c
38e5b82304f4d44c6bf25b3d548be998116ebb125891c64ad2017c91f09567f8 Include/internal/mimalloc/mimalloc/internal.h
```
