# Independent review of the held original-code observation

2026-10-31, completed at 21:29 UTC. **ACCEPT the exact held diagnostic for
one coordinator-activated observation.** No blocker found. This does not
accept a source correction, rerun the failed case, or grant qualification,
O3 work or timing.

The source remains clean at
`7f0b1819f84ee11b354d60394a4975e1fcb0a188`. Reviewed directory:
`build/worktrees/constant-operands/build/constant-operands-feedback/diagnosis-v1`.
Exact SHA-256 inputs:

- `plan.json`: `c1049f3c44e6fbdd93637878cd4f57476145b60ce22a5191cadb3a2a556c0e50`
- `launch-held.json`: `c9f7ea90894a97a36f9b3a55368b0c45569fb2d2b950025b9cf1b8d4264f5b86`
- `preparation.json`: `74bb17ae7e62bfbb7f941fbef459a9b4bfe45d5e04e50e3143460542e459c169`

The independent host-only receipt is
[`build/constant-operands-review/failure-observation-20261031-v1.json`](../../../../build/constant-operands-review/failure-observation-20261031-v1.json),
SHA-256 `f12d2eead54773880fd056998c8f38e3d3620c94741082ac89aaa75c69c5db94`.
It binds all four helper hashes and rehashes every declared input, including
the failed result/log/state/roots, prior cleanup, build/product receipts,
original manifest and reused runner. The new preparation was held without
activation during review; the failed run is preserved.

The original feedback ran eleven focused tests, with ten ordinary passes and
one failure at the source-shape negative assertion. Its raw CFG canonical,
multiple-use and other pass controls passed. Existing
`flowgraph.c:basicblock_inline_small_or_no_lineno_blocks()` can copy the
conditional example's two-instruction arithmetic/return join into a branch
before register lowering. That permits a fresh single-use RHS even though
the source expression appears to merge values. The failure does not identify
which nested function triggered the assertion, so actual emitted words are
the appropriate next evidence.

`words.py` imports the frozen test module and selects the `shared`, `canonical`
and `dynamic` CodeTypes directly from the original method's `co_consts`, with
exact ordered names. It neither creates function objects nor calls the nested
functions or test method, and does not compile a replacement spelling.
`dis.get_instructions(code, adaptive=False)` reports pristine bytes, handler,
physical inputs and inline RHS, with code filename and first line. Its
explicit `gc.collect()` supplies diagnostic root-counter coverage, not
arithmetic behavior coverage. The zero-call labels are consistent with the
reviewed source, rather than evidence of an independent runtime call counter.

The runner checks the exact target executable, debug configuration, actual
OFF/N0 policy and hash seed zero. Python origins for `dis` and `test_ir` are
pinned; `_testinternalcapi` must resolve inside the isolated debug build and
match its sealed product entry. Root records require the observed PID,
positive collection coverage and zero fallbacks. Unpinned totals remain
report-only. `loaded('debug')` checks the unchanged source, original activated
manifest, configuration, toolchain and sealed products before and after.

The one-shot supervisor takes the existing shared serialization lock and
reuses the frozen `run_row()` without changing it. Its local deadline is the
minimum of five minutes from the coordinator's activation and the original
**22:20:45.868428 UTC** deadline. The 210-second reserve leaves at most ninety
seconds for the target command; kill grace and verification remain bounded.
Exclusive outputs prevent an implicit retry. Current executable-idle checks,
build locking and post-gate cleanup remain in force.

The reused row runner writes `diagnostic-original-words-v1.*` launch, gate,
verification and current-cleanup records in the parent
`constant-operands-feedback` directory. Final outcome review must bind those
external artifacts as well as `diagnosis-v1` result, roots, words and cleanup;
the small completion file alone is not that complete binding.

All four helpers parsed and the declared hashes/source status matched in the
independent static check. No diagnostic, target interpreter, compiler probe,
configure, build or benchmark ran in this review. Actual output, terminal
status, root bindings and cleanup still require independent outcome review.
