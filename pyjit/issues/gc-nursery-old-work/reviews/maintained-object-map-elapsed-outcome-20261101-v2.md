# Maintained object-map pilot: independent elapsed outcome

2026-11-01 07:07 UTC. **Reject the current object-map increment on elapsed performance.** The completed experiment is valid; both predeclared primary and four-workload geomean gates fail in both rounds. No additional samples are warranted by this result.

Reviewed `build/object-map-elapsed-pilot-v2`: plan `e33fe9b21ccae3ad982c8dd32268ce3a888389ce7d6ffab9ea1a4c4a89345f49`, launch `0b20526104be40f7f3b4b80016b70ec289bf3a346eb32238fcbb5afd27a0af27`, final binding capsule `991c9ff247e81cb4612c57b703f3faf5d40eac0e0ebc2fcf2d09baa6666fa74b`. Candidate source is `76fb129ec28179488fd2ee012a0ae5f599779bc1`; its release image is `1fd0c6fd8529f5501b45fa496cb729b5e2313fe113834246e7dca6a24485e280`. The existing root binding review remains the configuration/toolchain/ELF bridge authority; the unchanged phase and paired helpers were replayed against the actual bindings.

The independent reader rehashed all 253 completion artifacts, replayed source/product/tool/library bindings and all twelve actual workload/image origin traces, and checked the literal order, arguments, environment and successful nonoverlapping intervals of all 36 commands. All 24 scored processes supplied five raw durations each. The 72 quiet records passed the frozen load/CPU policy; maximum observed load was 0.677246. All four product inventories match the current three images. Root confirmed supervisor 21198 terminal exit 0; gate return is 0, post-cleanup verification succeeded, and terminal cleanup records all three images idle under their locks before the deadline.

Ratios below are candidate/parent or candidate/upstream; values above 1 are slower. Medians were independently recomputed from the raw vectors with the registered loop divisors.

| Measurement | Round 0 | Round 1 |
|---|---:|---:|
| Coroutines, candidate/parent | 1.080163 | 1.054832 |
| Generators, candidate/parent | 1.013769 | 1.002744 |
| Comprehensions, candidate/parent | 1.052234 | 1.110831 |
| Richards, candidate/parent | 1.031457 | 1.003223 |
| Four-workload elapsed GM, candidate/parent | 1.044114 | 1.041969 |
| Four-workload elapsed GM, candidate/upstream | 1.173180 | 1.197602 |
| Four-workload elapsed GM, parent/upstream | 1.123613 | 1.149365 |
| Four-workload peak-RSS GM, candidate/parent | 0.996843 | 1.000286 |
| Four-workload peak-RSS GM, candidate/upstream | 3.394843 | 3.390503 |

The primary required candidate/parent <=0.99 and the elapsed GM <=1 in each round. All ten single-sample-deletion coroutine ratios remain above 1 in each round: ranges 1.062858–1.082477 and 1.033802–1.056855. This descriptive sensitivity check supports rejection; it is not a significance test. Coroutine candidate/parent sample spreads are 6.32%/6.43% and 4.33%/9.46%, and their raw ranges overlap. Generator and comprehension spreads are larger, reaching 21.94%; no precise causal or uncertainty estimate follows from five serial calls. Comprehensions exceed the 2% control-regression threshold in both rounds; Richards exceeds it only in round 0. No cause is established or compensating workload used to rescue the failed primary.

Whole-process peak RSS is nearly unchanged relative to the parent in this screen. Coroutine peaks are candidate/parent/upstream 67596/66880/14520 KiB and 67816/67196/14556 KiB. RSS includes setup and warmup, and does not measure interval live memory. Its diagnostic result does not reverse the elapsed decision.

Independent evidence: `build/object-map-elapsed-review-20261101-v2/audit.py` SHA256 `c34377ffbb752485ce821934fdd9ec7622688600cd8ff1b7e6055ba4bbaf75c2`; receipt `2e4ed47412482c6b3764e4f71c9f72980e2407082164bc57fbc405f6f3101fde`. Actual score SHA256 `f74d453371d7bbe832d7bdf96cf0b0975329ad585bbea6234ce6c4114b28c6ff`; terminal cleanup `27ea90c3f627e1e67fa576280aadc5afb424b944824c953d67274286312f80ed`.

This rejects this implementation, not every possible object-map design. Prior source/native failures remain failed and preserved; the focused correctness acceptance remains valid. Four workloads do not establish all-29 parity, collector selection, full qualification or integration. No target, compiler, extra workload, counter probe or repeated measurement ran during this review.
