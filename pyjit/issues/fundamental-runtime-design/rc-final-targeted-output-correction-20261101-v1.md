# Final RC targeted tranche: failed host validation and additive output reader

2026-11-01 07:25 UTC. **The original batch remains failed. Retained native observations support the intended focused tranche, pending root's independent review of this additive reader.** No test, target, compiler or native helper was rerun.

Root confirmed supervisor 68982 terminal exit 1. The frozen failure is `build/worktrees/rc-successor/build/rc-final-targeted-feedback-v1/feedback-batch-failure.json`, SHA256 `1490132a44a151cfaf3b58491d2f10f007c42906aa78cd391a03dccc9289724a`, timestamp 07:20:37.643591 UTC. Debug and release semantic rows completed and verified. The final row's actual RC subprocess returned 0 and the standard harness reported SUCCESS, eight final tests and two modules; its host validator then raised `natural repeated verbose ID evidence`. No batch completion or final-row verification receipt exists, and this review does not manufacture either.

The standard `refleak.py` progress emitter prints one of `.123456789X` without a newline after each repetition, with a space at its documented warmup transition. The next repetition's first verbose method therefore sometimes begins `.test_...` or ` 1test_...`. The original synthetic fixture omitted that concatenation; my readiness review missed the reader gap. The old expression accepted word prefixes incidentally but omitted dot/space-prefixed methods.

The correction lives only in reviewer-owned `build/rc-final-targeted-output-correction-20261101-v1`. `corrected_refleak_contract.py` is the frozen validator with one import and its ID-extraction call changed; all PID/config/filter/origin/result checks remain unchanged. `output_reader.py` uses full-line anchors, permits only one optional standard progress symbol with at most one preceding space, and requires the short method name to equal the full ID's final component. It retains the exact nine-times-each eight-ID multiset. It does not strip arbitrary text or search unanchored substrings.

Eight host-only controls pass against retained text and mutations: actual output; all documented prefixes; missing ID; extra ID; illegal prefixes; wrong short name; consistent-positive leak; malformed or fabricated-zero diagnostics. The first host fixture mistakenly classified the valid ` 1` prefix as illegal; that fixture failure is retained, corrected to two spaces, and rerun. The first composite audit assumed case-boundary records carried a child `kind`; its failed log/source are retained and corrected to inspect that field only on spawn/return events. Neither correction changes runtime evidence.

The actual leak log SHA256 is `15e7fad682e0550829e92c1098caeb2348e7127fb426fd0f83469811742d4fdf`. Corrected reading finds all 72 natural verbose bodies, four warmups and five measured repetitions per module, while returned stats remain eight. No custom suite/method/child records occur inside the leak row. Two real suspicious memory-block vectors remain visible:

| Module | Five measured deltas | Sum | Standard disposition |
|---|---|---:|---|
| `test_frame` | `[1, 0, 1, 1, 0]` | 3 | `(this is fine)` |
| `test_exceptions` | `[2, 1, 1, 1, 0]` | 5 | `(this is fine)` |

Neither is consistently positive, so the unchanged standard harness accepts them. This does not establish zero allocation deltas or an absence of every possible leak. Zero vectors were not printed or invented.

Independent retained-evidence replay validates 35 debug and 35 release semantic methods, zero skips, eight successful embedded programs, and all 24 distinct actual PIDs: ten per semantic row and four for the leak row. Worker scheduling/returned-result identities, actual CAPI/test origins, filters, seed/configuration, raw child streams, all launch arguments and return values agree. All three post-gate records report Python and `_testembed` idle before the deadline; current dual-executable scans also agree. Clean source remains `d7df12b1d81722927e71d9734c70eeafdccfee73`; both 584-product images and their configuration/toolchain/build receipts are unchanged, with no added bytecode. The replay rehashes 7,226 input/evidence paths.

Evidence: `build/rc-final-targeted-output-correction-20261101-v1/{reader.diff,output_reader.py,corrected_refleak_contract.py,check_host.py,host-controls.log,audit.py,outcome.json,receipt.json}`. Outcome SHA256 `887a1d0d00ad82de0cf212efca26d80410b55d9c775df3762d080b0c4c30227a`; receipt `7eb69aef1ca780b38820c0c544a84191a907e5ba97239c8e0a96e696c7e610d1`. Root must independently review the new reader before carrying the native observations as a composite focused result. All frozen helpers, original failure and earlier failed attempts remain unchanged. No full qualification, actual verifier-mode, performance or external ABI credit follows.
