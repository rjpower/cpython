# Golden fixture preparation and inventory replay, 2026-10-31 v1

Disposition: **ACCEPT the exact held fixture-only preparation for one coordinator activation.** Accept the existing module evidence as a failed diagnostic inventory, with the limits below. No compatibility, golden-edit, qualification, performance or landing acceptance is granted. No target ran during this review.

Exact inputs in `build/worktrees/constant-operands/build/constant-operands-golden-fixtures-v1`:

- Plan SHA256: `01b95bf71b459cca00e3987a69818da75dd20daf5b0124c2a538854d24fabdbd`.
- Held launch: `1c9a163cad4fe3ef34ae3f35d3ca1a226758c36fb1358f0d331aca74a6f9499e`.
- Fixture: `47f3ae8adc40a472b3fe289e5e96a3f47e0008c314d42c68d02dc83169927aee`.
- Controller: `86ba4416c0e32e39188b7ce3f451f646972e22df82859970b3740bb4e18ba668`.
- One-shot runner: `23728bb34a4d935eaed16117847f9ad0b76bc18623afa3a518bb24192008cb27`.
- Preparation: `8ddd1f6975c80d609376bb7e8e576728a75f64d10db027140d9b2d1559408298`.
- Host receipt: `build/constant-operands-review/golden-fixtures-preparation-20261031-v1.json`, SHA256 `53330314e1141d10bc3f847a9db5a32cd9c672e57c93ccf2d70a77e5cbdb32f6`.

The source remains clean `a475acbf46d117137f7201adcb4557ce92fb9ec5`; its exact test-only bridge to native debug source `7f0b1819f84ee11b354d60394a4975e1fcb0a188` matches. The independent host audit rehashed the plan/helper inputs, transitive remaining inputs, all 588 sealed debug products, executable/configuration and toolchain binaries. All match. The four helpers parse. Preparation remains held with no activation or words artifact.

## Existing inventory and the observer failure

The original unchanged full `test_dis` ran once: returned native code 2, 147 tests, six AssertionErrors, no errors or skips. The host wrapper then exited 1 before any fixture child. This was not a successful inventory execution. Its observer emits `suite-result` only after `test_func()` returns, but `single._run_suite` raises `TestFailedWithDetails` for a failed unittest result; `regrtest_runner` subsequently preserves the returned failures and stats. The missing outer snapshot is therefore expected on this path. The earlier preparation review missed that mismatch; this successor preserves the original review and failed artifacts.

The author's read-only replay (`replay_existing.py`, SHA256 `feed65cf5e477b169783a80aaf40dc4a27f9f208568c7aa9ce2199d511bac7a9`) was actually rerun on host Python and reproduced `replay.json` SHA256 `30c59bf7041e853dfd98a9b38c894d63816fc340de512d62005551f6ee376115`. It checks recorded state/config/source/snapshot/PID/seed/OFF-N0/auxiliary bindings and root coverage: 79 collections, zero fallback. Independently extracting the raw verbose identities and failure headers confirms 147 unique IDs, 141 ordinary passes and six failures. These IDs are diagnostic log evidence crosschecked against returned stats; **there is no outer suite-result token**. No full-module rerun is needed to repair this parser assumption.

The independent receipt also binds actual outer launch/terminal/gate/idle artifacts. Actual and declared argv differ only by the downward timeout clamp; outer terminal code is 1 and cleanup records no live target. The original in-child result hashes `driver.log` while it is still empty. Its terminal traceback is appended later. Both the historical empty digest and actual terminal driver-log digest are recorded separately in the receipt; the historical hash is not presented as the final log hash.

## Causal classification and bounded next observation

The six raw failures are two `test_disassemble_str` comparisons (the base/file variants), two `test_loop_quicken` comparisons, `InstructionTests.test_jumpy`, and `TestFinderMethods.test_findlabels`. The expression and loop diffs show removal of the small-integer producer and use of an inline RHS, with the loop backedge adjusted. `test_findlabels` derives its expected offsets from the same hardcoded `expected_opinfo_jumpy` list used by `test_jumpy`; it is not an independent live-target oracle. A stale list can explain both failures. This remains a hypothesis until the exact capture is reviewed; no golden edit is accepted here.

The held probe uses the exact five existing fixture strings with the existing `DisTests.get_disassembly(depth=0)` wrapper and conditional address normalization. It inspects the existing `jumpy` function using `dis.get_instructions(..., first_line=expected_jumpy_line)`, records raw `co_code`/`co_fbinit`, normalized actual/expected instruction rows and replacement text, and requires `findlabels` to agree with live instruction jump-target offsets. It calls neither `jumpy`, `outer`, `_prepare_test_cases` nor any fixture workload/test method. The preview contained unserializable expected cache bytes; the frozen successor normalizes expected positions/cache information exactly like actual rows and the existing comparison, resolving that concrete issue.

The controller checks the exact debug image, mode/seed/origins, sealed C-API product, positive root collections and zero fallback. Activation, shared serialization, build locking, terminal/cleanup handling and verification reuse the unchanged reviewed row runner. The one-shot deadline is the earlier of five minutes from activation and original `2026-10-31T22:20:45.868428+00:00`; the 210-second reserve leaves at most 90 seconds for target capture. Outer gate/launch/verify/idle receipts will again reside in the remaining-v1 directory and must be bound with local result/words/root artifacts in the outcome review. No new diagnostic framework or broader native run is part of this acceptance.
