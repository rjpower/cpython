# S3: retire collector snapshots before reentry

Source design,2026-10-30. Isolated implementation5410 passes release focus
and affected modules. Debug inline audit catches an unmarked output helper;
annotation-only successor009fa2dcbe8 passes its function checker and is
rebuilding both configurations. Those builds/audits now pass. Six new child
tests need explicit parsed-mode forwarding because script_helper isolation
ignored their inherited environment; corrected test-only replacements are
pending. No child-kill, complete native qualification or primary landing is
claimed from those original six runs. Quiet-three timing is parked. This
increment addresses selected collector reentry paths, not Queue2a
completion scheduling or every remaining allocator/extension contract.

Inspected primary d6a23b9fe0cf85180896be59bc02433f3c2d2473. Its runtime
sources equal the qualified5a45 union. Reused
`7a-2a-safety-design.md`, the preserved sweep review and
`boil-queue2a-sweep/build/Queue2a-sweep/snapshot-lock-audit.md`
SHA79cea15a9ab837baa5d0e3b20f4cde6df66efb51bee4e09d028f67b41d951e6f.
The revised late-order/OOM slice1 remains a separate owner patch; this
proposal neither replaces its rollback contract nor grants a build.

| Compatibility change | Preserved contract |
| --- | --- |
| Python debug-writer calls move after the stop callback and collecting reset, before queued finalizers. | Text, debug flags, bounded per-record writes and exception fallback remain; no global ordering across nested collections is promised. |
| Private weakref registry metadata uses libc storage and is untraced by public RAW/tracemalloc hooks. | Object/list ownership, stable entry addresses, creation OOM and the public allocator interfaces remain; no claim extends to arbitrary allocator hooks. |

## Contract and two boundaries

A: from `_PyGC_TraceLiveMark()` returning until every `res.dead` entry and
prepared-order/dependency array is consumed or retired. Keep the shared
domain GIL throughout. No Python, explicit detach, safepoint or callback
which permits another same-domain mutator belongs in this interval. Domain
pins protect interpreter storage; they do not make the dead set immutable.

B: the raw `garbage.items` array remains a separate object-bearing snapshot
until publication to `gc.garbage` or abandonment. `free(res.dead)` does not
close B. Current `gc_live.c:1596–1620` runs DEBUG_STATS, verification and
uncollectable output before publishing that array. Protect it explicitly
with the existing allocation-free `_PyGCRoot`, or retire it before any
permitted reentry. Do both for a small, robust publication boundary: root
the fixed array while it is read/appended, then remove the root and free
it before debug-writer dispatch and the stop callback.

## One protocol for both registries

Change exactly the two weakref-table acquisitions at
`Objects/weakrefobject.c:70,103` and the four code-range acquisitions at
`Python/gc_walk.c:54,76,96,121` to
`PyMutex_LockFlags(..., _Py_LOCK_DONT_DETACH)`. Do not change general
`PyMutex_Lock`, HEAD_LOCK, parking policy or allocator-configuration locks.

The range holders only search/copy/delete the fixed64-entry native array
and update its generation. Preserve those operations and invalidation.
No holder may allocate Python objects, acquire a dependent CPython lock,
detach or reach a safepoint. Registration/removal and lookup share this
contract, including calls from a thread without an attached state.

For weakrefs, make the private table's existing allocator callbacks use
fixed libc malloc/free, with the current size ceiling and zero-size
semantics preserved. This is a specific metadata ownership decision:
table/entries/buckets no longer pass through public RAW allocator or
tracemalloc hooks. Document that cost/observability change. Both creation
and steal/shrink must use that same allocator pair; entries already use
the table's allocator. Pointer hash/direct equality and NULL destructors
remain fixed. In a fresh built process no cross-allocator migration is
needed; do not hot-switch an existing table. Merely substituting an
apparently default RAW helper is insufficient without auditing its debug
allocator context too.

Make `create==0` genuinely noncreating: absent HAS_WEAKREFS returns NULL;
present HAS requires the already existing table/entry. A missing flagged
entry is an invariant failure, not table insertion or MemoryError during
GC. The creating branch retains fallible creation and raises after unlock.
Static builtin types retain their per-interpreter state path. Entry
addresses remain stable across bucket rehash; the object's domain owns its
list and entry removal, the process mutex owns table structure.

Why all sites: `_PyMutex_LockTimed()` can hand ownership to a detached
waiter before parking returns and `PyEval_AcquireThread()` completes
(`lock.c:119–124`, `parking_lot.c:328–389`). A collector retaining that
same GIL while waiting on the handed-off mutex would deadlock. Changing
only lookup or only GC callers leaves this cycle. Every acquisition of
each private static mutex must use the new protocol in the same binary.

Why RAW cannot remain inside this lock: weakref steal can shrink buckets
(`hashtable.c:182–216,280–300`), and public RAW dispatch is replaceable
(`obmalloc.c:1178–1210`). Tracemalloc adds TABLES underneath it, while
`_PyTraceMalloc_GetTraces()` allocates Python containers under TABLES
(`tracemalloc.c:1476+`). Its reentrant TLS suppresses recursive tracing,
not automatic GC. A plausible TABLES→GC→weakref versus weakref→RAW→TABLES
cycle defeats a leaf-holder proof; it is not a reproduced deadlock.
Removing this particular edge is preferable to weakening tracemalloc,
whitelisting allocators or assuming short lock holders cannot detach.

## Per-pass bytes, not persistent pending state

