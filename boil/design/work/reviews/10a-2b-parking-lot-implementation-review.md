# ROOTS2b Park prerequisite implementation review

Development source review, 2026-10-29. No native execution or runtime edit by
the reviewer. Qualification and landing require separate evidence review.

## V1: FIX_FIRST

Reviewed ROOTS merge head `d8be2e2c36d8d7651b728598f165c9324ae128f4` plus
the four-file frozen patch
`960f610e20dbbf0dcbbe7a88b4e6e5be1a89df69de17f49547efd5bc40e8d7b2`.
Paths for the retained patch and source map are
`build/takeover-20261029/park-prerequisite/implementation-v1.patch` and
`implementation-v1.json` in the ROOTS worktree; the source map hashes to
`13c6bf0a82e9ff73d71398a2b57d6e8a5f8e396de01b8dffaef16d477ce48923`.
All four current file hashes and the actual binary git diff match that source
map and patch. The preceding primary `64ab` merge changes 27 files, all under
`boil/`; it changes no native source before this patch.

The production hunk matches the approved design: only Park becomes precise,
with its caller-owned storage contract documented in the internal header.
All callers remain opaque. The caller/root proof is preserved in
[the design review](10a-2b-parking-lot-design-review.md).

The real-Park fixture otherwise meets that design. It notifies readiness and
immediately enters actual Park while holding the GIL, validates a pinned
capture whose PC is in Park, checks the exact opaque caller slot and value
before and after unpark, and verifies the same capture and no fallback. The
caller-precise negative expects that exact slot to disappear. The second-walk
comment correctly claims payload publication/unpark while unattached without
claiming the consumer has reached GIL acquisition. The C cleanup guards
uninitialized descriptor use with `registered`, restores registration and
unparks on exits; the Python finally releases and joins. Sentinels prove
structural coverage, not object GC survival.

Two bounded corrections remain:

1. **Fixture API mismatch.** `Modules/_testinternalcapi/gc_walk.c:859`
   declares `gc_walk_park_observe` with a `PyObject *` argument and return,
   but line 1062 registers `METH_O`. This fork requires
   `PyValue (*PyCFunctionO)(PyObject *, PyValue)`
   (`Include/methodobject.h:23–26`; `Objects/methodobject.c:579–583`). Use the
   proper value signature/conversions and method cast, or a correctly
   declared ordinary VARARGS hook. Current True/False values use singleton
   pointer bits (`pycore_value.h:164–169`), so Boolean calls may accidentally
   work on the current one-word ABI. There is no demonstrated crash; the
   initial prediction of guaranteed invalid dereference was corrected.
2. **Actual release gate0 inline-audit failure.** The retained
   `park-prerequisite/g0-default-v1.log`, SHA256
   `08836fc682fd393132def300f434886a15e00a65bf4b335cca7485ba08f0e52f`,
   reports that precise Park inlines
   `_PySemaphore_Wait`, whose body fails parsing at `parking_lot.c:126`:
   `millis = (DWORD) div`. This is the cast, not the `PyTime_t` declaration
   at line 120. Linux header discovery does not supply the Windows SDK's
   `DWORD`; the parser treats `(DWORD)` as an expression and rejects the
   following `div`. `PyTime_t` is already in TYPE_WORDS. The smallest proposed
   correction adds only the known scalar type `DWORD` to the existing
   TYPE_WORDS inventory, with a realistic cast positive and a paired
   inactive-Windows-branch unanchored-object negative requiring the precise
   held-local diagnostic. HANDLE declarations already parse through the
   generic declaration path; no HANDLE cast in this body needs an inventory
   addition. This checker/test scope needs the coordinator's explicit grant.

Do not omit inactive branches, waive effects, add an outlined allowance or
weaken the root diagnostic. The old parser must fail the cast positive; the
fixed parser must still reject the held-local negative. Recheck the complete
semaphore body and linked inline audit after the correction. Preserve v1,
its release gate failure and the incompatible API finding. Debug gate0 was
still active at this review checkpoint; no outcome is inferred. Apply any
correction only at a quiescent boundary, then retain a new source freeze and
review its exact delta.


Coordinator disposition, 11:42 PDT: the exact DWORD-only parser inventory
addition and TestRootCheckC cast-positive/held-object-negative extension are
granted. Proper PyValue METH_O correction is also authorized at quiescence.
No implementation-v2 source clearance or native result is implied by these
grants. The findings above remain historical and no user approval is pending.
