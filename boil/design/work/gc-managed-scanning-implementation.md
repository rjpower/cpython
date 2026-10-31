# First direct managed scanners

2026-10-30. Implemented sourceb9d829 is included in qualified0b15/integrationaee46.
[As-built record](gc-managed-scanning-1.md) and the
[combined performance report](gc-post-cleanup-performance.md) supersede this
proposal's pending qualification. The source-only checkout is retired with
verified packet archival and pushed branch history preserved. The implementation
below uses existing cell/tuple/frame layouts; it changes no collection policy.

## Concrete first cut

Specialize live marking for exact cells, nonempty exact tuples and the existing
physical frame value arrays. Keep the general `visitproc` protocol for native
types and diagnostic traversals. Do not introduce a universal layout table,
generated layout DSL, type-ID cutoff or function-pointer slot visitor.

The current path is `live_drain` → `_PyGC_TraceTraverse` → `tp_traverse` →
`_Py_VISIT_VALUE` → `live_visit` → `live_lookup` → `live_mark`
(`Python/gc_trace.c:959,1941,1970`). Cells have one canonical `PyValue ob_ref`;
tuples have `ob_item[0:Py_SIZE]` (`Include/cpython/cellobject.h:11`,
`tupleobject.h:10`). `_PyGC_VisitFrameStack` scans a physical value extent,
not an IR liveness map (`Python/gc_live.c:2281`). These three cases need no
per-element callback or allocation to describe their strong value fields.

Use one tiny internal range value: `{PyValue *base; Py_ssize_t n}`. Put its
definition beside the existing internal value/root structs and add three
static-inline accessors beside the existing layout helpers:

| Accessor | Exact definition | Preconditions |
|---|---|---|
| Cell values | `&cell->ob_ref`, length1 | Exact `PyCell_Type`; initialized value or NULL |
| Tuple values | `tuple->ob_item`, length `Py_SIZE(tuple)` | Exact `PyTuple_Type`, nonempty; safe initialized extent |
| Frame values | `_PyFrame_GetLocalsArray(frame)`, `_PyFrame_NumSlots(frame)` | Existing initialized-frame admission; physical extent, including NULL slots |

These are field/range definitions, not registered roots or lifetime owners.
They return addresses, never box/canonicalize values and never allocate.
Current generic cell/tuple/frame-array traversers consume the same accessors,
so their bounds cannot silently diverge from the new fast loop. Empty tuples
retain their existing path. Tuple subclasses and other shapes retain the
general traversal regardless of layout similarity.

## Direct live loop, with current validation retained

Factor the body of `live_visit` into a private static-inline precise-child
operation taking `(trace_t *, PyObject *)`. Keep `live_visit` as its thin
`visitproc` adapter. The direct range loop performs its tag check, obtains the
raw pointer for an object-valued slot, and calls that concrete operation.
The compiler sees both bodies; there is no indirect call for each array edge.
The loop is conceptually:

```c
for (Py_ssize_t i = 0; i < range.n && !t->oom; i++) {
    PyValue value = range.base[i];
    if (PyValue_IsObject(value)) {
        live_precise_child(t, PyValue_ObjectBorrow(value));
    }
}
```

`live_precise_child` initially preserves the existing exact `live_lookup`
and debug invalid/dangling-field checks before `live_mark`. A published object
map bit alone does not prove membership in this mark's interpreter/domain,
an active allocation, correct debug offset, or the current runtime. In
particular, do not follow a trusted-looking object pointer directly into its
header simply because its source slot belongs to a managed tuple. Raw/interior
words continue to use the existing conservative lookup. No domain/interior
lookup shortcut belongs to this slice.

`live_drain` still marks the type/header edge first. Select exact cell or
nonempty tuple once per popped object, then call the concrete range loop.
Preserve runtime-owned attributes and heap-type edges. The current exact
cell/tuple layouts have no instance dictionary or shape attribute payload;
make those assumptions explicit in debug assertions. If those layout facts
later change, the fast case must visit the new edges or fall back. All other
objects still call the complete `_PyGC_TraceTraverse`, including its special
static-type path, non-GC heap-type edge and managed-attribute handling. Calling
an extension's `tp_traverse` alone would omit those runtime-owned edges.

The existing non-live verifier/diagnostic traversal remains generic. This
preserves a useful independent edge source for checking the specialized path;
it is not a second authoritative layout description because the array bounds
come from the shared accessors.

## Frames without duplicate enumeration

Do not merely optimize `_PyGC_VisitFrameStack` while still passing `visitproc`
for each element. Its generic signature must remain for native and diagnostic
callers; add a live-only range loop at the actual live root enumeration.

Factor `gc_trace.c:1388–1449` into one private frame enumerator with a callback
once per ordinary frame and the existing one-value visitor for an
interpreter-owned return slot. Its adapters are:

