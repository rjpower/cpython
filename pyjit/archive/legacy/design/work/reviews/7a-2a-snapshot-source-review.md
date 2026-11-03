# Queue2a S3 snapshot/debug output: independent source review

2026-10-30. **SOURCE_CLEAR for bounded native validation** of the frozen
six-path candidate on `boil-queue2a-snapshot`, base
`7f4459bf2590e2aca4f249ab5ae5321d4bf2a3ac`. This is not native/test/landing
clearance or a proof that all collector callbacks preserve snapshots.
The coordinator owns execution grants and compatibility rulings.

Reviewed task evidence under `build/Queue2a-snapshot/source-v1/`:

| Artifact | SHA256 |
| --- | --- |
| `source-v1.diff` | `5e3e88fad4db168db5257cdca83b8fbb752d814b0ebeaf4ceb4d17b1a65005e6` |
| `checkpoint.json` | `505f18378c8459252758a3fc5ba403ff66ccc96d4a3b9c0b5c5ce5dd1ab9d430` |
| `gc_live.immediate-write-negative.c` | `dd73cd26eeb8c000dcbd07499e117f24f21eb20a8e9f1cf375d051848a3adf4d` |

Read the full diff, actual surrounding collector/weakref/walker/root and
hashtable bodies, PySys stderr implementation, all six proposed tests and
the copied negative. Independently rehashed all six source files against
the checkpoint; they match. `git diff --check` passes. No compiler, target
interpreter, native test, contention control or copied negative ran in this
review. The author's extracted-child syntax check is source-only evidence.
Preserved lock audit SHA79cea15a remains the historical baseline.

## Lock and ownership closure

All acquisitions of each affected private mutex are converted together:

| Mutex / acquisitions | Holder closure checked |
| --- | --- |
| `weakref_table_mutex`: `_PyWeakref_ListPtr` and `_PyWeakref_DropList` (`weakrefobject.c:76,118`) | Pointer hashing/direct equality, table/entry lookup, libc table allocation/rehash or steal/free/shrink. No key/value destructors, Python allocation, dependent CPython lock or explicit detach inside the ordinary holder path. |
| `ranges.mutex`: register, unregister, descriptor lookup, class lookup (`gc_walk.c:58,80,100,125`) | Fixed64-entry native record search/copy/delete, count/generation update. No allocation, callback, Python or dependent lock. |

Every acquisition uses DONT_DETACH. No old detaching waiter in the same
binary can receive a fair handoff and then require the collector's GIL to
unlock; changing just collector lookups would not have established that.
Both create/rehash and steal/shrink use the table's new libc allocator/free
pair. This removes the specific weakref→replaceable-RAW→tracemalloc TABLES
edge identified in79cea. It does not prove arbitrary allocator or tracer
callbacks elsewhere are leaf/non-detaching.

Noncreating lookup returns NULL for an absent HAS_WEAKREFS bit. A present
bit requires an existing table entry, with fatal invariant failure instead
of allocation/PyErr_NoMemory under the GC lookup. Creation still reports
OOM after unlocking. Static builtin types retain per-interpreter lists.
Process-global table mutation is mutex-protected across own-GIL domains;
individual list/object lifetime remains its domain's contract. Separately
allocated entries retain stable addresses across bucket rehash; no runtime
allocator migration is attempted. The intentional observability change is
private metadata bypassing public RAW/tracemalloc hooks, not a change to
public allocator APIs. It requires the already proposed compatibility
ruling and fresh processes/products, not mixing old/new allocator code.

## Snapshot and output lifetime proof

`collect` still computes/sweeps with the domain GIL. Selected registry
lookups/removals and debug output no longer introduce the two audited
reentry routes. It does not change order traversal, queue routing,
finalizer dispatch or the slice1 late-OOM rollback.

For the separate raw garbage array:

1. Its extent is fixed after batching; the unique garbage/SAVEALL
   construction is bounded by the signed original dead count. Before the
   cast to Py_ssize_t, the source asserts that bound. No later append to
   this raw array or array relocation occurs.
2. After `free(res.dead)`, a nonempty array gets one embedded `_PyGCRoot`
   before verification, uncollectable formatting or allocating publication.
   `_PyGC_AddRoot` is the existing nonallocating HEAD-protected interface.
   No callback lies between dead-scratch retirement and root publication.
3. Verification and every `PyList_Append(gc->garbage, ...)` see the rooted
   entire extent. Append OOM clears the error and breaks as before; no raw
   element is accessed after common cleanup.
4. Common `done` removes the root once, frees the array, then invokes stop.
   Successful empty arrays, mark failure, batch failure and late ordering
   rollback have no registered span; rollback sets garbage.n=0. An
   uninitialized root node is never removed. The node's stack address stays
   fixed for the complete registered interval.

The output batch is a stack-owned byte buffer in `collect_pass`, not a
pending list in live-state storage. `debug_cycle` copies type-name/address
text while valid; no deferred object/type pointer remains. Each bounded
local format becomes a complete NUL-terminated record. Checked addition
and geometric capacity growth are capped by PY_SSIZE_T_MAX. On growth
failure/overflow the retained prefix is written to C stderr, then the
current record, then all later records use C stderr. This path frees the
buffer, runs no Python/OOM callback and preserves within-pass order.

After `collect` retires arrays, invokes stop, clears collecting/frame and
restores its interrupted exception, `collect_pass` clears its entry context,
captures Python eligibility and flushes local bytes before queue draining.
The flush loop dereferences no saved live state across Python reentry;
nested/yielded collections own distinct buffers. Existing no-Python/closed
entry paths write directly to C stderr. Every normal return frees the
buffer, including early/no-work and failure returns from collect. Per-record
`PySys_WriteStderr` retains its error suppression, C fallback and raised
exception preservation (`sysmodule.c:4544`). Its format bound is respected.

Python writers deliberately run after stop/collecting reset; text and
within-pass record order survive, global ordering across concurrent/nested
collections is not promised. A writer can now recursively collect, as the
proposal discloses; its test uses a one-shot guard. C stdio/libc may block;
this is not a bounded-pause or allocation-success claim.

## Proposed controls and remaining qualification

The six new pure-Python subprocess cases exercise distinct contracts:
external stop-phase assertion (outside swallowed writer exceptions),
SAVEALL publication, yielded worker plus nested collection, second-growth
OOM with a retained prefix, writer/interrupted exception behavior and
inline no-Python C stderr. The OOM check requires the hook to fire, no
Python writes, unique object lines preceding exactly one stats line.
No previous test was removed or loosened. The integer-only fail hook is
independent of queue/order hooks and follows the existing internal pattern.

The copied negative restores immediate Python output only. Running just
the external phase case should fail the named
`debug writer before snapshot retirement` assertion; its writer records
phase/text and does not explicitly collect or dereference raw addresses.
Do not run the yielded/nested case against this deliberately unsafe copy.
A startup/import/compile error or crash would not be the intended negative.

Actual release/debug-kill, relevant existing GC/weakref/SAVEALL/queue and
inline coverage, copied named failure and justified sanitizer checks still
need reviewed launches/results. This source review does not establish
actual OOM/output counts, contention fairness/latency or root-walk outcomes.
The patch does not exhaust custom extension traverse/dealloc/reftracer
contracts, general buffer-release/detached-root debt, MARKED/OOM walks,
completion scheduling or close/fork ownership. Those limits prevent an
all-GC correctness claim but do not introduce a new blocker in this bounded
six-path source checkpoint.
