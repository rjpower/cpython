# Queue2b: owner metadata before retirement and adoption

Source-only readiness, 2026-10-30. Audited metadata bodies at
`boil-queue2a-sweep@7f4459bf2590e2aca4f249ab5ae5321d4bf2a3ac`,
against `7a-2b.md`, `7a-2c.md`, note13 sections3.4/5 and
`7a-lifecycle-prerequisites.md`. No implementation or native execution.

Coordinator implementation ruling: Watch/Unwatch guards are symmetric;
public type Watch/Unwatch reject ownerless static types with ValueError.
Global static `tp_watched` bits cannot represent interpreter-local IDs.
Heap immortals retain their allocator owner. The first increment covers
registration and destruction only; ordinary foreign mutation dispatch
remains required before a complete owner-safety claim. Implementation
starts from reviewed, not yet qualified, composed workstack `168f30a96ff`.

## Smallest coherent first slice

Implement registration/access guards AND owner-specific destruction
discovery together. Guards alone do not fix the existing discovery bug:
`_PyGC_TraceLiveMark()` builds watcher masks from the collecting interpreter,
and `live_walk_block()` passes that interpreter to destruction discovery
even while `live_walk_heaps()` is visiting another domain member's heaps.

1. Share the existing segment-to-mimalloc-subprocess ownership rule.
   A public API guard needs only an ownership classification/comparison
   with the attached interpreter. Perform that comparison under HEAD;
   never return an unpinned foreign `PyInterpreterState*` after unlocking.
   If a raw-owner helper is useful internally, require HEAD explicitly.
   Raise Python errors after releasing HEAD. No callbacks or allocation
   under HEAD, and no new owner registry.
2. Reject foreign non-reserved dict Watch and type Watch with the ruled
   ValueError; foreign Code_SetExtra raises SystemError. GetExtra on a
   valid foreign code returns success with `*extra=NULL`, without reading
   its extra array. Recommend the same ownership guard on Unwatch: a
   colliding foreign ID must not erase the owner's bit. This extends the
   explicit note13 registration wording and needs coordinator acceptance.
3. At each domain entry in `live_walk_heaps()`, select that owner's dict
   and type masks, including abandoned-heap visits; use the same owner
   for function/code watcher flags. Change `_PyCode_HasExtraFree()` to take
   that owner explicitly and update every caller/declaration. The mark
   domain already pins these owners through routing; no per-object public
   lookup or extra lifetime mechanism is necessary in this walk.

Preserve argument/ID validation and its current error precedence. No
metadata mutation or free callback on a rejected operation. This slice
does not add registration freeze, close drains, retirement or adoption.

## Object cases and decisions before code

| Case | Required treatment |
| --- | --- |
| Attached owner's ordinary heap object | Existing Watch/Unwatch/extra semantics remain. |
| Live foreign shared-heap object | Reject writes/bit changes; foreign extra reads report no extra. |
| Heap immortal | Still has an allocator owner. Immortality is not an exemption. |
| Static/non-heap object | No allocator owner exists. Handle explicitly; do not map missing segment to current. |
| Reserved dict watcher IDs | Preserve note13's exception and existing callback/ID validation. No automatic type-ID exception follows from it. |
| Isolated interpreter | Own metadata works unchanged. Do not construct illegal foreign own-GIL sharing to test the guard. |
| Closed/deleted/unknown heap owner | Do not read a possibly retired table or silently use main's table. Registration/access must fail closed until retirement/adoption defines the transition. |

**Static-type compatibility (superseded by the ruling above).** PyType_Watch currently accepts
static types; note13's heap/death owner rule does not define their
registration owner. PyType_Ready makes non-heap types immutable/immortal,
but that alone does not establish interpreter-local watcher-bit storage.
Preserving their current Watch/Unwatch behavior as an explicit static
exception is the smallest compatibility choice, with no claim of foreign
heap ownership. Rejecting ownerless registration is a separate API break.
Do not use `Py_TPFLAGS_IMMUTABLETYPE` as a static test: heap types can also
be immutable. No policy should exempt every immortal object.

**Ordinary mutation remains a separate boundary.** `_PyDict_SendEvent()`
and `_PyType_Modified_Unlocked()` read the current mutator's watcher tables.
An owner registering its own dict/type, followed by foreign C mutation,
can therefore still interpret colliding watcher IDs through the wrong
table. This first slice establishes registration and destruction
discovery, not all event dispatch. Before advertising complete watcher
ownership, choose an explicit foreign-mutation/dispatch contract. Do not
silently drop events or switch thread states inside these routines.

