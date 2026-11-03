# GC3a Unicode writer: unqualified source checkpoint

Isolated managed worktree boil-gc3a-unicode, branch boil/gc3a-unicode,
base f2a5b3ca33d1. This is an unqualified source checkpoint, with no configure,
build, target execution, probe, benchmark or native qualification. Disk before
creation had 212GiB free. Frozen parent, a285 finalizer source/products and
quiet-four-v1 inputs remain unchanged. The coordinator accepted source design
3cd46d77 before this implementation; source/native review remains separate.

## Implementation and ownership

The deprecated private _PyUnicodeWriter type/API block moved from the public
Unicode header into pycore_unicodeobject.h. Public PyUnicodeWriter stays
opaque. Core shared-module declarations remain exported where required.
The existing InitWithBuffer helper now has internal PyAPI_FUNC visibility
for its lifecycle check in the shared _testinternalcapi component; no public
declaration was added.
pycore_unicodeobject includes the existing _PyGCRoot definition; pycore_long,
abstract.c and StringIO now include the private definition explicitly. This
intentionally requires external private users to rebuild/migrate to the opaque
API; there is no compatibility layout or public root allocation in void Init.

Init links an embedded root over buffer, including while NULL, and sets an
explicit active bit. InitWithBuffer uses that same initialization/registration
before SetBuffer, preserving its old min_char0/min_length=size defaults.
The writer/node must stay at the same address until Finish/Dealloc. Buffer
replacements retain one node; recoverable write/Format errors retain the
session. No changes were made to Format rollback, resize/reftracer internals,
producer precision or the collector's root implementation.

Finish keeps buffer in the registered slot during narrowing/shrinking; each
replacement is published there immediately. It detaches the final raw result
only after allocating work and the existing consistency check. RemoveRoot,
reset and public PyMem_Free are noncollecting under the existing HEAD_LOCK
_DONT_DETACH/MEM contracts, so the result goes directly to the caller. Finish
failure and Dealloc clear the buffer, unregister iff active, and reset fields.
This preserves Finish→error-Dealloc and FinishWithSize→public-Discard cleanup
without double removal. Private reuse is explicit Finish/Dealloc→Init, never
memset/re-Init of a live node or direct writes to an ended writer. Public
Finish consumes its allocation as before.

## Finite owner inventory

The 21 existing private lifetimes and their success/error endpoints remain
unchanged; a host source audit compared their lifecycle call sequences.

| Owner | Instances | Existing terminal contract |
| --- | ---: | --- |
| unicodeobject.c: FromFormatV; UTF7; UTF8; UTF32; UTF16; UnicodeEscape; RawUnicodeEscape; ASCII; internal escape decoder; Charmap; translate; str.__format__ | 12 | Finish/FinishWithSize on success; Dealloc on errors. UTF8/ASCII use InitWithBuffer. Empty/fast bypasses precede Init. |
| abstract.c inline-float formatting; complexobject.c; floatobject.c; longobject.c | 4 | Finish success, Dealloc error. |
| unicode_format.c percent-format context | 1 | Stack context passed by pointer, Finish/Dealloc. |
| stringlib/unicode_format.h recursive str.format builder | 1 | Each recursive call owns a distinct local writer; early recursion check precedes Init. |
| multibytecodec.c decode, incremental decode, streamreader read | 3 | Writer is embedded in per-call stack buf, not persistent copied codec state. Finish and error-Dealloc (including after failed Finish). |

Helpers in unicode_formatter.c/numeric formatters/CJK headers/ISO2022 borrow
these addresses. No live aggregate copy/move/realloc was found. Existing public
Create uses a stable PyMem allocation; JSON/StringIO/test adapters pass its
pointer. Public Finish/Discard unregister before freeing it. StringIO's
existing buffer traversal and exact-str helper are retained. No never-buffered
post-Init exit is permitted to skip Finish/Dealloc.

## Test adapter and lifecycle source

The mixed writer adapter moved mechanically from _testcapi/unicode.c to the
new _testinternalcapi/unicode_writer.c component. Module/type registration,
Unix source selection and Windows project/filter entries follow the existing
component pattern. _testcapi still includes only public headers and now has
an opaque create/write/finish/empty-discard smoke. Two Python factories now
use the relocated type. Existing ctypes Format calls/get_pointer are unchanged;
no new address bridge, root oracle or memory fixture was added.

The new private lifecycle case covers never-buffered Dealloc and repeated
cleanup, InitWithBuffer, a wide buffer narrowed/truncated by FinishWithSize,
Finish→Init reuse, empty Finish and Finish→Dealloc. It checks state/content,
not independent root-class registration. Existing writer/format assertions
are preserved, including memory-error recovery, singleton, read-only,
changing kind, FinishWithSize and Format recheck cases.

Host-only source review checked: exact adapter relocation after module-name/
deprecation/local-declaration transforms; unchanged ASTs for all 28 existing
writer/format methods other than the two factories; no private writer in the
public header/public CAPI source; Windows XML/CRLF and Unix registration;
unchanged original 21-owner lifecycle call sequences; git diff --check.
These are source checks only, not executed-test, compilation or runtime proof.

## Proposed affected qualification (held)

After independent source review and explicit build/launch grants, use owned
coherent O3 release and O1 Py_DEBUG builds, each with newly built matching
stdlib/extensions. First whole test_capi.test_unicode once off/N0, then once
actual-kill/N0, using existing module/worker/state/root criteria and zero
fallback when collections occur. O1 is the coordinator-approved prospective
focused debug cohort; existing f2/e9 Og products stay frozen. Protect the two new cases and the existing
recover_utf8_error, memory_error when supported, readonly_optim, change_kind,
singletons, finish_with_size, Format recover_error/recheck_maxchar, and
existing test_from_format. Report actual counts/skip names; do not invent
execution credit for unavailable allocator/ctypes/debug capabilities.

Next consumer selection is whole test_codecs, test_multibytecodec, test_str,
test_format, test_io.test_memoryio and test_json. These cover InitWithBuffer,
codec callbacks/early exits/embedded contexts, numeric/str formatting and
persistent public writers. Run only under separately reviewed actual modes,
limits and normal global admission. Any stress/ASan or causal registration
negative needs its own concrete source/selection review; no registration
oracle has been implemented or claimed here. All old exclusion/GC debt remains
until the actual evidence justifies its scoped change. No Queue2/S3, PyMem
retirement, general resize or generational implementation is bundled.

The coordinator reviewed production lifecycle, headers/includes, the exact
ResizeCompact failure/consumed-success paths and adapter registration. The
InitWithBuffer visibility fix is included. Independent source review remains
in progress; this source commit does not authorize native work or waive the
actual combined-candidate qualification. A later composition may share one
fresh integration ladder rather than repeat complete suites per component.
