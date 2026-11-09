# Independent cadence feedback preparation — 2026-10-31 v1

Disposition: **ACCEPT the final v4 held helpers for coordinator activation. No blocker remains.** This review does not activate native work, establish collection behavior, or grant performance/qualification/landing credit. Earlier helper snapshots and their preparation findings remain preserved; no target ran during these corrections or this review.

## Exact accepted snapshot

Master directory: isolated `build/worktrees/nursery-adoption/build/nursery-cadence-first-feedback`.

| Artifact | SHA256 |
| --- | --- |
| plan.json | `9de48f327069070bdf05672d52d2ca9177946c3b1c57059789c84a52e39de996` |
| launch-held.json | `e216098e16895313d59a9fea90cebb2eeaa740c0baf0a5ee2e7918ded90aa993` |
| ready.json | `8fea267f07e53007893575e3055a679ba583324cb7238ec4d94abc3261aef1fd` |
| helper-diff.patch | `b31ea2dd599628af45a126c3890f9ffe0c243a028d3eaf4ed537ae5862f83f36` |

The master binds [exact-source acceptance](cadence-source-20261031-v1.md), SHA `babc17adc326e4b35b76c368c9b804c2feb8414d53e51714ab82f1a9a348a9f9`, and the PRIMARY feedback plan SHA `de75d43555197467d26c43d47b0edce6ec911e9d9934f764d519819698049cec`. The earlier v1 plan `0d0f7d6b…`/launch `92f774f2…` is not the accepted execution snapshot.

## Resolved findings

1. **Verifier mismatch:** v1 forced every direct negative OFF and validated `[0,0]`, but the exact attempt child11 requires retain `[1,0]`. It would have failed its startup assertion. Final per-negative policy comes from the matched positive child: attempt retain, major/postpone OFF. The actual original child bytes remain identical. Host controls reject an OFF attempt witness.
2. **Timeout output loss:** v1 captured PIPE output and wrote it only after ordinary return. Final code opens raw stdout/stderr files before waiting. An independently mocked timeout through the actual `run()` preserves both partial streams, propagates TimeoutExpired, and hashes them in a failed result. No target was spawned by that control.
3. **Optional auxiliary aggregation:** the platform auxiliary file was optional to the preserved validator but mandatory to final aggregation. Final metadata permits absence of only `candidate-focus.aux.jsonl`, records its actual presence, and hashes it if present. All mandatory artifacts remain required. I independently exercised absence, presence, and missing-mandatory rejection through the actual aggregation function.

The source, nominated test bodies, prior builds, and accepted immutable helpers were not changed to resolve these preparation defects. V1/v2/v3 artifacts remain under the master's review snapshots.

## Source, helper and row bindings

All four clean source scopes retain 6,944 bound inputs each; helper hashes/modes, selected compiler and linker tool origins, original build protocol, and protected prior debug/debug-retirement products pass. All four new `build/debug-cadence` Makefile/image paths are absent before activation. Candidate remains `c8cc7718ef3037500aa3902ba5c31acc2b8d7c60`. Each negative is a direct child of that candidate and changes only the nominated `Python/gc_live.c` hunk:

| Scope | Exact commit | Source fault |
| --- | --- | --- |
| major | `1806c105fb7f638355deca23b834d09c57ce98bf` | Both automatic minor selection outcomes become major |
| postpone | `5cdcd65771e1d57978fee27ca44c0244ce5545ec` | Each successful minor adds RETRY_BYTES to M |
| attempt | `ac5066c7363f20834666ae858f2cb18965c8936c` | Close H after start callback, before young marking |

I inspected those actual commit diffs against the reviewed nominations, in addition to the frozen branch/input checks. No runtime option or modification to a sealed image is involved.

The eight rows remain serial: candidate fresh debug build, candidate focus, then each negative's fresh build and direct child. Positive execution completes before any negative build/run. Candidate focus requires thirteen actual parent IDs and twenty-two actual fresh children with exact per-case original-code hash multisets, zero planned skips, actual policies and restored stress, and positive main-interpreter root collections for every observed child PID. Zero fallback/unpinned counters remain mandatory. The full worker/result/full-contract protocol is reused unchanged; the focused deltas are child observation/validation and direct negatives.

The settrace child retains its natural direct `assert_python_ok` isolation: actual `-I`, ignored environment flags, randomized hashing, environment seed text still zero, OFF/N0. It does not receive the queue-child startup assertions or falsely earn deterministic-seed coverage. `dump_root_stats()` deliberately reads its path with `getenv`, so the isolated child can still provide actual root counters. All other children retain the natural queue-child path. Dynamic stress/kind/fault/payload behavior is proven by successful original source assertions, not per-call observer wrappers.

Each direct negative retains its exact source-derived prelude/body/cleanup and earns **zero ordinary parent-method or regrtest credit**. Required evidence is typed CompletedProcess return 1, the nominated final AssertionError, actual child start/end identity and policy/helper/CAPI/executable origins, and positive actual-child main-root counters. Major additionally requires an actual start-kind sequence without a minor; postpone requires exact measured-M/chunk/batch/last-step scalars; attempt requires actual changed H, preserved M and non-reset debt after the source's earlier fault-consumption checks. Wrong exceptions, crashes, missing witnesses or different assertions fail the batch. No outer suite-result is fabricated for these direct auxiliaries.

## Deadline, cleanup and host verification

Activation must bind the exact master/scope manifests, both independent reviews, coordinator exclusivity, and a positive window of at most 2,700 seconds ending no later than 2026-11-01 04:18:11 UTC. It does not reset the overall deadline. Existing accepted gate/run_row/idle/cleanup code is reused by hash with explicit root/build rebinding. Build caps are 420 seconds, focus 180, each negative 90, each verifier 30; actual exposure is downward-clamped using the existing 210-second reserve. Gate terminal results precede failure handling and idle cleanup, and verification/aggregation must finish before the batch deadline. First unexpected failure stops; raw files and current cleanup remain reviewable. No broad suite or measurement is included.

Independent host replay reused the frozen pure preflight actions while omitting only their final write into the frozen preparation directory. It checked the positive synthetic 13/22 contract, direct-isolation exception, per-negative policy/failure shapes and malformed witnesses, selected tool origins, clean source inputs and helper parsing. These are simulations, not native evidence. The exact master loader was exercised only with a uniquely separate HOST fixture; its activation was disabled afterward. The actual master activation remains absent. The timeout and optional-aggregation controls likewise used task-owned host fixtures and mocked target interfaces only.

Audit script and receipt: `build/nursery-adoption-review/cadence-helper-preparation-20261031-v1.py` and `.json`. No build, target interpreter, suite, benchmark, or probe was executed. Actual linked precision, useful collection/fault/resource behavior, all three expected negative failures, cleanup, and subsequent coherent GC feedback remain pending on coordinator activation.
