# Activation 53 report

All three activation candidates are rejected after direct six-pair comparisons.
Golden51 `0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254` remains selected and
unchanged. The independent [cold review](cold-review.md) supports this decision.
The [benchmark tables](benchmark-tables.md) retain each campaign and the fresh
selected/upstream comparison. Candidate branches are pushed. Verified archives
retain patches and native/configuration evidence; the nine task source/build
directories are retired. Selected builds and controls remain intact.

## Why activation was investigated

Selected ordinary exact calls already reserve and initialize once, overlap
argument homes and publish ordinary returns inline. Generative construction
still creates a temporary thread frame and moves its physical homes to heap
storage; generator completion uses an adapter and decodes the caller operation.
Those are real lifecycle costs, alongside the richer frame header, canonical
value handling and callback-safe retirement. Upstream also has costs selected
avoids, and its collapsed evaluator prevents direct symbol-by-symbol comparison.
The [audits](call-audit.md), [resume audit](resume-audit.md),
[thesis](thesis.md) and [corrected design](../../design/activation-transfers.md)
describe the contracts.

The [phase census](phase-evidence.md) observed, per unchanged workload loop,
100002 generator starts and returns with 1568946 yield/resume pairs; coroutines
had 242787 starts and returns with no visible yield/resume events; DeltaBlue had
51422 starts and returns. Counts match upstream but monitoring perturbs execution
and does not measure cycles or fast-path hits. The fresh selected coroutine
profile located 9.28% of samples in exact CALL, 7.62% in RETURN_GENERATOR and
6.45% in generator completion. These are sample locations, not removable time
budgets. Creation/completion and repeated suspension have different weights.

## Why upstream executes faster

Selected51's retained post-warmup PMU campaign, distinct from the fresh elapsed
campaign, shows substantially more retired machine instructions for unchanged
workloads. Ratios below are selected/upstream. Relative cycles per instruction
are the cycle ratio divided by the instruction ratio, using the same paired
counter campaign.

| Workload | Instructions | User cycles | Relative cycles/instruction |
| --- | ---: | ---: | ---: |
| DeltaBlue | 1.352487 | 1.173937 | 0.867983 |
| Generators | 1.414550 | 1.443516 | 1.020477 |
| Coroutines | 1.388163 | 1.249676 | 0.900237 |

Evidence: `BASE/pyjit-link-wave1/interpreter-round51/final51-perf-stat-clean/normalized-summary.json`.
The primary aggregate distinction is more executed instructions; average
cycles/instruction are better on DeltaBlue/coroutines and nearly equal on
generators. This does not partition instructions or stalls by mechanism.

The source comparison identifies concrete differences, rather than assigning
shared Python obligations to this runtime alone:

- **More general input/result transfer.** Upstream SEND_GEN pushes the input at
  the child's known stack frontier and advances the parent's stack pointer.
  Selected SEND_GEN NULLs its source home, records an explicit recipient/PC and
  calls the inlined `_PyFrame_SetReturnValue`: pending redirect, discarded
  recipient, prior recipient owner and its retirement must be considered.
  Selected YIELD records the next incoming recipient; upstream's stack position
  supplies that location. Both update real frame/exception/recursion state.
  The selected generator profile places 27.41% of samples in SEND_GEN and
  13.92% in YIELD_VALUE, making repeated transfer a stronger target than final
  construction for this workload. These are locations, not removable budgets.
- **More construction and storage bookkeeping.** The selected header is
  112 bytes versus upstream's 80. Generator construction moves every physical
  register home and NULLs every source home. Upstream copies only through the
  live stack frontier and leaves popped source storage untouched. The selected
  frontier invariant currently requires those NULLs. Both runtimes allocate
  generator/coroutine objects; allocation itself is not a relative explanation.
- **Runtime recovery of register ownership facts.** Ordinary register consumers
  can name locals or temporaries. Generated paths use the local boundary to
  select borrowing versus consuming/NULLing, and result paths support arbitrary
  recipients. Upstream stack effects supply some of those lifetime/location
  facts implicitly. General PyValue consumers also distinguish inline values
  from objects before object access. Specialized native paths can remove some
  checks; source appearances alone do not count executed tests.

Exact ordinary calls already save upstream's argument copies and repeated local
NULL filling. They do not recursively enter the C evaluator. Both runtimes own
executing code, check providers, maintain recursion and handle observers. Those
shared costs cannot explain the relative gap merely by appearing in our profile.
Nor does this comparison prove that the frame header, PyValues or one helper
accounts for a particular percentage. The missing evidence is a weighted native
instruction attribution across corresponding complete paths; upstream's
collapsed evaluator prevents a direct per-symbol percentage comparison.

## Experiment decisions

[Completion](completion-result.md) removes the inspected hot C completion
boundary but loses 3.1% on coroutines and 1.1% on Richards in all six pairs.
[Full final-home construction](final-homes-result.md) improves generators 7.0%,
but loses 2.8% on coroutines, 24.4% on logging silent and 1.5% on comprehensions,
all in six of six pairs. [Native-only construction](native-construction-result.md)
improves generators 5.9%, but loses 2.4% on DeltaBlue and 2.7% on Richards,
again in all six pairs. Logging pairs have a wide spread. Inspected boundary
removal and observed route hits do not establish acceptable breadth.

Separate full-candidate instruction diagnostics give candidate/golden51 ratios
of 0.985784 instructions per loop and 1.027712 elapsed for coroutines, and
1.008263 instructions and 1.244130 elapsed for logging. Work count alone does
not explain elapsed time, and no hardware cause is established. Campaigns are
independent direct comparisons; do not subtract or multiply their ratios to
attribute a subchange.

Candidate qualification was focused, with source/native reviews, release/debug
checks and actual route evidence as detailed in the component reports. The full
construction lane caught and corrected an observer regression before its final
checks. No rejected candidate ran full suites or all29. The selected round51
full diagnostics remain the prior result: nine debugger-jump errors in release,
those nine plus one LLTrace failure in debug. They are documented limits, not a
new full-suite pass.

A fresh two-pair selected51/upstream all29 screen has geometric mean 1.156831
and six wins in 29. Its generator ratios span 1.437–1.784, so the 1.6014
geometric mean is a noisy screen, not a stable estimate. A separate six-pair hot
confirmation gives 1.450583 for generators, 1.257081
for coroutines and 1.174507 for DeltaBlue versus upstream; all six pairs lose
in each case. Logging silent is 0.730916 with a wide 0.662089–0.771119 range.
The [next steps](next-steps.md) document future
investigation; no rejected lane is promoted by this refresh.
