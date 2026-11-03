# Precise edge validation and a possible release fast path

Read-only review of frozen union `17efeeac9f860afe7928b42453150144f353e545`,
2026-10-30. No runtime edits, tests, builds or profiles were performed.
This note does not change that candidate's qualification or authorize another
backend. Post-landing profiles must establish whether this cost is material.

Precise managed fields have a stronger contract than conservative words.
The current implementation still sends both through allocator lookup. That
is a possible simplification boundary, but removing all validation from every
`visitproc` call is not yet justified. A smaller prospective cut can avoid
lookup on already completely marked objects and immortals, while retaining
the existing validation before every new mark and SHALLOW upgrade. Gate even
the header read with alignment and the existing object-start map; static
referents stay on the old path initially.

## Current work and its purpose

In `Python/gc_trace.c`, `live_precise_child:1977` serves both native
`live_visit` and direct managed cell/tuple/frame value loops. After a debug
encoding check it calls `live_lookup(op, interior=0)` unconditionally.
The lookup does more than test the object-start map:

| Layer | Actual check |
| --- | --- |
| `live_page_of:1758` | Segment-map bounds and membership; allocator sub-process in the collection domain; metadata/free-slice exclusion; page coverage and object/GC heap tag. |
| `live_block_of:1817` | Block start reconstructed from page start/block size, including capacity bounds. |
| `live_lookup:1834` | Pointer is exactly block plus allocator debug offset, then has its initialized-object bit. |
| `live_mark:1909` | Immortal and existing-mark handling; precise upgrade of a SHALLOW mark; work-stack push and sticky OOM. |

Conservative words need the address interpretation: they can be integers,
interior pointers, raw PyMem carriers, stale addresses or allocator metadata.
Native traversal and explicit root adapters also use the precise visitor;
their current interface reports an object value without declaring a trusted
managed layout/publication class. Keeping that slow boundary preserves the
existing compatibility and diagnostic contract.

The object map is not sufficient on its own. It is process-wide and contains
no domain, heap-tag, extent or debug-offset information. Its bit granularity
is eight bytes, so `op + 1` shares an object's bit; only the exact lookup
rejects that unaligned address. `_Py_NewReference` sets a bit after header
initialization, not after every field is safely published. The deliberate
`_testcapi.pyobject_init_outside_heap` fixture can set a bit for a libc block
in release, while debug rejects that invalid allocation. These are reasons
not to treat a bit test as a general object validator; the malformed cases
are not counterexamples to a correctly declared exact-field contract.

## Obligations that remain

- **Exact active identity.** A trusted field must contain NULL, an inline
  value or an aligned, live object-start pointer with a readable initialized
  header. `PyValue_IsObject` distinguishes encodings, not allocation identity.
  No integer, interior address or borrowed pointer whose lifetime ended is
  admitted by calling it precise.
- **Statics and immortals.** Static objects are legal referents and commonly
  have no bitmap bit. They must not acquire heap mark bits. Heap immortals
  are skipped by `live_mark`, but their outgoing references are independently
  visited by `_PyGC_LiveVisitRoots` (`gc_live.c:278`); registered static types
  have their own root traversal (`gc_trace.c:2065`). An early heap-immortal
  return cannot replace those root paths. Static referents fail the bitmap
  gate and retain the original lookup behavior.
- **Domain and stability.** `live_domain:1702` still supports allocator
  domains. The declared profile rejects new subinterpreters
  (`pylifecycle.c:2623`), but an `ndomain == 1` result alone does not prove
  that every process-wide map bit belongs to it. Keep domain validation on
  first discovery; any stronger shortcut must name the supported main/GIL
  runtime and preserve the no-Python/no-detach marking interval. No new
  assumption about dormant foreign/adoption behavior follows.
- **Publication.** An initialized header, an active object start and safely
  traversable fields are separate states. Conservatively discovered partial
  objects need `live_safe_to_traverse` and SHALLOW scanning. Known managed
  ranges provide a finite starting point: cells initialize their value before
  tracking, tuples zero their elements, and `_PyFrame_Initialize` initializes
  the frame header/remaining slots before use. The ordinary-frame range
  explicitly excludes interpreter-owned entry frames. This is not a proof
  for arbitrary extension `tp_traverse` bodies or every constructor.
- **SHALLOW.** `MARKED` is not enough to skip an edge. A precise edge to
  `MARKED|SHALLOW` must clear SHALLOW and queue real traversal, as it does now.
  Otherwise the retained object's referents can die.
- **Debug/verifier behavior.** Preserve inline bits in a `PyObject *` field
  rejection, allocator-offset checks, dangling/dead-byte diagnostics, killed
  object quarantine and verifier A/B attribution. Header-first access to a
  freed or ASan-poisoned object would change those diagnostics. Keep debug,
  verifier and initially debug-allocator paths on the unchanged validator.
