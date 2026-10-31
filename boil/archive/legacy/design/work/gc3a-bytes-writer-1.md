# GC 3a: registered bytes-writer owner slot

Bounded validation checkpoint, 2026-10-29. The coordinator accepted author
design eed092a8 and source patch 5c115979, committed as f5e818b0454. The
release, O1-debug and owned ASan checks below are complete; the copied omission
fails the intended class oracle. Actual-result review and eventual ROOTS union
qualification remain coordinator-owned. No landing or performance claim follows.
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

At the source checkpoint, source inspection and `git diff --check` passed;
no native command had run. Root then accepted the concrete diff and granted
the bounded builds and controls recorded below. That source-only status is
historical; the original patch snapshot remains unchanged.

## Actual bounded validation at f5e818b0454

Every positive native product below was built from clean committed native
source `f5e818b0454764be06968b463b600820553ccbe3`. Later changes to this record
are documentation only and do not relabel binary source identities. GCC is
`/usr/bin/x86_64-linux-gnu-gcc-15`; source-specific retained frame pointers,
leaf frame pointers and automatic zero-initialization remain enabled.

| Owned build | Actual configuration | Build outcome |
| --- | --- | --- |
| default | `CC=gcc`, NDEBUG/O3 with symbols | PASS |
| debug | `CC=gcc --with-pydebug CFLAGS=-O1 -g`; final O1 overrides default Og, assertions/symbols retained | PASS |
| asan | `CC=gcc --with-address-sanitizer --with-pydebug`, existing default Og | PASS |

Builds used make -j4, one global machine slot, the existing build lock and a
2,400-second ceiling. Each retained exact command/environment, compiler hash,
Makefile/pyconfig/config status/log hashes and complete product maps. All
three checksharedmods steps reported 67 shared modules and zero import
failures; ten unavailable optional modules remain explicit in build logs.

The existing main/worker actual-state and result observers were copied to the
owned evidence directory unchanged. The ASan successor changes exactly its
cohort assertion from excluding to requiring `--with-address-sanitizer`
(diff 8b012c79; helper cbc6d28c). Both driver and worker assert actual mode,
extras and stress. Completed root journals include the separate auxiliary
process records; no blanket mode-observation claim is made for custom children.

| Transaction | Native result | Tests / skipped | Collections | Fallback / unpinned |
| --- | --- | ---: | ---: | ---: |
| release, whole test_capi.test_bytes, off/stress0 | PASS | 52 / 10 | 59 | 0 / 0 |
| O1-debug, whole module, kill/extras0/stress0 | PASS | 52 / 0 | 73 | 0 / 0 |
| release, writer classes, off/stress1000 | PASS | 38 / 10 | 1,405 | 0 / 0 |
| O1-debug, writer classes, kill/extras0/stress1000 | PASS | 38 / 0 | 1,519 | 0 / 0 |
| ASan debug, writer classes, kill/extras0/stress0 | PASS | 38 / 0 | 73 | 0 / 0 |

The writer selection is exactly `*BytesWriterTest.*` plus
`*ByteArrayWriterTest.*`: 21 bytes and 17 bytearray cases. The ten release
skips are test_canary_byte, test_get_data, test_get_data_canary, test_grow_canary
and test_resize_canary on each class, with their existing debug-only reasons.
No selected ASan case is excluded. ASan used
`detect_stack_use_after_return=0:detect_leaks=0`; combined stdout/stderr reports
success with no unexpected sanitizer diagnostic, not leak freedom.
Every test transaction retained equal complete native pre/post inventories.
No normal/stress transaction was repeated.

The causal control is a separate managed source tree/branch
`boil-gc3a-bytes-negative` / `boil/gc3a-bytes-negative` at
`d16f5e9258e7f2b9f0c312b8abb50c1e93047d59`, built independently with the
same O3 default configuration. Its only source delta deletes the writer's
one Add and both Remove calls. Header/layout, C fixture and Python tests
remain byte-identical. Both new tests fail with
`AssertionError: create: writer owner not registered` (native regrtest exit 2),
without crash, import or compilation failure. Actual off/stress0 journals
pair, with 43 collections and zero fallback/unpinned. This establishes the
registration oracle independently of conservative-stack retention.

An optional release-kill attempt was invalid: startup rejects kill without
Py_DEBUG before Python initialization, native exit 1, zero tests. Preserve its
original adapter FAIL receipt plus separate INVALID_CONFIGURATION_NO_TESTS
interpretation; it is not a runtime correctness failure. The coordinator
ruled against an extra O3-debug build. O3 strict reclamation is unestablished.

An admission mistake also remains explicit. Initial whole-module positives
and the causal negative passed a task TMPDIR to old gate_run, which therefore
used private slot files. Their native/correctness results remain exact, but
there is no retrospective global test-lock claim. Builds used global admission.
The prospective correction leaves gate TMPDIR unset and sets target TMPDIR
through `/usr/bin/env` inside the gate command; both stress jobs and ASan use
global `/tmp/boilgate-slot.N` admission with fresh target cache/tmp. A live
debug-stress driver FD9 observation confirms global slot2. The coordinator
accepted retaining prior results without rerunning them solely for admission.
The shared primary gate fix is not imported into this frozen cohort.

Receipts/config/product/raw-output hashes are indexed in
`build/gc3a-bytes/checkpoint-summary.json`. Initial a79b806d is preserved as
`checkpoint-summary-before-asan-a79b806d.json`. ASan launch binding is d632bf6e,
actual test plan 1ef5473d, JUnit c7e77dd0, combined log 28187ccc, state1855ecbc
and products09ce831f. All requested jobs are terminal; no broader native
cohort, filter retirement, Unicode or general PyMem repair follows from this
checkpoint. Actual ROOTS successor composition, corresponding qualification
and landing review remain required before integration.