## Callback, allocation and error contracts

Keep SetExtra's publication-before-old-free behavior and invoke the old
free function outside the code critical section. A growth failure leaves
the old array/payload intact. A rejected foreign SetExtra invokes neither
owner's nor caller's free function. GetExtra's bad-object SystemError and
negative/out-of-range missing-slot behavior remain distinct from foreign
access. Keep the caller responsible for pointer lifetime after replacement.

Destruction discovery only inspects tables; it calls no watcher/free
function. Destruction dispatch continues on the queued object's owner,
outside HEAD. Preserve watcher-error reporting through the existing
unraisable path. A code-extra free function has no error return; retain
existing reentry/error handling rather than inventing an error result.
Existing whole-array freeing detaches `co_extra` before callbacks and can
call a registered free function with a NULL slot; do not change that
contract incidentally. Preserve the incoming exception as required by the
existing dispatch path.

## Files and causal controls

Runtime scope: `Python/gc_trace.c`, `Include/internal/pycore_gc_trace.h`,
`Objects/{dictobject,typeobject,codeobject}.c`, `Include/internal/pycore_code.h`.
Tests: existing watcher/code-extra modules and focused shared-interpreter
GC cases; only a necessary bounded test helper if existing APIs cannot
observe interpreter identity. Docs: C API dict/type/code pages, note13 and
the compatibility ledger. No pystate/lifecycle/mimalloc changes in this
first slice.

Use existing `stash_set/stash_get` (initialize its root on main), legacy
shared interpreters, `exec_interpreter`/existing interpreter execution,
watcher APIs and the `test_code.CoExtra` ctypes pattern. Keep free-function
objects alive until all tagged code is retired; payloads can be primitive
tokens without dereferencing raw memory.

- Give main/subinterpreter colliding IDs, then attempt foreign Watch,
  Unwatch, SetExtra and GetExtra. Assert exact exceptions/miss, unchanged
  owner bit/slot behavior and no free callback on rejection. Removing each
  guard must fail its corresponding assertion, not merely startup.
- Give the owner active dict/type/function/code watcher/free tables while
  the collector has none, and conversely; collect foreign-owner garbage,
  then service its actual owner. Assert correct owner/event/token exactly
  once. The old collecting-table discovery must fail this causal oracle.
  Existing `_testcapi/watchers.c` event lists are process-global: their
  contents alone do not establish callback interpreter identity.
- Preserve own-object replacement/free, invalid-ID/type/index, callback
  error and different-thread cases. Include reserved-dict success and the
  explicitly ruled static-type behavior. Test heap immortals separately
  from statics if a supported existing immortal helper can establish it.

These are proposed controls, not executed evidence or a new probe grant.

## Adoption dependencies still absent

The audited source has no registration_frozen, Code_ClearExtras, LiveAdopt
or subprocess-absorption implementation. Before shared READY/WAITING work
can transfer to main, complete all of the following:

- Freeze new owner-local registrations without closing dispatch admission;
  clear user watcher bits while preserving reserved bits.
- Retire code extras on their owner: detach before free callbacks, restart
  safely after allocation/collection/reentry/GIL release, including garbage
  code. Keep its tables alive until this finishes; never-opened/fork paths
  explicitly do not call free functions.
- Establish kind-independent RUNNING quiescence and reentrant-close policy;
  latch admission only at the successful stopped-world readiness commit.
- After pin waits, publish CLOSED and transfer shared READY/WAITING nodes,
  graph identities/edges/counts and completion ownership under HEAD. This
  is the proposed explicit revision of note13's strict foreign drain, not
  an implemented contract. Never transfer a RUNNING dispatch.
- Preserve/adopt allocator segments, immortal registries, kept-thread
  extents and registered roots; relink root owner pointers and absorb
  abandoned subprocess heaps with the reviewed lock order. Define isolated
  disposal and final main cleanup separately.
- Replace fork's current surrogate with reviewed no-Python child disposal;
  settle surviving dispatch-chain/quiescence policy without fake completion.

No Python/free function under HEAD. Registration guards are a useful
independent prerequisite; they do not make heap deletion, adoption, fork
or completion-graph activation safe by themselves.
