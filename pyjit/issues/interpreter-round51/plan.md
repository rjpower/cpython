# Interpreter round 51 plan

Improve interpreter performance on unchanged workloads while preserving Python
behavior, explicit ownership and observation contracts. The selected starting
runtime is `bf810ec1a33`; [status](../../STATUS.md) records its qualification
and links the completed measurement report and tables.

## Select the work

Astra supplies five evidence-based options, including expected benefit, affected
hot paths, semantic and ownership risks, and a discriminating measurement for
each. The coordinator selects bounded, independent subtrees for Sol work after
reviewing those options. The hot benchmark set is pending that selection; choose
it to expose the predicted benefit and likely regressions.

## Execute each selected subtree

1. Sol proposes a bounded change with invariants, affected files and tests.
2. An independent Sol review checks semantics, ownership, recovery and the
   experiment's ability to answer the performance question.
3. Sol implements in an isolated subtree and runs focused tests, including a
   release build where C changes are involved.
4. An independent Sol post-review examines the actual diff and executed evidence.
5. The coordinator integrates and measures accepted work against fixed controls,
   then decides whether to retain it. Preserve individual results and tradeoffs.

Serialize quiet measurements and shared builds. Use the established GCC
configuration, fixed affinity and hash, unchanged kernels and JIT disabled.
Run all 29 kernels as guardrails at the final coherent checkpoint; assess
individual regressions as well as the aggregate. Run applicable release/debug
focused checks and full diagnostics at that checkpoint.

The runner quarantines only the established GDB pretty-printer class, with an
explicit opt-in to restore it. Other GDB groups, debugger-jump checks and LLTrace
remain enabled. Quarantined diagnostics are skipped, not fixed. Compare remaining
failures against the baseline and investigate new failures. Observation and
Python behavior remain admission concerns.
