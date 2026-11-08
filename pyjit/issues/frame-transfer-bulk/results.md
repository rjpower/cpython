# Frame-transfer result: reject the bulk-copy implementation

The source-equivalent candidate is not selected for integration. It passes the
bounded OFF behavior checks, but two independent process rounds show no
repeatable benefit in the targeted coroutine/generator work. Retain its source
and evidence; do not add size thresholds, build another configuration or repeat
measurements to rescue it. This is an experiment decision, not proof that frame
transfer has no further optimization opportunity.

Candidate source/native `5b4d7d9126cbf95122e29065afeda0fa726128c7`, isolated
`boil-frame-transfer-bulk` from `b3f2bcc605c`. Only the reviewed frame header
changed; runtime/library/test/tool sources otherwise match qualified parent
`0b15b4bbc9b`. Source is retained on the pushed topic branch and is not integrated.

Fresh accepted O3 configure/make and cleanup pass in 117.84 seconds, including
116 module checks with zero import failures. Compiler/tool hashes and effective
O3/NDEBUG, frame-pointer, zero-init and eval-fno-gcse policies match the retained
parent. No baseline rebuild. [Build result](../../../../boil-frame-transfer-bulk/build/frame-transfer-validation/release/build-result.json)
is retained in the owned worktree; current source/config/tool/product guards pass.

The scalar transfer loop is replaced in the linked evaluator by calls to
`memcpy` at ELF `0x458ac3` and `memset` at `0x458ad9`. The evaluator's size grows
from `0x22d5c` to `0x22da8` (76 bytes). These calls make small-frame overhead an
actual experimental question; simpler source is not evidence of faster code.

Ten protected existing cases pass once on actual release OFF/N0: four modules,
zero skips, five paired driver/workers, 128 recorded collections, zero fallback
and zero detached snapshot visits; verification/cleanup pass.

The next row incorrectly requested KILL on that release image. Startup rejects
it with `PYTHON_GCVERIFY=kill needs a debug build`, native1/host1 before any test
worker. The launch/configuration failure remains failed, with no test or kill
credit, no successful-row cleanup credit and no retry. Postterminal source,
configuration, tools and original products pass the unchanged guard. Coordinator
accepts OFF feedback to judge this performance experiment; applicable debug/
sanitizer qualification is deferred until code and cost justify landing.

The frozen [feedback index](../../../../boil-frame-transfer-bulk/build/frame-transfer-validation/focused-feedback-completion.json)
is `4b63cfe7c087190d281c6aea4cb13ac34535aba175dc7e696eb00d57f6a2c21d`.

## Completed quiet comparison

Two predeclared paired-four rounds ran from 15:33:00.263 to 15:34:20.556 UTC on
2026-10-31 (80.29 seconds). Both finish native/inner/gate0, with source/product
postguards and cleanup true. The quiet reservation was released at15:34:48 UTC.
Existing byte-identical driver/supervisor, CPU0/sibling8 policy, global two
slots, both build locks, clean environments and load<4 admission were retained.
No failed admission, retry or sample removal occurred.

Registered loops are coroutines4, comprehensions6000, generators2, Richards2.
Each row/image has one warmup followed by five serial samples per fresh process;
the second round starts new processes. Side order remains balanced by row.
All **80 elapsed values** are retained. Candidate qualification is experimental
OFF focus10, not LAND/full qualification. Actual image/Lib/extensions feedback
seeds the explicitly limited provenance bootstrap; each round's accepted own
JSON trace witness passes. This is not per-workload import tracing.

| Parent/candidate elapsed-median ratio | Round 1 | Round 2 |
| --- | ---: | ---: |
| Coroutines | 1.002263 | 0.965235 |
| Comprehensions | 1.110455 | 0.920576 |
| Generators | 0.996630 | 0.992480 |
| Richards | 0.980551 | 1.002656 |

Above1 favors candidate. Coroutine gain is effectively absent in round1 and
reverses in round2; generator point estimates are slightly worse in both.
Comprehension ratios change sign, with candidate within-process spreads41.23%
and55.28%. Corresponding generator spreads are21.13%/21.55%. Spread means
`100*(max/min-1)`. Ranges and per-process medians remain descriptive; serial
samples are correlated and only two process rounds provide no formal confidence
interval. The results do not establish a precise causal regression. They provide
no repeatable gain to justify larger qualification or integration. Binary layout
also changes, so source-equivalence does not isolate every cache/layout effect.

The [terminal index](../../../../boil-frame-transfer-bulk/build/frame-transfer-paired/completion.json)
is `735b6e563d152a6ea090128c3263c4f414a1abb039bba6bb6cbb63e9c668fab8`
(190 absolute artifact hashes). Raw stdout, normalized vectors, commands,
load observations, image/source manifests, witnesses, original startup failure
and postguards remain linked. The [offline replay](../../../../boil-frame-transfer-bulk/build/frame-transfer-paired/replay.py)
(`be583fb122018321ad284895d5c7885312a40e1d3ca315c1aabadefb190ba990`)
recomputes all80 values, medians/ranges/spreads and eight ratios, checks the
original scores and terminal guards, and records input hashes in
[review.json](../../../../boil-frame-transfer-bulk/build/frame-transfer-paired/review.json)
(`6235b617a2acdf49040a84590311ce5ec0265ea9ff5824e0c4cf0bef55ef3e5b`).
Run it with host `uv run --no-project python`; it starts no native target.

No debug/ASan build, full suite, new CPU profile or29-row score was produced.
The rejected branch remains separate from primary and the qualified serving
image. Further interpreter hypotheses need separate bounded selection; this
experiment supplies no reason to delay the independent young-space work.

Coordinator review independently replayed all 80 elapsed values and eight ratios,
and verified all 190 terminal artifact hashes. The rejection is accepted.
