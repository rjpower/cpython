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

## V2 fixes and structured semaphore proposal

Reviewed retained `implementation-v2.patch`, SHA256
`c7114e5f6b8af4fda3d228efc1fc9c93fc1242da329455e97cebb20d1d5a8fe0`,
and its source map `implementation-v2.json`, SHA256
`91970a6bca0773abf2d7f2d29d0d068e50f94608e835f17616178d6f38451a71`.
The narrow API/type corrections clear source review: the observer now takes
and returns PyValue, accepts only the exact Boolean values without boxing,
returns PyValue_NULL for its initial error and PyValue_FromResult for the
common result, and uses the method cast. The DWORD-only inventory extension
has the realistic cast positive and exact held-local negative inside an
inactive Windows branch. The coordinator had granted this checker/test
extension at 11:42; no approval remains pending for these corrections.

`DWORD-parser-controls.log`, SHA256
`a3bd4a776da759f7c0d60e849da72ecbc88a49f2e96fcd3189f0da0c501261ae`,
retains the actual new test failing with the old parser and passing with
the correction, including its required held-local diagnostic. This does
not close the complete semaphore body: its next source check fails on the
conditional `else` spanning `#endif`. That failure is retained in
`semaphore-source-check-v2.log`, SHA256
`3a32ac6f73803e7aed46e381c249fbda1c1388fd554d24a00eea592270054c25`.
No v2 native gate result is inferred.

The exact, initially unapplied `semaphore-structured-proposal.patch` hashes
to `688badc429a7e559dcbbb0afce12ba9a80ae87103b8f511290c56c964b656a1c`.
Verdict: **IMPLEMENT_STRUCTURED_SEMWAIT**, subsequently granted by the
coordinator. This is a same-file scalar control-flow refactor, not a parser
waiver or qualification verdict. It initializes `used_monotonic=false`
inside the timed branch, sets it true after sem_clockwait returns regardless
of success, and uses `if (!used_monotonic)` for the existing realtime fallback.

For a nonnegative timeout, absent or false feature availability takes exactly
one realtime timed wait. True availability takes exactly one monotonic wait,
including success, EINTR, ETIMEDOUT and fatal-error outcomes; none falls
through to a second wait. A negative timeout still takes exactly one untimed
sem_wait. Clock reads, timeout arithmetic/clamping, errno handling and result
mapping are unchanged; assigning the flag adds no call or errno write.
Windows and the pthread-condition backend are untouched.

Preserving the original source form in the parser would require joining a
partial C if/else across MacroIf boundaries and checking the following block
under both preprocessor alternatives. Simply skipping the directive would
not preserve feature-absent semantics. The small explicit raw-state choice
has a finite equivalence proof and avoids that broader parser extension.
It does not evade another owner's handoff or replace a root/effect contract.

Required bounded proof after implementation: compile the actual extracted old
and new function bodies with controlled semaphore/time APIs across feature
absent/defined0/defined1, timeout negative/zero/positive, and success/EINTR/
ETIMEDOUT/fatal errno outcomes. Assert wait kind and exact count, clock and
deadline, returned result or fatal path; ensure successful waits are not
misclassified because of stale errno. Preserve old-form parser failure,
new-form whole-body success and the inactive-branch root negative. Stub
controls establish this branch equivalence only; actual Linux builds, linked
inline audits and Park/compiled-stack fixtures remain required. They make no
Windows or other platform runtime claim. Review the new combined source
freeze and control artifacts before a final source verdict.


Coordinator disposition, 11:42 PDT: the exact DWORD-only parser inventory
addition and TestRootCheckC cast-positive/held-object-negative extension are
granted. Proper PyValue METH_O correction is also authorized at quiescence.
No implementation-v2 source clearance or native result is implied by these
grants. The findings above remain historical and no user approval is pending.

## V3: SOURCE_REVIEW_CLEAR

Reviewed frozen six-file patch
`94d68f30d30935075d045c67a230eff0bbd6b192c32acec00c16fe341dd6bda5`
and `implementation-v3.json`, SHA256
`b82a7c2a14a121d3ec98063a50f096719454462807177fef24ca74d8ee554821`,
at the same merge head `d8be2e2c36d8d7651b728598f165c9324ae128f4`.
All six current file hashes and the actual binary git diff match this
manifest/patch. The only change from retained v2 is the exact granted
structured semaphore hunk. The API/type corrections reviewed above remain
unchanged. There is no remaining development source blocker.

The semantic-control driver `semaphore_controls.py`, SHA256
`c8a6e42ed589460f1972993eea18bfb3e43ba29d04e14d2e6ece856603764cbe`,
extracts the actual complete old/new function bodies and compiles each for
feature absent, 0 and 1. I independently checked both body hashes against
frozen8ce/current source and each of the six retained C inputs: each contains
its exact body once. Report `semaphore-equivalence.json`, SHA256
`6e691f5c752531a241ccea9fa8555288ad7eb9db484821e5dab61f0ecc0044da`,
contains exactly the 36 distinct feature/timeout/outcome combinations and
72 old/new observations. Every pair agrees and every result independently
matches the expected wait kind/count, clock/deadline, return/fatal result
and errno, including stale EIO on success. The driver compiles with assertions
enabled and `-O2 -Wall -Werror`. These are copied-body branch controls, not a
pristine baseline, real semaphore timing or Windows runtime qualification.

The owner reports the complete new semaphore source check exits0; its retained
`semaphore-source-check-v3.log` is empty. The old DWORD and CPP failures remain
preserved. At this checkpoint v3 release/debug builds and actual linked inline
audits are running; no result from them, the native Park fixture, the original
retention oracle or any full profile is inferred. This source verdict does
not clear qualification or landing. No native command was run by the reviewer.
