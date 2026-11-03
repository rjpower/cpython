# Golden inventory preparation review, 2026-10-31 v1

Disposition: **ACCEPT the exact held preparation for one coordinator-activated diagnostic.** No blocking finding. This review authorizes neither compatibility acceptance nor qualification, timing, source changes, or landing. No target ran during this review.

## Exact inputs

- Clean source HEAD: `a475acbf46d117137f7201adcb4557ce92fb9ec5`; retained debug image source: `7f0b1819f84ee11b354d60394a4975e1fcb0a188`.
- Directory: `build/worktrees/constant-operands/build/constant-operands-golden-inventory-v1`.
- Plan SHA256: `da2d3febd7dc46f4dfcd8c065f68f17b4c0a451e9f29ab824a7c3f19c1db0323`.
- Held launch SHA256: `7b476fa1fe3803f9fc4e6ff5c1b0e8b23ee01f5a3afe9ee695b5112e8c6f6cb3`.
- `run_once.py`: `8058d1cb8b48c51170868667cf67cab4a3962e19844a40c20417cea7f28b133b`.
- `run_inventory.py`: `bed7a626130a073a2d135dff39b8f885883da9a92c7c196bbcd65ceea0352a13`.
- `fixtures.py`: `32925474329b841edcc7f23282bfb5ef5418da85b86a6afeb29c3a240901df01`.
- `prepare.py`: `8a7c1ba023b98b0833376f757076f3843ffa75c620815ed61124e79cd6b62ab2`.
- Independent host receipt: `build/constant-operands-review/golden-inventory-preparation-20261031-v1.json`, SHA256 `545739ae8e2a88c369c606557a1560bce23b6bda459892e1a466b1054feba84f`.

The reviewer-owned host audit rehashed all 21 plan inputs, all four helper pins and the transitive remaining-plan inputs, parsed the four helpers, confirmed absent activation and clean exact HEAD, and reproduced the native-to-test-source bridge path list and binary patch digest. It did not import or execute preparation, probe, target, or builder code.

## Scope and failure handling

The literal module argv selects unchanged `test_dis`, seed zero, one worker and its existing observer wrappers, with `--failfast` omitted. Existing failed-run receipts remain pinned. The inventory requires native return code 2, a returned `FAILED` suite with no errors, nonempty AssertionError tracebacks, consistent executed IDs/stats/skips, and exact parent/worker/auxiliary bindings. A crash, unexpected status or non-assertion error prevents the fixture child. Since the module runs without failfast, this classification happens after that unchanged module returns.

**The assertion guard does not prove that every failure is a golden-string mismatch.** The result and README correctly label all assertions as requiring causal review. Every collected failure must be reviewed before changing goldens or claiming compatibility; this diagnostic provides no such credit.

The second child obtains precisely the five pinned existing source/expected strings: `expr_str`, `simple_stmt_str`, `annot_stmt_str`, `fn_with_annotate_str`, and `compound_stmt_str`. Its `get_disassembly(source, depth=0)` call uses the same default wrapper and conditional `strip_addresses` normalization as `DisTests.do_disassembly_test`/`do_disassembly_compare`. The `dis` string path compiles with eval/exec modes and disassembles the resulting code; it does not evaluate those fixture workloads or call their test method. Raw and normalized outputs, expected strings and unified diffs are retained.

## Execution boundaries and outcome requirements

Activation requires exact plan/launch/source/review hashes and the specific one-diagnostic decision. The unchanged remaining helper validates the exact source bridge, prior accepted reviews, frozen debug build/config/products and toolchain. Target children use explicit OFF/N0, seed-zero environments with no inherited stress, pinned origins and positive root collection witnesses with zero fallback. These are runtime assertions, not observed outcomes of this review.

`run_once.py` takes the shared serialization lock and one-shot lock. Its deadline is the earlier of activation plus ten minutes and the original `2026-10-31T22:20:45.868428+00:00`. The unchanged row runner reserves 210 seconds and clamps only the outer timeout downward, leaving at most 390 seconds for the gated host/target sequence from a fresh ten-minute start. It retains terminal evidence before cleanup checks, checks current build-lock/process idleness in `finally`, verifies under the build lock, and requires completion before the deadline.

The outer launch/result/gate/verify/idle receipts are written under `constant-operands-feedback-remaining-v1` with prefix `golden-inventory-v1`, while module/fixture/result/cleanup artifacts live in this new directory. An independent outcome review must bind both sets, actual activation/start/deadline and their hashes; `completion.json` alone is insufficient. No source, helper or prior receipt changed during review.
