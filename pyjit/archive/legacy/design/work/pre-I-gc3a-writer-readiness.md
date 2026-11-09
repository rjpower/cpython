# GC3a: bounded writer/PyMem readiness audit

Source-only checkpoint, 2026-10-29. This adds concrete lifetime boundaries to
note11 section6.2, not implementation permission or a new execution plan.
Current ROOTS2b source/native qualification remains frozen; no source, helper,
test, build or probe was executed or changed. No held Queue2 work was retried.
Revalidate the actual ROOTS/H3 integration before assigning 3a implementation.

Inspected primary HEAD `57b5a61a6f26917d8e4878527c396d00cae58198` and ROOTS
`ce2ac8991eebb1bbb10698ff2d07a79bd4cba452`. The writer sources and three writer
headers below have identical bytes in both. GC root internals differ between
the branches; use the actual integrated root/span/checker interfaces, not the
primary source alone. Note11 was read at SHA
`3ebd7b5d90f4d67c0208691be1bec930fcea64586dcd1722226e11528ce0124a`;
PROCESS's inspected writer exclusions assign repair to3a and keep other debt.

| Common source | SHA256 |
|---|---|
| `Objects/bytesobject.c` | `9d55cde90b6515e56f89bb2c0b8a4c70661e125eb2947e3605143b39924353cf` |
| `Objects/unicode_writer.c` | `40feac81c97c950d664ea1335096e00e5ec1e9320c2f2ea35481eaa6b40b0f64` |
| `Objects/unicodeobject.c` | `117b0fd7bd00e0c781fa181de23a00d9233cdb55c4eb70928bbd775c4092a2f6` |
| `Include/internal/pycore_bytesobject.h` | `6568052660e92e642f3946fdf476d114a1852d79c12d7f27ed78b6f32a411d94` |
| `Include/cpython/unicodeobject.h` | `9380367be92a7e5223e2cefc08b863cf5a1c0089a6f9b7e5956602cf70260689` |
| `Include/internal/pycore_unicodeobject.h` | `03e2eea2f08dc2524ead3f5f157286959028af7c4afc5e4d123b18fa1f9c3c8e` |

ROOTS root header is `63bfca0186b99b9e8a9fb6a16f44eab0021b43adcaf63fe3d919a2fb583a57a9`;
gc_trace.c is `a992e22b36eb4c31031cddf1d92101c2cb91e6363de376cd678528648cccaa01`.
No current native result is evidence for the proposed repair below.

## Exact ownership gaps and handoffs

`PyBytesWriter` is a stable PyMem allocation, not a traced Python owner.
Its `obj` field is NULL while using the inline 256-byte buffer, then owns a
bytes or bytearray object (`pycore_bytesobject.h:105`). Creation initializes
NULL and may immediately allocate a heap buffer (`bytesobject.c:3864–3902`).
Resize first preserves old data through allocation/copy and then updates
`writer->obj` (`byteswriter_resize:3787–3854`, `_PyBytes_ResizeKeepOnError:3446`).
Derived `char *` pointers are recalculated by offset after growth at4089/4140;
they are views of the object, not replacement roots.

Finish is a separate ownership transfer (`byteswriter_finish_with_size:3936`).
It can resize bytearray, shrink mutable bytes, choose singleton/empty results,
or allocate from the small buffer, then detaches `obj` and frees the writer.
Error paths call Discard. Registering a root only after the first resize is
too late for creation, and unlinking before final allocating work is too early.
Publish the new object to its registered slot before another collecting call;
handoff must leave the returned result anchored by the returning frame until
the caller receives it. Remove a linked node exactly once before its owner
memory is freed on successful finish, failed finish and discard, including
allocation/overflow failure and bytearray error paths.

Unicode has both public heap-allocated and private stack/embedded writers.
`PyUnicodeWriter_Create` allocates exactly sizeof(_PyUnicodeWriter), initializes
it and creates the buffer lazily (`unicode_writer.c:81–104`). The private
`_PyUnicodeWriter_Init` is void and memset-initializes the whole structure at66.
`_PyUnicodeWriter_InitWithBuffer:120` independently memsets and calls SetBuffer;
it bypasses Init. SetBuffer and SetReadOnly (`pycore_unicodeobject.h:127/141`)
replace the owner and refresh data/kind/size or retain a shared readonly string.
Prepare at133–211 allocates, widens/copies, transitions readonly storage or
resizes compact Unicode. The old buffer must remain live through allocation
and copy; the replacement must become the rooted owner before later calls.

