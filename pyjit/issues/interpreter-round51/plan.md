# Interpreter plan

Round 51 tested five architectural fact-reuse options from runtime
`bf810ec1a33`. The coordinator selects **3+4 at `0e5e61f4c6b`**, with explicit
performance tradeoffs. Full diagnostics and independent review are complete
with the established limits. Golden is published as
`pyjit-golden-20261111-runtime`. [Status](../../STATUS.md) owns admission;
the [report](report.md) and
[benchmark tables](benchmark-tables.md) own results.

## Completed portfolio

Astra performed one [grouped design review](astra-options.md). Sol agents
proposed, peer-reviewed, implemented and post-reviewed each option in an isolated
subtree. The coordinator built, tested, measured composition and selected:

| Option | Experiment | Decision |
| --- | --- | --- |
| 1 | Cached inline-values layout displacement | Discard: insufficient cross-kernel benefit |
| 2 | Canonical list/tuple iteration publication | Discard: comprehensions gain did not justify incremental losses |
| 3 | One reservation and shared exact-call initialization | Retain in measured 3+4 checkpoint |
| 4 | Canonical ADD/SUB result joins | Retain in measured 3+4 checkpoint |
| 5 | Normal-return empty-scratch proof in generator cleanup | Discard: confirmed DeltaBlue regression |

The hot set was **DeltaBlue, Richards, comprehensions, generators, coroutines
and spectral norm**, with **silent logging** as a mandatory guard. Two balanced
pairs screened each option; six confirmed selected wins and suspicious losses.
The combination and optional iteration addition were measured directly. Final
all-29 comparisons used upstream, golden39 and the actual previous selected
image. The rebuilt bf810 control showed small same-source calibration differences;
its component ratios were never substituted for the final prior-image comparison.

The portfolio is closed. Retired task-created worktrees/builds were removed;
commits, patches, configurations, native binaries and raw measurements remain
in the evidence store. No rejected feature or alternative evaluator is enabled.

## Next bounded investigation

Focus the next hot set on **generators, coroutines and DeltaBlue**, retaining
silent logging as a guard and all 29 kernels at the integration point.

1. **Propose a delegated-resume continuation contract.** Current generator
   samples rank SEND_GEN and YIELD_VALUE first. Inspect emitted/native control
   flow and event frequency, then specify whether a proved empty backward
   redispatch can be removed. Preserve suspended PCs, distinct incoming/outgoing
   owners, current-frame/exception/recursion state, provider guards, observation
   and real recovery. Do not count the sampled fraction as removable time.
   Require independent design review before implementation; reject the proposal
   if new decoding or state work outweighs the saved dispatch.
2. **Explain the remaining object/call work on DeltaBlue.** Exact CALL and
   guarded attribute handlers remain prominent. Use the selected profile and
   native paths to identify redundant work shared by multiple workloads.
   Cached layout displacement has already failed this round; smaller source or
   handler size is not sufficient grounds to retry it. Compare any admitted
   redesign with the selected checkpoint, including its existing regressions.
3. **Gate constant-operand work on a weighted census.** Measure hidden constant
   homes, activations, reads per activation and frame/copy footprint before
   proposing an operand-domain change. The code object already owns canonical
   constants; a second bank alone is redundant. Feedback already uses direct
   scaled offsets, so removing a supposed pointer table is not an option.

The [source assessment](next-plan-assessment.md) records the transition
hypothesis, ownership/encoding constraints and prior negative evidence.
Native method/tracing JIT tiers remain a separate roadmap stage.

## Integration contract

Use propose → independent peer review → implement → focused test → independent
post-review → integrate. Keep one GCC configuration, fixed affinity/hash,
unchanged kernels and JIT disabled. Serialize quiet timing/profiling against
builds, tests and bulk cleanup. Confirm broad wins and material losses; never
multiply separate campaign ratios. Full release/debug diagnostics belong at
coherent integration points, not every opcode edit.

The runner quarantines only the established GDB pretty-printer class, with an
explicit opt-in to restore it. Other GDB groups, debugger-jump checks and LLTrace
remain enabled. Quarantined diagnostics are skipped, not fixed. Report actual
remaining failures and investigate new failures; Python behavior and explicit
ownership remain admission concerns.
