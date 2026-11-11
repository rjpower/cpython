# Interpreter round 51 plan

Improve interpreter performance on unchanged workloads while preserving Python
behavior, explicit ownership and observation contracts. The selected starting
runtime is `bf810ec1a33`; [status](../../STATUS.md) records its qualification
and links the completed measurement report and tables.

## Select the work

Astra's [grouped review](astra-options.md) defines five evidence-based options.
Each starts independently from runtime `bf810ec1a33` with the same GCC release
configuration. Sol owns implementation and independent component review.

| Option | Experiment | Current state |
| --- | --- | --- |
| 1 | Cached inline-values layout displacement | Proposal peer-reviewed; implementing in option1 subtree |
| 2 | Canonical list/tuple iteration publication | Proposal peer-reviewed; implementing in option2 subtree |
| 3 | One reservation and shared exact-call initialization | Sol preparing bounded proposal in option3 subtree |
| 4 | Canonical ADD/SUB result joins | Queued for Sol after option2 |
| 5 | Normal-return empty-scratch proof in generator cleanup | Queued for Sol after option1 |

The hot set is **DeltaBlue, Richards, comprehensions, generators, coroutines,
spectral_norm**, with **logging_silent** as a mandatory guard. Run two balanced
pairs to screen each candidate, then six on claimed improvements and suspicious
regressions. Require repeatable benefit on at least two unchanged kernels; inspect
all 29 before promotion. Performance loss discards a candidate even if its native
code is smaller. Composition is measured directly, never by multiplying ratios.

The bf810 control is frozen in `round51-baseline{-release}`; the original bf810
image is retained as a final direct comparison as well. An initial two-pair
control calibration has small differences despite identical source/configuration
(roughly -3.6% to +0.9% across the hot set); it is not an optimization result.
Final admission must include the actual previous selected image. Upstream and
older golden controls remain immutable.

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