- Generic: the current `visit_frame` and `_PyFrame_Traverse` behavior.
- Live: the same fixed header edges, then the direct frame value-range loop.

Factor the fixed `_PyFrame_Traverse` header portion into a private helper in
`Python/frame.c`/`pycore_frame.h`; both adapters use it. Globals/builtins remain
the root frame visitor's additional edges. This first slice can keep callbacks
for that fixed small header set; it removes the callback for each register.
The frame enumerator's callback is per frame, not per value. Avoid duplicating
the pushed-frame/chunk walk or adding an intermediate frame vector.

Preserve all current admission details: skip a pushed header whose
`f_executable` is NULL; include pushed but unlinked frames; avoid double-visiting
thread-owned frames from the active chain; include running generator/coroutine
frames outside the data stack; visit **only the return slot** for an
interpreter-owned entry/base frame. Its other fields can be poisoned. Continue
to scan `co_framesize - FRAME_SPECIALS_SIZE`; neither untracking nor a partial
opcode-effects decoder is a liveness map.

The frame-array range helper is reusable by a future updater. The fixed header
helper still reports values through `visitproc`; it is not yet a complete
writable frame-header map. That later consumer must supply explicit locations
or pin those edges before moving anything. This proposal does not claim that
frames or all C aliases are relocation-ready.

## Future update loop and semantic boundaries

A copying collector will consume the same ranges with a separate statically
bound loop: tag-check the slot, obtain a forwarding result, and write the
updated pointer bits back to that slot. Do not add an unused updater now.
In particular, marking must not normalize raw numeric boxes into inline values;
future updates preserve tags and the storage identity required by raw anchors.

Ranges describe strong fields only. Weakref registry entries, borrowed caches,
exported buffer aliases and referrer-dependent native cleanup do not become
strong fields to simplify a scanner. Tuple cached hashes are scalars. Frame
headers contain mixed pointer/value fields and cannot be reinterpreted as one
homogeneous array. A writable backing slot also says nothing about an already
borrowed interior C pointer: movement still needs publication, pinning/reload
and complete alias proofs.

The GIL/no-detach marking interval is unchanged. The accessors do not acquire
locks or extend object lifetimes. Use the existing construction safety and
SHALLOW-to-deep checks before popping/traversing an object. Do not scan fresh
uninitialized tuple items merely because a start bit exists; the new path
must accept exactly the objects the current generic path safely traverses.
The region packet's stronger initialization proposal is separate work.

## Small file cut and verification

| Files | Change |
|---|---|
| `Include/internal/pycore_structs.h` and existing cell/tuple/interpreter-frame helpers | Range type and three field/bounds accessors; no allocator metadata |
| `Objects/cellobject.c`, `Objects/tupleobject.c` | Generic traversal uses the shared range definitions |
| `Python/gc_live.c` | Existing generic frame-array traversal uses the frame range helper only; no lifecycle change |
| `Python/frame.c`, `Include/internal/pycore_frame.h` | Share fixed frame header visitation |
| `Python/gc_trace.c` | Concrete inline precise-child/range marking; exact-layout dispatch; shared frame enumeration with generic/live adapters |
| Existing GC/cell/tuple/frame/generator test modules | Observable edge survival, frame entries and mixed inline/object coverage |

Merge landed native cleanup first and coordinate the small `gc_live.c` range
refactor with that owner. Do not split root files or introduce a registry as
part of scanner implementation.

Meaningful checks should cover a cell as the only strong owner of a mutable
payload; tuples mixing NULL construction slots, inline numbers, raw object
values and nested container cycles; exact tuple versus subclass fallback;
and a value held only in an ordinary, overlapping-call, suspended or escaped
frame slot through collection. Preserve exception/return/entry-frame and
callback-reentry controls. Use existing child lifetime/kill mechanisms so a
second Python local or conservative address does not accidentally rescue a
missing edge. Existing generic referent enumeration can compare child
identities on fully initialized fixtures; combine that with actual survival
checks, not only duplicated loops that agree on the same mistake.

The source checker and linked audits must see honest noncollecting effects,
not a new whitelist for unknown scanners. Run the appropriate normal/kill
affected controls after source review, preserving the existing generic
diagnostic path and allocation/layout behavior. No native jobs are requested
by this source packet.

Keep the precise-child helper and range loop small. Prefer ordinary static
inline initially; inspect actual optimized assembly before forcing inlining.
Inlining the whole generic traversal or duplicating `live_lookup` at many
sites could increase code size and instruction-cache pressure. The desired
compiled property is no `visitproc` call per common array element, with one
shared field definition and unchanged validation. Report code-size change and
sample/elapsed effects separately: current mark attribution does not isolate
callback dispatch, so no predicted gain or new benchmark campaign follows
from this plan.