- **OOM and state lifetime.** Mark-before-push and SHALLOW upgrade can still
  encounter stack allocation failure. `push:846` sets sticky `t->oom`; the
  heap walk clears marks and failure returns no dead inventory
  (`gc_trace.c:3454–3493`). No shortcut may report a successful partial mark,
  drop a failed push, reuse marks across collections or bypass this rollback.

## Smallest future design

After qualification, first inspect the existing profile for exact-child
lookup and duplicate-edge cost. If material, use the existing core-managed
range entry point, not a new registry or a public traversal ABI change.
Select an eligible release/main/GIL/nonverifier path once for the mark.
Limit that choice to the ordinary production root-mark phase, with debug
allocator hooks off; diagnostic root-class marks and other mark-bit users
keep the existing path.

For a value from a known initialized cell, nonempty exact tuple or ordinary
frame range: skip NULL/inline encodings as now; require pointer alignment and
the existing object-map bit before reading the header; return only for heap
IMMORTAL or `MARKED && !SHALLOW`. Any failed gate, unmarked object or SHALLOW
object uses the unchanged `live_lookup` and `live_mark`. Keep native
visitproc, explicit roots, statics/root enumeration and conservative scans on their existing
paths initially. No new mark or flag mutation happens before validation.

For those two return cases, the source proof is local: current `live_mark`
already returns before adding stack-hit/counter records or pushing work.
The new route therefore avoids repeated address validation, not required
traversal or callback work. The first visit still validates identity/domain;
SHALLOW still upgrades; all marking/OOM bookkeeping stays centralized.
The bitmap is only a gate before header access, not permission to accept
ownership or create a mark. It combines with the private managed-field and
current-mark contracts; it does not replace them. Those contracts and per-mark
mode selection need independent review before implementation. An extra bitmap
read/header branch can also hurt mostly-first-visit graphs, so there is no
speedup prediction here.

The current-mark proof must be phase-scoped. New objects have collector flags
cleared by `new_reference` (`object.c:2661`). Every object marked by the
ordinary root phase first passed domain/exact lookup; the proposed shortcut
sets no new bits. The subsequent heap walk clears MARKED/SHALLOW on both
success and sticky-OOM failure. Under the main-only/GIL/no-detach interval,
no other interpreter's mark or concurrent mutator can install a competing
mark. Thus a fully marked target in an admitted managed field can use that
existing mark during this phase; `ndomain == 1` by itself is not the proof.

There are concrete counterexamples to using MARKED as a timeless identity:
`gc_live.c` also uses it as a journaled scratch bit in lifecycle cycle/order
traversal (`:765–881`), and as SWEPT after deallocation (`:1353–1377`). Those
phases do not call this production root-mark helper. Keep the shortcut out
of them, and preserve their journal-before-mark/clear-on-OOM rules. The next
ordinary mark starts only after the collection/retention/sweep transaction
has ended. First discovery still reaches `live_mark` and the same heap walk,
so reachable-again finalizer/legacy state reset and resurrection handling
remain unchanged. Immortal and fully marked returns currently occur before
`nmarked`, `verify_list` and `stack_hit`; preserving that order preserves
these counters. Verifier A/B modes continue through the full original path.

Stale, unaligned or tagged words are never promoted to this managed contract
by a successful address test. Unknown inputs stay in conservative/native/
root/debug routes. Within a managed range, unaligned inputs fail the cheap
gate; normally freed objects have their map bit cleared before reuse and
thus fall back before header access. A stale address reused for a different
active object is not distinguished by today's full lookup either. This is
not a general corrupted-pointer defense: an invalid outside-allocator
initialization can leave a misleading map bit. Native inputs retain full
validation, and corrupted core fields remain a contract violation rather
than a legal precise edge. Keep debug/verifier detection unchanged.
Debug-kill quarantine can poison the header itself, which is another reason
to exclude that mode. Ordinary release validation before the first mark
still uses the actual object pointer/debug offset rather than assuming
block equals object. No release lifetime guarantee changes for valid fields.

Full removal of page/domain lookup for newly reached objects is a separate
larger proof. It needs explicit source/publication/allocator guarantees for
every admitted edge, static exceptions and domain ownership; the current
shared `visitproc` signature does not express them. Movement is unnecessary
for the smaller cut and must not be smuggled into it.

Relevant existing falsifying controls include raw blocks/segment metadata,
exact versus interior lookup, huge-segment lifetime, static-type readiness,
allocation safepoints, verifier shallow mode and work-stack recovery after
OOM. Preserve their public lifetime/data oracles and debug diagnostics.
No new control or campaign was run or added in this review.

Source anchors: `Python/gc_trace.c:1640–2069,3394–3493`;
`Include/internal/pycore_gc_trace.h:173–247`;
`Objects/object.c:2661`, `Objects/obmalloc.c:980,1246–1303,1640–1710`;
`Objects/cellobject.c:11`, `Objects/tupleobject.c:64`;
`Include/internal/pycore_interpframe.h:269–315`;
`Lib/test/test_gc.py:2240–2295,3245,3637,3700`.
