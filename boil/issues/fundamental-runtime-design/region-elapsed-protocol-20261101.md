# Interpreter-region early elapsed decision

Declared before scores, 2026-11-01. Hypothesis: compiler-proven single-use
compare/forward-branch regions reduce net interpreter cost by eliminating an
intermediate home roundtrip and second dispatch. Charge guards, common handler
layout and all added code to the candidate.

Compare one source-frozen candidate with the qualified maintained tracing
parent, whose runtime is byte-equivalent to candidate parent `5c24163405d0`.
Require actual source/product/configuration and origin bindings; matched
GCC O3/evaluator/frame-pointer/zero-init, no JIT/PGO/LTO/debug/stats/tail.

Use the unchanged registered coroutines, generators, comprehensions and
Richards workloads and loops, in that order. Two fresh-process rounds, five
serial samples after warmup per arm/row: 16 scored processes, 80 elapsed
values. Alternate first arm by (row index + round index) modulo 2. Eight
separate zero-sample origin warmups remain unscored. Reuse accepted quiet
runner/gates, CPU0/sibling8, both slots and build locks, maximum load 4.
GNU time records whole-process peak RSS on scored processes only.

For each round, coroutine candidate/parent median elapsed must be below 1.0
and the four-row geometric mean at most 1.0. Review all raw vectors, spreads
and control losses; a control regression above 2% needs an explanation before
adoption. Borderline or inconsistent results earn no acceptance or automatic
extra samples. There is no arbitrary text-size or RSS veto. Review memory
as an explicit measured tradeoff.

Before interpreting elapsed, establish the actual registered site's compiler
admission/warmed region opcode, ordinary fallback semantics, and complete
linked paths covering guard costs, removed homes and receiving dispatch.
Dynamic fast/fallback frequency remains unmeasured; do not claim isolated
home-roundtrip attribution. No new stats build or hardware counter is required.

A fresh activation is at most 45 minutes including cleanup; clamp any campaign
used for the GC parity decision to its 15:14:44 UTC cutoff. This pilot cannot
establish all-29 parity or replace final qualification. A useful survivor enters
the fixed full-29 decision as one coherent candidate; no addition of isolated
speedups supplies composition evidence.
