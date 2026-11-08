# Causal control preparation review, 2026-10-31 v1

Disposition: **ACCEPT the exact held six-row preparation for one coordinator-activated causal batch.** No blocking finding remains. This is preparation acceptance, not native execution, qualification, cadence, landing or performance acceptance. Activation must follow the IR window's actual terminal/idle confirmation.

## Exact inputs

Master directory: `build/worktrees/nursery-adoption/build/nursery-adoption-negative-controls`.

- Plan SHA256: `a182fdb515f96e131a94f5c552d7c7dce1b48b8719f6e57adbfcd983e59a6409`.
- Held launch: `16316123c9e0539e6e6a6b5cf8aa53a9eb0129556c305afe0bf27e3e28833d21`.
- READY: `69319f23aa91a570505ba639c03ea643b265d8126188580fc0334c9da9973e09`.
- Per-arm helper diff: `00de4cfc5d0c89b1426ae7a8f730a347a70f339ba8e8b83328852f422e936a63`.
- Author host receipt: `7add40ffedb3cc8a2c1e522fb1594b79e131f58efc5e58dae68ff09c7ed6d42c`.
- Independent receipt: `build/nursery-adoption-review/causal-controls-preparation-20261031-v1.json`, SHA256 `37a09c0f9b6654e20ad8b075745714fd1a960213ede9e75084b9b2c7c7a99a3c`.
- Independent host-control replay: `build/nursery-adoption-review/causal-controls-host-replay-20261031-v1.json`, SHA256 `7add40ffedb3cc8a2c1e522fb1594b79e131f58efc5e58dae68ff09c7ed6d42c`.

The audit independently rehashed all 6,944 tracked inputs and symlinks in each of three clean scopes, all helper hashes/modes, immutable supervisor inputs, baseline products/configuration/toolchain, and source-review links. It replayed the frozen host preflight with only its exclusive output destination changed to a reviewer-owned receipt. All 29 Python and six shell helper checks pass; the replay exactly reproduces the author's receipt. No owner preparation script was executed or rewritten.

## Source and causal scope

Baseline is `ef0af95904b87d888eee4a98b6a745835ff95085`, differing from accepted compiled source `856222c62458bf3648e51d0bb0d278b1fbf54bd7` only in the reviewed finite-major test. All sealed baseline runtime products remain unchanged. Temporary negatives have that exact baseline parent and change only `Python/gc_live.c`: `d0dd0774107406256bd78a0d7bd45e2fbda15583` passes zero to the guarded manual collection entry; `b13efb1792dc0816b10ae1d75a49f0388e317593` resets allocation debt only after a successful minor. Neither is an integration candidate or runtime option. Their fresh build directories contain no configured Makefile or interpreter yet.

The strengthened positive witness interleaves bounded allocations and real manual minors under one established major budget. It preserves a finite exposure, bounded per-batch allocation, an actual-major assertion and old-cycle reclamation; the separate exact debt/statistics oracle remains. The selected negatives distinguish old-owner retention, exact debt preservation and finite major progress. A different guard failure or nominated-message text appearing only in printed source cannot count as the expected failure.

The six literal rows are: baseline OFF with three methods/children; baseline KILL with both extras and the finite-major method/child; fresh force-major build; its one old-boundary negative; fresh debt-reset build; its two debt/finite-major negatives. Both positives must pass before either negative build begins. Four positive method executions/children and three expected-negative method executions/children are planned; none has run in this preparation.

## Witnesses and failure handling

The original test bodies and `assert_python_ok` expectations remain unchanged. The negative observer wraps the existing `run_python_until_end`, records its actual typed return code/stdout/stderr, and returns the same result/command before the original assertion raises. It does not wrap GC/stress setters or change collector behavior. The child prefix still records actual seed, mode/extras, stress, executable/C-API/observer origin and normal exit state. Exit observation records only integer scalar limits/iteration counts after the finite test body.

The frozen full-result observer is byte-identical in every scope. Failed suites intentionally have no `suite-result`: validation uses actual returned errors/failures/stats, exact unique verbose case lines, scheduled worker/config/seed identity and actual case start/end/token pairs. It makes no synthesized outer-token claim. Expected negatives require native regrtest status 2, exact one/two failures, no errors/skips, child status 1 and the nominated final child exception line. Crashes, import/syntax failures, wrong assertions, missing normal child exit, changed original-body hashes or missing root collections reject the row. Every actual negative child must have positive main-interpreter collections with zero fallback/unpinned events.

The independent replay accepts the valid synthetic expected failure and rejects ten malformed controls: missing typed return, zero child status, wrong final assertion, extras, observer, original body, returned error, fake outer snapshot, wrong worker identity and zero child collections. These are host protocol checks and earn no native correctness credit.

## Launch bounds and resolved findings

The separately pinned master launcher imports the unchanged accepted supervisor body. The preview initially rebound output/build paths without rebinding its working root, so a negative arm's receipt could describe a different cwd from the actual subprocess. The frozen successor also rebinds `supervisor.ROOT` to each literal job cwd. Output, cwd and build-path rebinding now agree; the supervisor body remains unchanged. The missing positive-child collection guard was also added before freeze.

Explicit activation binds both reviews, master and scope manifests, source inputs and baseline products, and requires the coordinator's exclusive-native-window confirmation. The batch takes at most 45 minutes from activation, clamped to overall `2026-11-01T04:18:11 UTC`. The prior first-feedback deadline is preserved historically, not extended. Each gate uses both global machine slots, its exact build lock, process-group termination and current idle cleanup; native timeouts clamp downward with the unchanged 210-second reserve. Builds declare 900 seconds, test rows 180, verifier wrappers 120. First unexpected failure stops the batch without retries, preserving all prior artifacts and both negative worktrees.

The outcome review must bind actual activation/start/deadline, all six launch/terminal/gate/verify/idle chains, positive case/child/root evidence, each fresh negative build's source/products/configuration/toolchain, actual typed failure records and final exceptions. Baseline runtime bytes must remain preserved. Qualification, automatic-halfway cadence and cost decisions remain separate work. No target, build, activation, runtime/helper edit or native validation was performed by this reviewer.
