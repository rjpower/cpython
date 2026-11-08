# Cold integrated review progress

2026-11-08 12:12 UTC. Read the CPython AI policy and canonical guidance, then
inspected both landed diffs at `0539e0c1ff9` and `73c20472a9f` against their
actual source and generated handler. Initial pass found no must-fix issue.
Independently reconciled the retained call-route `0/1,000/2,000` counts: ten
outer allocating overlap entries and ten inner exact-thread entries per loop;
the overlap route has no measured fallback. The existing stats baseline reports
161 entered handlers and twenty frames per loop. The P1/P2 direct timing ratios
are bimodal and do not establish a gain.

Next: complete evidence/identity review, inspect the M1 draft when ready,
record findings and limits in `cold-integrated-review.md`. No native build or
measurement is being run by this reviewer.

2026-11-08 12:27 UTC. Completed the landed evidence audit, including frozen
ELF/hash identity, M2b's exact 151-handler/zero-NOP scaling, and the later
basic timing screen. Independently reviewed the materialized nine-file M1
diff, generated C, fixed-GCC handler/provider disassembly, builds and final
focused receipts. No must-fix finding; separate `m1-post-review.md` records
the callback, ownership and retirement argument and the timing limit. The
initial retirement test expectation was corrected only after the same event
order appeared on retained R5 and M1 binaries. Current P3 worktree source
pre-review has no must-fix finding after restoring the shared version guard
and splitting marker effect metadata from call-invoke markers.

Next: inspect the final P3 materialization and its generated/native evidence,
then update the integrated review with the final batch verdict. This reviewer
continues to make no runtime or native changes.

2026-11-08 12:45 UTC. Completed independent P3-only source/generated/fixed-GCC
post-review: private guarded entry is confined to `CALL_PY_GENERAL`, generic
fallback remains, and release/debug builds and 583-case focused runs passed.
The first merged stats-build smoke found seven failures in the independent IR
contract scanner because it recognized only the old invoke marker. Reviewed
the two-file test-only fix at `1c8774797f3`; release/debug contract and marker
checks passed, with no runtime correctness defect found. The temporary M1
counter overlay showed ten attempts and ten hits per workload loop, then was
restored to byte-identical clean source. The post-P3 two-round basic screen is
1.3824x upstream for logging, still above parity and not an isolated P3
effect. Integrated cold review and both leaf post-reviews now reflect this.

Next: audit final merged release/debug and timing receipts and close the
performance verdict. No native work is performed by this reviewer.

2026-11-08 12:54 UTC. Initial full release qualification ran 54,395 tests and
found one `test_capi` failure: automatic discovery called P1/P2's one-argument
`test_frame_code_home` helper with no argument. Reviewed the minimal two-file
rename to `check_frame_code_home`; the helper body and ownership assertions
did not change. Both builds and focused release/debug
`test_capi`/`test_pyvalue_api` runs passed 1,354 tests with nine skips.
The correction is committed as `0267d428808`; R is clean. Updated the cold
report. Full release/debug qualification is restarting on that HEAD, and
final timing admission remains pending. No native work was done by me.

2026-11-08 13:09 UTC. Independently checked the qualified clean-HEAD
receipts and terminal logs. Release and debug full suites each ran 54,394
tests (2,531/2,469 skips), with 537 test files OK and SUCCESS; both receipts
name `0267d428808` and empty source diffs. Release/debug identity rebuilds,
stats build and 142-test smoke, and 306-unit value-API lint all returned zero
on the same HEAD. The integrated review now calls merged correctness
qualification complete; the fresh six-round timing campaigns remain open.
Separate source-only follow-ups record the kwargs freelist proof limit and
upstream borrowed cross-frame argument ownership. The latter led to an
unselected next-packet frame-view proposal for independent peer review.
This reviewer made no runtime or native changes.

2026-11-08 13:22 UTC. Source-only follow-up found a real upstream
borrowed-argument frame path behind the candidate's current owning-message
snapshot; the actual benchmark message's mortality and attributable refcount
cost remain unmeasured. Wrote a next-packet frame-view proposal, then revised
it against an independent peer review. The revision gives a concrete
caller/window/child/support-owner graph, counted GC edge example, no-fail
promotion before copy/fallback, variant-specific consumer closure, and a
retained-load pilot with a conditional bridge to the actual allocating
logging call. It selects no representation and authorizes no implementation.
Root owns the ongoing clean-HEAD timing campaign. No runtime or native work
was done by this reviewer.
