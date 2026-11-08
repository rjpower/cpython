# Causal control outcome review, 2026-10-31 v1

Disposition: **ACCEPT all nominated causal-control outcomes.** No blocking finding. The tests distinguish real minors from forced majors and detect both exact debt loss and finite automatic-major starvation. This satisfies this causal checkpoint; cadence implementation/activation, qualification, landing and performance still require their separate decisions.

Master directory: `build/worktrees/nursery-adoption/build/nursery-adoption-negative-controls`. Completion SHA256: `dbc77a5aae875fad5ddd4f9f2f97fc66268259f60fb5ad5e6df5395dd0138e6b`. Plan remains `a182fdb515f96e131a94f5c552d7c7dce1b48b8719f6e57adbfcd983e59a6409`; manifest remains `16316123c9e0539e6e6a6b5cf8aa53a9eb0129556c305afe0bf27e3e28833d21`.

Independent receipt: `build/nursery-adoption-review/causal-controls-outcome-20261031-v1.json`, SHA256 `9e87986f41b16cc6a1458ee5220a72b4926a16eff1d2687509a0f14c543354ba`. The host audit rehashed all 70 completion artifacts, source inputs/symlinks and helper hashes/modes in each clean scope, and all 588 sealed products per scope with configuration/toolchain/executable identities. It replayed the unchanged positive and dedicated expected-failure validators against actual state/child/root/log records, and checked every literal launch/terminal/gate/verify/current-idle chain.

| Selection | Actual methods/children | Actual parent/child mode | Result | Collections |
| --- | ---: | --- | --- | ---: |
| Baseline OFF | 3/3 | OFF/N0 | Passed, no skips | 220 |
| Baseline finite-major | 1/1 | KILL, both extras/N0 | Passed, no skips | 211 |
| Forced-major old-boundary | 1/1 | OFF/N0 | One nominated assertion failure | 99 |
| Debt-reset debt/finite-major | 2/2 | OFF/N0 | Two nominated assertion failures | 351 |

The baseline positives completed before either negative build. The actual source scopes remain baseline `ef0af95904b87d888eee4a98b6a745835ff95085`, force-major `d0dd0774107406256bd78a0d7bd45e2fbda15583` and debt-reset `b13efb1792dc0816b10ae1d75a49f0388e317593`. Baseline reused the preserved compiled856 runtime through the reviewed test-only bridge; both negatives have fresh source-specific O1 debug products. Negative commits remain isolated and are not ancestors of the adopted baseline. The baseline runtime differs from compiled856 only in the reviewed Python test file, and its products remain byte-identical.

## Actual nominated failures

- PID `829149`, forced-major: `AssertionError: minor reclaimed an old owner`.
- PID `845069`, debt-reset: `AssertionError: minor reset major allocation debt`.
- PID `845107`, debt-reset finite-major: `AssertionError: ('manual minors starved the automatic major', 3852696, 32768, 244)`.

Each actual child returned 1 after the original `assert_python_ok` expectation remained intact. The parent-side observer captured typed stdout/stderr before that assertion raised. Each outer negative module returned 2, with exactly its one/two planned failures, no errors or skips, and matching unique verbose IDs and actual case start/end tokens. The final child exception lines match their nominations; none is inferred merely from source text printed in the outer traceback. Negative suites have no `suite-result` snapshot and make no synthesized outer-token claim. These three failures are diagnostics and earn no ordinary pass credit.

The finite negative recorded limit `3,852,696`, initial remaining debt `3,851,944`, chunk `32,768`, 244 batches and final zero-based step 243. It reached the fixed bound and failed the major-progress assertion, rather than an initial budget/delta guard. The positive OFF finite case recorded limit `3,851,848`, remaining `3,851,096`, the same chunk and 244-batch bound, and step 103; KILL/extras recorded limit `4,156,816`, remaining `4,156,064`, 262 batches and step 111. Successful bound original bodies assert the actual automatic major and old-cycle reclamation. Those scalar records are post-body observations, not wrappers around collector APIs or exact per-collection telemetry. No bounds were retuned after seeing results.

## Roots, cleanup and limits

The independent receipt maps every actual child PID to its case, actual mode and main-interpreter root records. Positive OFF PIDs are `773247`, `773278`, `773287`; KILL/extras is `774784`. Together with the three negative PIDs above, every child has positive collections and zero fallback/unpinned events. Parent, worker and auxiliary mode/config/seed/source/origin bindings also match. Assertions during the negative bodies are followed by normal child-end evidence; they are distinct from the earlier expected KILL-abort auxiliaries and do not gain ordinary pass credit.

The coordinator confirmed root-owned session `80557` exited zero. Actual completion is `22:35:14.403893 UTC`; the final debt-negative idle receipt is `22:35:08.335771 UTC`. All rows and verification completed before the activated finite deadline. Actual argv equals the frozen literal command except positive downward timeout clamps, and actual cwd matches each arm. Current checks find no live baseline or negative interpreter and acquire each exact build lock. Temporary negative worktrees remain preserved evidence; no negative source was merged or restored over the baseline.

This small batch establishes the selected causal test sensitivity, not broad correctness coverage, automatic-minor scheduling or cost. No native target, retry, build, source/helper edit or timing was performed by this reviewer. A separately reviewed cadence increment may now use these accepted controls without claiming that cadence itself has passed.
