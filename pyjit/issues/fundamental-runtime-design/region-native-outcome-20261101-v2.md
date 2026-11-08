# Interpreter-region v2 native outcome

2026-11-01, coordinator. **STOP before focused cases: evidence directory absent.**
Session 40701 returned 1 at 04:21:47 UTC. Source
`40498d1e0d3773ac2a1180eb8cac7fd04831028d` built successfully in a fresh
GCC O1/debug directory. Configure and make returned zero; the module check
reported 116 modules, ten missing optional modules and zero import failures.
The debug image and its frozen source/configuration/product/tool receipts
remain valid build feedback, without runtime qualification.

The focused host wrapper failed while creating its temporary pycache directory
under the missing `interpreter-region-feedback-v2/affected` directory. Its
exception-result write failed for the same reason. No test target or selected
case ran. The supervisor retained the raw gate traceback and stopped before
release build/tests. Actual post-gate cleanup acquired the debug build lock
and found no live target.

The v2 preparation copied files but omitted this empty output directory.
Readiness checked imports and empty build directories, but missed the actual
temporary/result-write destination. Root also saw its absence during a read
and failed to act. This is a preparation/review failure, not an IR semantic
failure or a reason to change runtime code.

Preserve all v2 products and receipts under
`build/worktrees/interpreter-regions/build/interpreter-region-feedback-v2`.
The unused activation window grants no retry. Prepare a separate v3 activation
and outputs, proving actual temporary/open-exclusive-file writes before launch.
Reuse the valid debug image if an exact immutable build-plan bridge preserves
every source/configuration/product/tool check; otherwise rebuild explicitly.
The unchanged 34 cases and zero permitted outer skips remain mandatory.

V1 remains a distinct missing-activation refusal with no build or target.
No IR native behavior, elapsed result, parity or integration is accepted.
