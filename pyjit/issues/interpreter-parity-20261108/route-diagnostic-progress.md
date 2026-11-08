# Call route diagnostic progress

2026-11-08: Prepared disposable stats-only counter diff in W and preserved only
that delta in `E8/call-routes-diagnostic.patch`, relative to W with P1/P2
already present. It adds exact-call thread/other counters, overlap and fallback
entry/outcome counters and transaction terminal-selection counters, with
independent kwargs/varargs/defaults and instrumented subsets. Fields, print,
merge and DSL producer are included; coordinator must regenerate the product.
Reviewed code paths; `git diff --check` and reverse-apply check pass. No native
build, test, capture or commit. Awaiting independent review. Counters remain
in W until coordinator requests restoration; the patch excludes P1/P2.

Coordinator accepted the patch for capture and requested retirement from W.
Reverse-applied the preserved patch and verified W has exactly the original
five P1/P2 files, matching its prior diff stat; `git diff --check` passes.
Coordinator reports P1/P2 release and debug focused sets passed (540 each).
