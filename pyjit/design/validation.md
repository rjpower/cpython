# Testing and feature admission

Checks must answer the increment's question on the actual runtime. Run cheap
feedback early and broad qualification on coherent candidates. A source review,
AST parse or host reference control is useful evidence, not an executed pass
on the rebuilt interpreter. [Process](../PROCESS.md) owns the work cycle.

## Gates

| Stage | Required evidence |
| --- | --- |
| Design review | Concrete benefit/hypothesis, scope, semantics, ownership/effects, recovery, migration and success/reject criteria |
| Focused development | Diff/signature review, affected generators, compile/syntax checks and meaningful tests for changed behavior/error paths |
| Runnable increment | Real linked candidate and required extensions; focused tests execute the intended generic/specialized/native routes, including failure and callback cases |
| Code/result review and commit | Independent review of actual source and outcomes; findings fixed; explicit limits and dependency state in the experiment |
| Coordinator integration | Reviewed commits merged, conflicts/dependencies resolved; build and focused regression checks on the merged tree |
| Runtime/ABI admission | Release/debug suites and applicable C-extension, traversal/cycle, stress and sanitizer checks for the changed surface; rebuilt artifact identity verified |
| Performance decision | Correct coherent candidate, matched controls and workload roster, raw measurements, per-workload and aggregate results, memory and relevant phase costs |

Documentation-only changes need content, navigation/link and diff review, not a
native build. Source checkpoints may be committed before the whole dependency
chain runs; label them unqualified and do not admit an incomplete runtime.

## Choose checks for the change

Container/layout/ownership work needs real values, traversal, cycles, weakrefs,
reentry and error cleanup. Native/ABI work needs actual caller→callee→publication
routes, owned argument/result lifetimes, OOM and rebuilt module loading. IR/JIT
work needs guarded fallback, exceptional paths, observation, recovery and code
lifetime. Generated products must regenerate reproducibly from their producer.

Broader release/debug suites, C-extension tests, stress and ASan are required
as applicable before admitting a coherent runtime/ABI change. Declare their
concrete commands, configuration, exclusions and finite limits in the experiment;
do not import obsolete tracing-root counters or a fixed historical stress count
as universal gates. Preserve skips/failures and explain unsupported modules.

The current qualification runner temporarily excludes the GDB
`PrettyPrintTests` class at the user's request. Its explicit opt-in restores
those diagnostics; other GDB groups, debugger-jump checks and LLTrace remain
enabled. The [qualification policy](../issues/interpreter-round51/qualification-policy.md)
defines the exact filter. This is a declared coverage gap, not a repair or a
green full-suite claim; report all remaining failures.

Reuse unchanged valid evidence explicitly. Rerun affected checks when code,
configuration or integration changes; do not rerun expensive suites merely to
accumulate receipts. Fix real failures rather than suppressing diagnostics,
weakening desired tests or silently dropping workloads.

## Performance and integration

Elapsed performance and memory decide performance hypotheses. Instructions,
code size and profiles explain tradeoffs; code size alone is not a rejection
threshold. Experiments improve selected architectural goals rather than vote
again on whether a partial JIT or universal values should exist.

Verify source, executable, library/module and compiler/configuration identities.
The interpreter checkpoint must prove JIT execution is disabled. Follow
[measurement](measurement.md) for raw vectors, matching, spread and aggregation.
Preserve regressions; repetition needs a specific unresolved measurement question.

The coordinator validates composition and the merged tree, not just isolated
components. Serialize shared builds, full suites and quiet timing; do not alter
products while they execute. Record real worker completion and cleanup. Exact
commands/locks belong in the experiment and operational tooling, not this wiki
page. Commit and integration identities belong in results before claiming landed.
