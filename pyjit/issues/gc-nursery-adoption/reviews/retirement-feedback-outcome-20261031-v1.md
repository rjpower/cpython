# Retirement feedback outcome review, 2026-10-31 v1

Disposition: **ACCEPT this exact batch as successful first native feedback.** No blocking outcome finding. This does not accept landing, qualification, automatic nursery cadence, or performance. Required force-major and debt-reset ablations remain open before qualification; the automatic-cadence source increment and its cost decision remain separate work.

## Exact evidence

- Clean source HEAD: `856222c62458bf3648e51d0bb0d278b1fbf54bd7`.
- Evidence directory: `build/worktrees/nursery-adoption/build/nursery-adoption-feedback-retirement`.
- Plan SHA256: `d5933054028035800bfbe0ad4cb87a77f8fc13d21fb454fa2d23260daa9c5568`.
- Held manifest SHA256: `bdebe4659fedb4ad977788b1ce7b826368644564f87ffca2c0f3410281d5e6cf`.
- READY SHA256: `91df2d8d0b4e57cb4c2486ce9126dba222bb94454a1e451a7e7cd1837134c4df`.
- Completion SHA256: `3bb58d5a6c5dff679806ede5ec1bbe0f9a6ac38f65a2a2ddc6f1dd38d7cc90c1`.
- Independent host receipt: `build/nursery-adoption-review/retirement-feedback-outcome-20261031-v1.json`, SHA256 `f1faf960ffd8fd7838d965d4f1b37bf2e1664b9e8e600524fd541cd125dc1a69`.
- Terminal/snapshot/child-origin supplement: `build/nursery-adoption-review/retirement-feedback-terminal-20261031-v1.json`, SHA256 `85534ad82303c299bf8a3ee0434c723ed275398e0f73f96f59687066fd266059`.

The coordinator explicitly reported polling root-owned session `90952` to exit code zero at `21:54:44 UTC`. This reviewer separately checked the completion identity and recorded evidence, rather than treating file existence as terminal-session proof. The batch completed at `21:53:34.769030 UTC`, before its unchanged `22:07:46.664152 UTC` deadline.

The host audit rehashed all 91 completion artifacts, all 6,944 source inputs, symlinks, helper hashes/modes, activation reviews, primary feedback plan, build configuration and toolchain binaries. All 588 sealed build products match; there are no additional generated cache products. The fresh O1 debug configure and make both returned zero. The source, helper and build identities agree with the reviewed corrected-source snapshot; earlier failures and diagnostic artifacts remain separate.

The supplement verifies the source-snapshot digest and exact input/symlink maps, every child-record provenance binding, and all 125 actual nursery child C-API origins against the sealed products, including the expected KILL auxiliaries. It also preserves the coordinator's terminal-session attestation without claiming that this reviewer polled that session.

## Observed results

| Row | Actual tests | Skips | Fresh nursery children | Normal child ends | Collections | Fallbacks |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Focus OFF | 20 | 0 | 37 | 36 | 498 | 0 |
| Focus KILL, both extras | 9 | 0 | 12 | 12 | 276 | 0 |
| Stress RETAIN, both extras | 1 | 0 | 2 | 2 | 175 | 0 |
| Remote statistics OFF | 2 | 0 | 0 | 0 | 87 | 0 |
| Whole `test_gc` OFF | 204 | 21 | 37 | 36 | 10,179 | 0 |
| Whole `test_gc` KILL | 204 | 21 | 37 | 36 | 10,178 | 0 |

All six test rows returned zero, with one passed module each. The focused matrix contains 30 method executions and exactly 51 fresh nursery children, including one expected KILL failure. The remote-statistics row actually executed both cases; no availability skip earns execution credit. Both complete GC rows retain every nursery case, with exact expected skips and no selection narrowing.

The unchanged full-suite and child validators were replayed against raw state, child and root JSONL. Parent/worker module rosters, result tokens, case IDs, filters, seed zero, state/config/snapshot/source bindings, native return codes and protected-case coverage match the plan. Each whole run has the exact same 21 skip pairs: one free-threaded-GC guard, one tail-call-interpreter guard, and 19 unsupported-subinterpreter guards. The exact IDs and reasons are retained in the independent receipt; there are no nursery/checker/stress blanket skips.

## Child and root evidence

The receipt explicitly maps every normal nursery child PID to its case ID and actual main-interpreter root records. Each has positive collections and zero fallback: 36 focus OFF, 12 focus KILL/extras, two RETAIN/extras, and 36 in each whole-suite row. These are actual child records, not inferred coverage from the driver or worker.

Three expected KILL auxiliaries are recorded separately: PID `634116` in focus OFF, `636952` in whole OFF, and `639271` in whole KILL. Each belongs to `test_kill_detects_young_missing_root`, has an actual KILL/N0 start and return code `-6`, and lacks a normal child-end event. None earns normal-end or normal-child root credit.

Actual start/end mode records show the planned OFF/RETAIN/KILL and extras combinations, with N0 at entry and after local restoration. In particular, KILL/extras has 12 actual `[2, 3]`/N0 children; RETAIN/extras has two `[1, 3]`/N0 children. Stress 1 and 1000 operate inside the unchanged child body, so the observed endpoint records do not claim those interior settings. Interior stress evidence is the successful, hash-bound original code asserting the configured interval, an actual automatic major, and a successful manual minor before restoring N0. This does not claim a nine-policy-by-two-stress cross product.

Both full runs record exactly one unpinned event, exclusively in the suite-worker root record: PIDs `636441` and `638748`. The passing `StackWalkTests.test_unpinned_counter` deliberately compares direct `PyEval_SaveThread()` detachment against the other mode and asserts a one-count difference. That source/test result is consistent with the worker totals; no per-collection attribution is recorded. Nursery children have no unpinned event, and every root record has zero fallback. A claim of zero unpinned events for the whole suite would be false.

The corrected free/resize oracles, same-address and allocator-failure preservation, real address reuse, and scratch last-record metadata assertions all passed in their bound original child bodies. Their passing result does not reinterpret the earlier perturbed diagnostic failure or remove its recorded limits.

## Terminal and cleanup checks

All seven literal launch chains, including the fresh build, match the held commands and environment. The only launch-argv change is a positive downward timeout clamp; launch and terminal receipts agree. Gate-log hashes, verifier argv/zero results/log hashes, original deadlines and current cleanup receipts match. Every recorded cleanup has an empty live-process list and precedes the deadline. The independent host check also found no live process using this target executable and acquired its exact build lock.

Only recorded artifacts and pure host validators ran during this review. No candidate interpreter, native validation, source edit, helper edit, build, retry or timing was performed by the reviewer.