`collect()` has one caller, `collect_pass()` (`gc_live.c:1505,1643`). Use a
caller-owned native `debug_output` value and a private out-parameter. This
is smaller than the design's pending list in `_gc_live_state`: no global
writer lock, interpreter-lifetime payload, new fini drain or queue state.

While A/B are open, retain today's bounded1000-byte local formatting and
append complete NUL-terminated records to checked libc storage. Include
collectable, stats and uncollectable lines in their existing within-pass
order. Copy type-name text and formatted addresses while objects are
valid; retain no PyObject, type-name pointer or deferred formatting work.
Check addition, growth and byte products. On buffer OOM/overflow, flush
the stored prefix and failed line to C stderr, free the buffer and use C
stderr for the rest of that pass. No dropping, Python error allocation or
Python write in this fallback. Native stdio can block and increase pause
time; there is no bounded-latency claim. Existing no-Python/closed paths
can use C stderr immediately.

After batching stops, `garbage.items` is fixed and its count is bounded by
the signed dead count. Register a stack-owned `_PyGCRoot` for its live
extent before verification/publication, using `gc_trace.c:4467–4500`.
The node and array address stay stable. Append errors retain today's clear
and stop behavior. On every success/OOM path, retire the raw array and
remove its root exactly once before invoking the stop callback. Unappended
objects need not be kept forever; no later debug line may dereference
them. Output bytes are independent of them. Do not use fallible public
`PyGC_AddRoot()` or claim libc memory is conservatively scanned.

Return the bytes from `collect()`. In `collect_pass()`, clear the collection
entry as today, capture whether Python/queue drain is allowed, and drain
the detached local records before the existing queue drain. Only local
bytes and the normal current thread are used by the writer loop; no
saved live-state pointer is dereferenced by that loop after Python reentry.
Free bytes on all exits. Keep PySys_WriteStderr's per-record error/fallback
behavior and interrupted-exception preservation (`sysmodule.c:4544`).

This deliberately moves Python debug writes after the stop callback and
collecting reset, but before queued finalizers. Preserve text, flags and
within-pass line order; do not promise global line order across nested or
concurrent collections. A writer can collect/release the GIL once the
snapshots are gone. A nested pass owns a different byte batch. A recursive
test writer must use a one-shot guard; no new global recursion scheduler
or writer lock is proposed.

## Small file boundary and validation

Runtime files: `Objects/weakrefobject.c`, `Python/gc_walk.c`,
`Python/gc_live.c`. Existing `pycore_gc_trace.h` root interfaces suffice;
do not change root representation, public allocation APIs or the checker
classification to make the patch pass. Tests belong in existing
`Lib/test/test_gc.py`; only if a specific OOM/control needs an integer-only
hook, use the existing `Modules/_testinternalcapi.c` + `pycore_gc.h`
pattern, separately reviewed. No general lock/provenance framework.

The first causal check needs no unsafe address fixture: with automatic GC
disabled and explicit manual collection, replace stderr with a one-shot
Python writer and record the existing stop callback. The writer records
whether the stop marker was already set at its first write; assert that
record **after** gc.collect returns, since PySys_WriteStderr swallows writer
exceptions. Under DEBUG_COLLECTABLE the old immediate path fails the
named `debug writer before snapshot retirement` assertion without nested
GC or memory corruption. Add a
DEBUG_STATS/SAVEALL case requiring the saved objects are already published
before writer entry; do not inspect raw objects from formatted addresses.
After that boundary check, let the writer yield to an ordinary thread
which collects, then perform one nested collection. Check completion,
saved-object identity and normal callback/finalizer counts. A copied
restoration of the immediate write must fail the same named phase oracle;
the historical S3 segfault/ASan failure stays preserved, not rerun by
default. Root registration is an additional source-lifetime proof; the
phase test alone is not a fail-old oracle for omitting that span.

Exercise buffer OOM after a real retained prefix, requiring ordered C
output and no Python writer for the remainder, plus writer exceptions,
no-Python inline collection and supported success/error cleanup. Review
all six real lock sites and both weakref allocation/retirement closures;
a host source check can reject reverting any acquisition or restoring RAW
callbacks. That is a protocol-coverage check, not a deterministic native
handoff/deadlock reproduction. Reuse existing weakref, range/StackWalk,
tracemalloc and interpreter concurrency tests for affected behavior.

After source review: bounded output tests and existing GC/weakref/
tracemalloc/range/queue lifecycle tests in normal release and actual debug
kill; one ASan reentrant-output case for the concrete lifetime hazard.
Record actual states, skips, zero fallback, cleanup and stop on unexpected
failure. No blanket20-repeat matrix or new timing experiment.

## Explicit limits before claiming whole-GC safety

Fixed registry allocation removes one specific RAW nesting edge. It does
not prove all sweep deallocators or arbitrary replacement allocators safe.
`_PyReftracerTrack` raises `in_tracer`, which prevents this thread's automatic
collection, but does not prevent explicit detach or mutation by an
arbitrary C tracer (`pycore_object.h:69–85`). Public profiling documentation
does not yet state the stronger no-detach/edge-observational contract.
Keep that existing contract/enforcement work explicit; do not silently
bless custom tracers, buffer callbacks or custom RAW hooks. Default
tracemalloc's reentrant/free guards are evidence for its actual body, not
for all callbacks. Completion ordering, close/fork waiting graphs,
MARKED-on-OOM cleanup and platform closure remain separate GC increments.
