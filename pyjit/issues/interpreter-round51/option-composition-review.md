# Round 51 composition assessment

Recommendation: measure **3 + 4 first**, then test **adding 2** only if its
incremental result justifies it. Keep **5 out of the first combination** and
defer it on the current evidence. Agree with rejection of 1. No combination is
admitted by this review. This is a cold performance/composition assessment;
its author implemented 3 and 4, so independent merged source approval must come
from another reviewer.

## Evidence and repeated costs

Inspected actual component changes, component reviews, all five screens and
completed confirmations in `E/screen-optionN` and `E/confirm-optionN`, where
E is `BASE/pyjit-link-wave1/interpreter-round51`. Screen and confirmation identity records agree
before/after, identify the clean bf810 rebuilt control and correct candidate,
and report no failure. Commands record CPU 15, hash 0 and JIT disabled. Raw
paired ratios below are elapsed candidate/control, not instruction counts.
Confirmations use six balanced pairs; a dash means that case was not selected
for that confirmation, not parity.

| Option | DeltaBlue | Comprehensions | Generators | Coroutines | Spectral norm | Logging silent |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 0.989596 | 0.960253 | 1.022307 | — | — | 1.010509 |
| 3 | 1.022158 | 0.974249 | 1.021656 | — | 0.892686 | 0.852953 |
| 4 | 1.028112 | — | 1.007676 | 0.950130 | 0.864861 | 0.975341 |
| 5 | 1.051695 | — | 1.008468 | 0.958486 | 0.900842 | 0.990015 |

For 3, 4 and 5, every reported gain and loss has the same direction in all six
pairs. For 2, comprehensions wins all six; DeltaBlue wins five, while generators
and logging lose five. Option2 DeltaBlue spans 0.963045–1.006201, so its modest
mean improvement is less decisive than its comprehension result. Its Richards
screen wins both pairs (0.981836 mean), but Richards has no six-pair confirmation
here. Other unconfirmed screens must retain their two-pair label.

The losses are not isolated noisy pairs. Options 3 and 4 each lose 2–3% on
DeltaBlue; 2 and 3 each lose about 2.2% on generators. Option5's DeltaBlue loss
increases from 2.6% in the screen to 5.2% in confirmation, with confirmation
pairs all between 1.046646 and 1.057938. No aggregate should waive these costs.
Their causes are not established by the source review.

## Mechanisms and limits

**2:** The implementation removes redundant canonical cursor/item publication
and the folded-recipient round trip in the proved list/tuple body. It preserves
the existing destination transaction and ordinary observer adapter. This is a
credible direct mechanism for comprehensions. It does not remove the C list
getter or optimize generic generator delegation. The timed tree-traversal
benchmark mainly delegates generators, so a generator win should not be
promised from this producer change. Its confirmed generator/logging losses need
an explicit decision; native publication savings do not excuse them.

**4:** The arithmetic source and value APIs are unchanged. The bounded proof
changes two generated final-store macros, removing normalization after the
scaled overflow join while retaining overflow allocation, NULL and displaced
owner handling. Spectral norm repeatedly executes integer sums in eval_A;
recursive coroutines execute n-1, n-2 and result addition. These are the clearest
direct workload/mechanism matches in this round. That makes 4 a strong core
contender, not an unconditional admission: DeltaBlue loses every confirmation
pair despite its arithmetic traffic.

**3:** Exact activation now shares reservation and initialization, with cold
frontier fill and heap-caller argument transfer. The normal initialized thread
hit retains overlap and has no new helper call; executing code and function
remain separate owners. Repeated source conversions were already optimized
away and are not its mechanism. The small established fast-path change does
not explain a 14.7% logging gain by itself. Accept that as an observed result
of this candidate image if direct comparisons hold; do not describe it as
14.7% cheaper Python calls or claim its cause is solved. Its broad confirmed
logging/spectral/comprehension gains justify trying composition, while its
DeltaBlue/generator losses remain real.

