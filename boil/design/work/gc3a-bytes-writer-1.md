# GC 3a: registered bytes-writer owner slot

Implementation checkpoint, 2026-10-29. The coordinator accepted author design
eed092a8 and authorized the focused source/test patch. The implementation is
ready for source review; no build, target execution or copied negative has run.
Readiness source: `pre-I-gc3a-writer-readiness.md` (877eced2). The coordinator
explicitly permits isolated development before ROOTS lands; eventual landing
must compose the ROOTS successor and qualify that actual union.

## Ownership and scope

Owned worktree `/home/power/code/cpython/.worktrees/boil-gc3a-bytes`, branch
`boil/gc3a-bytes`, starts clean at
`ce2ac8991eebb1bbb10698ff2d07a79bd4cba452`. Creation used 70 MiB with 214 GiB
available, reused the repository object store and created no build/cache.
Frozen ROOTS, H3 and primary runtime products remain untouched.

The proposed patch owns only:

- `Include/internal/pycore_bytesobject.h`: include `pycore_gc_trace.h` and append
  one `_PyGCRoot` to the private writer struct.
- `Objects/bytesobject.c`: register `&writer->obj` after initialization; unlink
  before terminal writer storage release on finish/discard.
- `Modules/_testinternalcapi.c`: one additive fixture near `gc_live_roots_of`
  and its method-table entry, using the already included private headers.
- `Lib/test/test_capi/test_bytes.py`: inherited writer lifecycle controls in
  `BaseWriterTest`, exercised for both `BytesWriterTest` and
  `ByteArrayWriterTest`. Both existing classes live in this file.
- This work record and later bounded evidence.

No Unicode writer, allocator mitigation retirement, queue, H3, compiler effect
classification or public ABI changes enter this slice. Public `PyBytesWriter`
is opaque; appending the node preserves existing member offsets and the
256-byte small buffer. All private consumers rebuild. Source layout arithmetic
on LP64 is 280 + 40 = 320 bytes; that is not a measured native sizeof result.

## Registered ownership and terminal handoff

After `obj = NULL` and other writer fields are initialized, call
`_PyGC_AddRoot(&writer->root, &writer->obj, 1)` before the first possible heap
buffer allocation. Negative size or writer-allocation failure has no node.
Every successfully allocated/registered writer reaches exactly one successful
finish or discard, including the existing initial-buffer-failure discard path.

The registered slot stays at one fixed address for the writer lifetime. Small
buffer use leaves it NULL; promotion stores the new owner into the same slot.
Existing resize helpers continue to update that slot and preserve their
allocation/reftracer protections. Raw data pointers remain derived addresses,
not registered roots. No existing opaque producer boundary is removed.

Discard resets its existing debug canary while ownership remains registered,
then unlinks before the existing decref/free terminal release. It must not leave
a registered node referring into freed writer memory. NULL discard stays a
no-op; no new double-discard guarantee is introduced.

Finish keeps the node through every allocating operation and final resize.
The heap path already executes `result = writer->obj; writer->obj = NULL`
after its final resize. Add only `_PyGC_RemoveRoot(&writer->root)` before the
common `PyMem_Free(writer); return result` tail. Small-buffer construction and
empty/singleton paths keep their existing opaque frame protection. No extra
production frame-root/span is proposed for this direct handoff.

The tail proof is specific: `_PyGC_RemoveRoot` uses HEAD_LOCK with
`_Py_LOCK_DONT_DETACH` (zero flags), unlinks, and HEAD_UNLOCKs. The contended
mutex path parks without thread-state detach or signal handling. Its unpark
callback `mutex_unpark` reads the monotonic clock and updates atomics; it runs
no Python callback. `PyMem_Free` is noncollecting under the existing MEM-domain
allocator contract. Therefore neither tail call introduces collection between
slot clearing and returning the result. This does not classify arbitrary lock
operations as noncollecting or change the call-effects checker.

`_PyGC_AddRoot` requires an attached current thread state and records its
interpreter. `_PyGC_RemoveRoot` does not require the creating thread; HEAD_LOCK
protects the interpreter list. Existing same-interpreter writer handoff between
externally serialized threads remains supported. Free-threaded Add/Remove are
existing no-ops; the added node does not invent collector semantics there.

## First deterministic control and bounded validation

Propose one `_testinternalcapi.gc_byteswriter_roots(use_bytearray)` fixture,
METH_VARARGS with the existing object argument/result convention. Do not copy
`gc_live_roots_of`'s METH_O PyValue signature. The fixture creates fresh heap
buffers beyond the small buffer, checks the registered-root class before and
after growth, performs collection, and verifies content. It also covers
small-to-heap promotion, successful finish, discard and deterministic oversized
creation/growth failure with cleanup. Successful finish results receive the
fixture's existing one-value root convention before any allocating diagnostic
or forced collection; that is a fixture obligation, not a production handoff
span. Return None after C checks, rather than exporting a buffer as an oracle.

The decisive assertion is `_PyGC_TraceLiveRootsOf(tstate, address)` containing
`_PyGC_TRACE_ROOT_INTERP`, not survival or the independent STACKS bit. Save only
the numeric buffer address for that query; the tracer marks each root class
separately and frees nothing. Require the INTERP bit to disappear after finish
while the separately rooted result remains valid. After discard/failure,
collect again to exercise unlink-before-free cleanup. FT skips only the
collector-class assertion; existing content/error/thread controls still apply.

The first negative is a copied mutation omitting the writer's Add and matching
Remove calls together, leaving the fixture unchanged. It must fail the first
registered-class assertion even if conservative C-stack scanning preserves the
buffer. Omitting both halves avoids substituting an uninitialized-node crash
for the semantic oracle. The copied control must use separately authorized
build products and never mutate frozen ROOTS or the eventual positive source.

After source review, propose focused bytes/bytearray writer tests on default
and debug with normal and kill verification; exercise the existing serialized
thread handoff. Include the deterministic omission control and allocation/error
cleanup cases. Additional build/profile/landing gates remain coordinator-owned;
no old ce2ac receipts qualify this patch or its eventual ROOTS composition.

## Decisions requested before the production patch

Accept the additive CAPI location/signature, the single registered owner slot,
and the noncollecting finish-tail proof above. Independent review should flag
any missing terminal path or root interface mismatch before implementation.
The next reviewable change is the focused source diff; this author checkpoint
is not self-approval or a native execution grant.

## Focused implementation checkpoint

The approved author draft is preserved as
`build/gc3a-bytes/design-approved-eed092a8.md` with its original eed092a8
identity. The source patch adds one private node/include, one Add and two
terminal Remove calls. Existing allocating resize/finish producers remain
opaque and unchanged. Discard resets the debug canary before unlinking and
then relinquishes its object and writer storage.

The additive VARARGS fixture exercises fresh heap creation and small-buffer
promotion, growth, deterministic oversized-resize failure while the old owner
remains valid, shrinking finish, exact result type/length/content, discard,
invalid finish and oversized initial creation cleanup. Both writer test classes
inherit its invocation. The INTERP-class assertion runs on the default build;
FT keeps the lifecycle/content/error checks. A separate thread root protects
the queried object while a failing registration assertion formats its error,
so the copied omission is expected to report the class failure safely. Finish
results enter the already-pushed fixture value root immediately on return.

Source inspection and `git diff --check` passed. No configure, compiler, target
interpreter, test, probe or benchmark command has run for this patch. Root
review of this concrete diff precedes coherent native builds and the copied
omission control; there is no new execution grant in this record.
