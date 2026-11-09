# Independent comparison-region elapsed decision

2026-11-01. **REJECT this source increment.** Candidate
`40498d1e0d3773ac2a1180eb8cac7fd04831028d` is slower on the primary workload and
the four-workload geometric mean in both predeclared rounds. Keep the accepted
parent runtime; retain the isolated candidate and all source, correctness,
mechanism and measurement evidence. Do not advance this increment to full-29
qualification or collect automatic extra samples.

All values below are independently recomputed from the original 80 elapsed
values, normalized by the unchanged registered loops. Positive change is slower.

| Workload | Round 0 elapsed | Round 1 elapsed | Round 0 spread parent / candidate | Round 1 spread parent / candidate |
| --- | ---: | ---: | ---: | ---: |
| Coroutines | +10.86% | +14.11% | 8.45% / 7.54% | 9.31% / 7.10% |
| Generators | +26.08% | +21.33% | 26.71% / 16.93% | 20.17% / 17.30% |
| Comprehensions | +9.62% | +0.22% | 25.10% / 11.04% | 19.66% / 11.09% |
| Richards | +19.03% | +20.62% | 0.21% / 0.45% | 0.73% / 2.55% |
| Four-row geometric mean | **+16.21%** | **+13.74%** | | |

Both required primary ratios below 1.0 and geometric means at most 1.0 fail.
The >2% control-regression threshold is exceeded by all three controls in round
0 and by generators and Richards in round 1. Comprehensions vary between rounds,
but the primary and two other controls lose in both. These results are not a
borderline survivor. Five serial samples are correlated; that limitation does
not justify dismissing the consistent losses as noise. A descriptive
leave-one-out check, using only the same observations, keeps the primary ratio
above 1.09 in both rounds and Richards above 1.18. It is not a new acceptance
rule or an additional sample request.

GNU time whole-process peak RSS shows no compensating large resource benefit:

| Workload | Round 0 parent → candidate, KiB | Round 1 parent → candidate, KiB |
| --- | ---: | ---: |
| Coroutines | 69948 → 70052 | 66860 → 65852 |
| Generators | 73628 → 71552 | 71852 → 71572 |
| Comprehensions | 85900 → 85696 | 86176 → 85956 |
| Richards | 30960 → 31024 | 30900 → 31308 |

These are whole-process maxima, including startup/warmup and the five samples,
not retained-heap or per-iteration occupancy measurements. Memory is reported
as a tradeoff; no arbitrary RSS or text-size veto is used. Elapsed decides this
clear loss.

The actual campaign finished successfully, with 16 fresh scored processes,
five samples after one registered-loop warmup each, and eight separate
zero-sample/one-loop origin processes. All 24 returned zero. The original
alternating arm order and four workload/loop selections match the frozen plan.
All eight raw loader/import traces replay successfully against exact source,
native product and ELF manifests, including each runtime's own json source and
_json extension. No origin warmup is counted as a scored sample.

All 193 launch artifact bindings, source/config/tool/product identities and
all four before/after/terminal product inventories match. The compiler profile
remains matched GCC O3 with evaluator -fno-gcse, frame pointers and zero-init,
without JIT/PGO/LTO/debug/stats/tail. Affinity was CPU 0; sibling 8 and both slots
were reserved under the recorded locks. All 48 quiet checks passed unchanged
policy, with maximum observed one-minute load 0.4287109375 against the declared
limit 4. Gate and inner result were zero; both image PID sets were empty under
their locks, and cleanup finished before the activated deadline.

The separately accepted witness establishes actual warmed region 303 and linked
removal of the bool home and second dispatch. It also documents extra admission
guards and frame/IP/slot reloads. That provides a plausible mechanism for net
cost, not a proved attribution of this elapsed loss: fast/fallback frequency is
unmeasured, and the home-materialization ablation was never built. These results
reject this implementation package; they do not prove that every possible
region representation or optimizing middle-end must lose.

Independent receipt: `build/region-elapsed-outcome-review-20261101-v1/receipt.json`,
SHA256 `b709944f8f5c25d28a8b48493d277a33133fd3e42f2cce46c2e2eb1456e1144d`.
It retains all 80 normalized values, raw hashes, medians, spreads, ratios, RSS,
origin replays and terminal checks. The first host audit compared the origin
parser's broad scope label with the wrapper's explicitly narrowed label and
failed; its script/log remain preserved. The corrected audit verifies the
wrapper's exact declared label and all underlying fields. No runtime artifact
changed and no native process was rerun.

No new target, compiler, probe, benchmark or sample ran in this review. This
four-row pilot makes no all-29 parity claim and does not alter the user's GC
decision cutoff of **15:14:44 UTC**.