**5:** The explicit completed entry is semantically sound and changes normal
nonescaped generator/coroutine clearing from total slots to co_nlocalsplus.
However, frame.c shares generic and completed clearing through a runtime
boolean. The named wrappers communicate the proof, then converge on a body
that tests that flag dynamically. Ordinary thread cleanup still takes the
generic entry, supplies false, and pays the new flag test before the existing
thread-owner test. New return routing also reaches every normal return. This
is a meaningful broad execution cost for a generator-focused benefit, without
a new per-frame flag or an ownership bug.

Spectral norm's timed kernel has no generator/coroutine completion. Its 9.9%
confirmed gain cannot be attributed to skipping generator scratch; the cause
of that gain is unproved. Coroutines are a plausible direct beneficiary, but
the generator target itself loses all six pairs. With a 5.2% DeltaBlue loss,
no clear unique confirmed advantage over the other contenders, and broader
routing, 5 should not enter by default. Preserve its evidence and defer it;
no rescue implementation or new inner-loop experiment is recommended now.

## Composition order and shared contracts

1. Measure **3 + 4** directly against bf810. Their strongest confirmed benefits
   differ: 3 supplies the logging gain; 4 supplies the coroutine gain. Both help
   spectral norm separately, so its combined benefit is especially uncertain.
   The first question is whether the image retains those benefits and improves,
   preserves or worsens the repeated DeltaBlue/generator costs.
2. Test **(3 + 4) + 2** only as a direct incremental comparison as well as
   against bf810. Option2's comprehension gain overlaps 3's; it is not a free
   extra 4%. Prefer the smaller combination if that addition fails to give a
   repeatable useful gain or worsens generators/logging. A confirmed Richards
   benefit would strengthen the case, but its current screen is insufficient
   to claim one. If 3+4 loses a key benefit, compare the smaller contenders
   directly instead of expanding to all four options.
3. Do not include **5** merely because its standalone average looks attractive.
   Reconsider only after a winning smaller composition exists and a direct
   addition shows useful incremental benefit with an acceptable DeltaBlue cost.
   Current evidence favors deferral. Do not multiply any component ratios.

The source contracts can compose without a new abstraction, but the generator
merge needs deliberate review. Option2's canonical_iteration_outputs applies
only to its proved window layout and publishes cursor before item/retirement.
Option4's canonical_inline_int_source enters only scalar canonical_result_source;
its outer format excludes windows. Neither fact may overwrite the other's
layout result. Option5 changes the exact return-body helper and release-call
classification; preserve that complete source/AST proof independently of both
producer recognizers. Regenerate once from merged producers and inspect that
only the intended list/tuple, ADD/SUB and return changes survive. Keep positive,
renamed, negative-source and stale-AST tests for each proof.

If 5 is ever combined with 3, reconcile their nonoverlapping frame declarations
without merging the capabilities. Reservation must still establish capacity,
truncate the initialized frontier before header writes and cover the larger of
callee end/saved top. Completion's empty-scratch proof cannot authorize reduced
initialization, suspended cleanup or a partial escaped-frame copy. Test the
combined ordinary/self and heap-generator calls, mortal constant homes,
narrow/wide reuse, escaped frames, exceptional scratch and finalizer reentry.

## Final decision criteria

A coherent candidate needs release/debug focused behavior and all producer
negative tests, independent merged source/native review, the strict value API
gate and full diagnostics under the documented quarantine. Preserve observer,
PEP523/DTrace, recursion, allocation-error and return-owner behavior. Component
passes are not a merged pass.

For performance, screen all seven hot/guard cases and all 29 kernels directly;
confirm claimed improvements and suspicious losses with six balanced pairs.
Compare the chosen composition with both the frozen bf810 rebuild and the actual
prior selected image, then immutable golden39/upstream. The existing same-source
control calibration is another reason not to infer final selected-image ratios
from component campaigns. Logging's upstream headroom cannot silently waive a
direct selected50 loss.

Require repeated useful gains on at least two unchanged workloads and report
individual regressions, not just the mean. A persistent DeltaBlue/generator
loss needs investigation and an explicit coordinator decision with the size and
unresolved cause stated. If the combination cannot meet that decision, retain a
smaller measured contender or the prior checkpoint. Do not claim broad parity,
additive benefits, a cause for the large logging gain, or a generator-cleanup
explanation for spectral norm from the evidence available here.
