# Independent manual-collection witness correction review

2026-11-01. **ACCEPT for fresh focused native feedback only.** Reviewed clean
commit `34deae3b29260aea286b0c4017b963e092a2db62`, parent
`67190a9d81c683f7e1713300b9a99feb2bc85a93`, in
`build/worktrees/gc-integrated-spans`. All four changed files were read.
Capsule `build/span-manual-correction-v3/capsule.json` has SHA256
`670583d2da6402cd73b2be27c1cad468f9a42946cb492bfa65b0f2bb5f17e22c`.
No remaining concrete source blocker was found.

The observed failure has a precise source explanation: `PyGC_Collect()` exits
when automatic GC is disabled, while both fixtures deliberately run with GC
disabled. The preserved v2 log shows backend realloc and the supported C-call
constructor passed, followed by an empty callback observation. It does not
establish constructor-prefix cleanup; that case had not run.

Both cold witness helpers now call `_PyGC_Collect(tstate,
NUM_GENERATIONS - 1, _Py_GC_REASON_MANUAL)`, the actual `gc.collect()` entry.
This forces a manual major without enabling automatic GC. The internal entry
bypasses the public enabled check; its existing reentrancy/domain/lifecycle
contract is unchanged. The route helper restores its scoped route and heap
before error propagation. The partial-constructor helper reads its receipt,
disarms the fault and directly frees the untracked reservation on its error
exit. Its wrapper preserves an existing exception. Both use the existing clinic
convention that a -1 result with an exception is an error.

The tests now require automatic GC to remain disabled before and after each
helper. Nonempty callback observation and false callback allocation routes
remain mandatory. Fault 5 consumption, positive actual MARKED and SHALLOW
pre-counts, zero post-counts and cleared constructor flags remain mandatory.
No case, literal child, route/failure guard or positive witness was removed.
The correction changes no production allocator, marker, collector policy or
header path; the `gcspan.c` hunk is confined to its cold debug witness.

Independent host replay passed 22 existing root-checker methods (21 ordinary,
one existing expected failure), all six affected source/header root checks,
and module/child parsing. The roster remains 20 methods and three unchanged
literal children. All 23 capsule artifact bindings match and the worktree
remains clean. Reviewer outputs are in
`build/span-manual-review-20261101-v3/`, including `receipt.json`. No compiler,
target or native reproduction ran during review.

Fresh images and separately reviewed/activated helpers must establish the actual
callback and constructor-prefix witnesses. Preserve both prior failure runs;
this source correction is not evidence that the tests now pass, that release
works, or that storage/performance is accepted. The user's GC decision deadline
remains **15:14:44 UTC**.