Critically, `_PyUnicodeWriter_Finish:552–553` moves the buffer to local `str`
and clears `writer->buffer` **before** final work. It can allocate a narrower
string for recheck_maxchar at582 or call `_PyUnicode_ResizeCompact` at597.
A permanent root of `&writer->buffer` alone misses these detached-result
paths. Transfer to a frame-scoped raw root before clearing, or keep the rooted
slot authoritative through finalization. That choice must cover unchanged,
readonly, singleton/empty, shrink/widen and allocation-failure exits. Finish
must leave the private writer reusable/empty as its current contract requires;
public Finish additionally frees the writer, while Discard calls Dealloc at644.
Recoverable Format errors keep a public writer alive for another write, as
`PyUnicodeWriterFormatTest.test_recover_error` and test_recheck_maxchar require.

`_PyUnicode_ResizeCompact` (`unicodeobject.c:1077`) and bytes in-place shrink
(`bytesobject.c:3394`) also have reftracer/realloc/new-reference boundaries.
Keep their present opaque/root protections until their own producer/result
ownership is proved. A writer root does not by itself prove that a newly
allocated result local or arbitrary raw input data pointer is anchored.
Likewise `%S/%R/%A` formatting creates temporary str/repr/ascii objects before
writer growth (`unicodeobject.c:2903–2951`); those producer locals are a
different obligation from the writer's persistent buffer.

## Smallest first boundary and existing interfaces

Prefer a first bytes-writer lifecycle repair over simultaneous compiler-unit,
Unicode-layout and all-PyMem retirement. Existing allocation-free embedded
`_PyGCRoot` ownership patterns are PyArena (`pyarena.c:67/149/179`), thread
bootstate (`_threadmodule.c:343/475`) and XIData (`crossinterp.c:371`). A bytes
writer is a natural stable owner for such a node over `&obj`, initialized with
NULL and registered before its first allocation. Preserve its small-buffer
layout contracts and exact default/free-threaded behavior. An owner must
approve the concrete field/header/lifetime choice before code changes.

Use existing `_PyValueRoot` raw roots/spans for frame-scoped handoffs, including
the integrated `_PyValueSpan_ObjectSlot` out-parameter interface when appropriate.
Note11 explicitly makes spans LIFO/frame-scoped and registered nodes suitable
for roots outliving a C frame. Do not store a LIFO thread-state root in a
public heap writer and let it escape its creating frame. Raw anchors preserve
actual object pointers; never canonicalize them into semantic numeric values.

Unicode needs a focused ownership/layout decision before implementation:
the private structure is exposed in a deprecated public header and is used on
stacks/inside codec contexts, while the public writer is opaque. Internal
embedded nodes add no allocation failure, but their representation and all
private initialization/finish/deallocation callers must be coherent. The public
`PyGC_AddRoot` pair allocates a raw node and can raise MemoryError; silently
calling it from void `_PyUnicodeWriter_Init` is invalid. Do not invent a new
root framework to avoid choosing between these actual interfaces. Inventory
the finite private callers, including InitWithBuffer and codec writers, in
that separate design. Keep Unicode exclusions until that ownership closes.

## Deterministic controls and affected qualification

Before broad stress, prove the root independently of lucky conservative stack
retention. A minimal existing _testinternalcapi fixture can hold a writer with
heap-sized buffer, expose only its buffer address as an integer, and query the
existing `gc_live_roots_of(addr)` before a forced collection. Registered roots
are visited from interpreter roots (`gc_trace.c:1541`); require that precise
root class. Old/no-registration control must lack it even if a C stack happens
to retain the object. Do not pass the object itself as the oracle argument.
This proves root registration, not absence of every stale root or complete
collection safety; content/lifetime and hazard controls remain necessary.
After growth/replacement require the new buffer's root; on finish/discard no
node may point into freed writer storage. Pair a copied omitted-root control
and an early-unregister-before-finish control with positive ownership cases.

Force collection at first allocation, small-to-heap growth, heap replacement,
Unicode readonly-to-owned widening and detached finish narrowing/shrink. Check
content and lifetime, OOM/overflow rollback, failed Format recovery, empty and
singleton results, bytearray paths, valid private-writer reuse and exactly-once
cleanup on supported success/error paths. Check misuse behavior only where an
existing documented API contract defines it; require no new double-free semantics.
Use current exact writer/resize regressions first, release/debug relevant
off/kill/stress modes and ASan for stale accesses; repeat only for a stated
exposure hypothesis. The historical stress crash is evidence, not a new test
run here. Existing tests cover bytes/bytearray Writer classes and Unicode
Writer/Format classes plus test_from_format; broader consumers include
unicode/format/codecs, binascii and struct, selected according to changed code.

A bytes-only fix can justify only its own exclusion removal after proof.
Compiler-unit metadata, the remaining PyMem inventory, raw-buffer owner
retirement, checker/readers3b and detached/queue state3c remain independent.
Do not remove transitive PyMem scanning, zero-init, struct scanning or release
barriers as a side effect, and do not alter the current2b qualification inputs.
