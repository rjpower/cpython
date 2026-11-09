# Independent span test-hook link correction review

2026-11-01. **ACCEPT source for fresh focused native feedback only.** Reviewed
clean commit `7d10d5bfd33cb7302df8008065c4dd637e453478`, parent
`34deae3b29260aea286b0c4017b963e092a2db62`, in
`build/worktrees/gc-integrated-spans`. The exact three-file diff has no remaining
concrete source blocker. Capsule `build/span-link-correction-v4/capsule.json`
has SHA256 `34ef3d487c3cc4ea92a924a745dfd58739965fca968c11412bc0fb8413dbfbbd`;
all eight bound artifacts match.

The v3 native failure exposed a module-link boundary missed in my v3 source
review: `_PyGC_Collect` is an ordinary internal `extern`, and the test extension
cannot resolve it in the supported hidden-visibility build. That failure occurred
before cases and earns no route or failed-prefix witness credit.

The correction exports only `_PyGC_SpanTestCollectWithRoute`, declared with
`PyAPI_FUNC(int)` in `Include/internal/pycore_gc_span.h`. `Objects/gcspan.c`
includes that declaration before its definition and is already in the core's
`OBJECT_OBJS`. On the supported Linux GCC/Clang path, `Include/exports.h`
maps `PyAPI_FUNC` to default symbol visibility; the existing executable link
uses export-dynamic. The extension includes the same header and calls the
narrow hook. `_PyGC_Collect` remains an unexported internal declaration, and
the extension now contains no call to it. The no-mimalloc implementation returns
success, preserving the previous conditional no-op behavior.

Independent host comparison proves the moved body is identical after removing
comments and adapting its Python-object return convention to integer status.
It saves the current heap and route, sets the scoped GC route, requests a manual
major, checks route restoration, and restores both saved fields before either
error exit. An existing collection exception is preserved; a route mismatch
raises the same assertion. The shim returns NULL on failure and None on success.
The existing core collection pass still clears the typed route during callbacks
and restores it afterward. No production allocator, marker, scheduling, test
body, case ID or build policy changed.

The prior 22-method host checker result (one existing expected failure) and six
source checks are retained and hash-verified, rather than repeated. The 20 span
methods and their literal children are unchanged. Independent receipt:
`build/span-link-review-20261101-v4/receipt.json`, SHA256
`2f9307ba32c3e1efef6bd519f6766b5ab8f5c23c8d2d0d727ca89d30e98728f4`.
No compiler, target, link inspection or native reproduction ran in this review.

Fresh separately authorized debug/release feedback must prove actual export and
import, callback routing, constructor-prefix MARKED/SHALLOW cleanup, and the
remaining focused cases. Preserve all v1–v3 failures and images. This acceptance
does not establish native success or performance. All GC execution remains
bounded by the user's **15:14:44 UTC** decision deadline.
