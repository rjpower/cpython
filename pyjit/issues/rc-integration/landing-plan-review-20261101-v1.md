# Independent held RC landing-plan review

2026-11-01. **ACCEPT bounded landing/helper preparation only.** Reviewed plan
SHA `354a016965bdb567d06e84a89660223e58b8a9285066a9bcc7458311691786b3`
and facts SHA `11daeaa21b09ebd070ba240c69d60f7dd43d04c2f0f02e0984823d5fb08b26cd`.
This does not approve the pending StackRef scope amendment, claim a packaging
pass, or authorize merge, compilation or native execution.

I independently replayed the facts against primary13dc78ddf100 and selected
RC8e132becdad485f9d281d63c5ec1c9c60ef3e56f. Common ancestor74f1705774b1,
136primary changes and896candidate changes agree exactly. The changed sets do
not overlap. All primary changes are prose under boil, outside tools/bench.
Candidate's only two boil-prose additions are the historical RC closure
checkpoint/results. No current untracked path obstructs the incoming changes.
After root's new0f04444c095 prose-only checkpoint I recomputed the ancestry and
zero overlap; the frozen13dc facts remain historical rather than being edited.
Zero overlap is not a performed merge/conflict check.

All five protected files match their captured bytes, raw modes, exact index
entries and statuses: one unstaged tracked modification and four untracked
files. The untracked modes are0664; the modified tracked file is0644. Their
existing index state must remain unchanged through landing. There is no global
clean-worktree predicate available on main: the future source guard must allow
**exactly those recorded five states**, compare their bytes/modes/index entries,
and reject any other unexpected status. Do not ignore the whole boil tree or
stash/reset these files. Root must commit its own reviewed prose before the
native window, refresh ancestry/source bindings, and freeze HEAD until actual
terminal cleanup. Later worklog updates cannot silently change that authority.

I verified all42 distinct full IDs against actual class/method AST nodes and
source-file hashes. The first39 are exactly the accepted first-feedback roster;
the three namespace/diagnostic consumers are the declared additions. Eight
modules are involved. Each fresh image must return42passes/zero skips; this is
84outer method executions across two images, not a descendant/subtest census.
The source-equality predicate must compare the complete path/mode/type/blob
projection outside boil plus boil/tools and boil/bench, including deletions and
top-level ports, against the selected candidate. Checking only changed incoming
paths or overlaying candidate files would be insufficient. Review that actual
two-parent merge and protected-file bridge before any build.

Both proposed fresh main build directories are currently absent. Their use
preserves the historical main default/debug/release products. Existing isolated
images cannot establish main origins through PYTHONPATH. Exact GCC/mimalloc/
computed/shapes release-defaultO3 and debugO1 configurations, own-main stdlib/
extensions/CAPI/traceback and fresh product manifests must be witnessed using
the existing executor. The four declared row ceilings plus four210-second
reserves total3600seconds inclusive. Helpers must clamp every positive run and
verification timeout to the remaining deadline, retain partial output, stop at
the first unexpected result, and preserve standard dual-image cleanup/locks.
Actual helper materialization remains separately reviewable and unbound now.

The plan correctly leaves critical closure open: actual packaging feedback and
the independently decided global-StackRef support/gate disposition. The two
failed upstream checker builds provide no comparison-test result or automatic
RC exemption. Existing full-suite/ASan/refcount receipts retain their actual
scope; neither a fictitious global-checker pass nor blanket module exclusions
are accepted here. A proven tooling-only bridge need not restart the whole
qualification ladder, but it cannot substitute for the pending dynamic
installer/extension checks. No new source, merge, target or compiler invocation
occurred in this review.

Evidence: `build/rc-landing-plan-independent-review-20261101-v1/` records the
frozen-fact replay and actual0f ancestry/protected refresh. Recompute these
predicates after any further primary or candidate commit before binding native
authority; do not substitute a newer runtime or mix IR/JIT work into this cohort.
