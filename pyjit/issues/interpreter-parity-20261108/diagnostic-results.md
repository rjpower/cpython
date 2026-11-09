# Initial diagnostic results

2026-11-08. Runtime remains qualified R5 `0953757f4e7`; no new runtime
optimization is integrated. Evidence directory:
`python-boil-the-ocean/build/pyjit-link-wave1/interpreter-parity-20261108/`.

## Pinned elapsed baseline

The six balanced rounds in `baseline-target/` give candidate/upstream
ratios 1.648208, 1.665205, 1.636744, 1.646092, 1.658995 and 1.689003;
geometric mean **1.657289**. The coordinator rechecked all 12 child outputs
and 84 timings against the journal, arithmetic, native CPU15/JIT/hash checks,
load limits, empty stderr and stable before/after identities. The collector
had independent source review before execution. Independent result review is
a separate gate. This is a baseline, not a source improvement or parity.

## Existing statistics, before adding instrumentation

The unchanged R5 stats executable ran `capture-existing-stats.py` three times,
with 20,000 warmup loops and intervals of 0, 1,000 and 2,000 loops. Each
process cleared stats, enabled them, ran the original logging workload,
dumped while enabled, disabled and cleared stats. Native assertions checked
CPU15, JIT disabled and absence of the natural-method admission API. The
three processes exited successfully, emitted a dump and validated empty log
output. `existing-stats/` retains commands, stdout/stderr, dump/marker files,
source/build hashes and the scaling summary.

| Event | Count per workload loop after empty-interval subtraction |
| --- | ---: |
| Handler dispatches | 161 |
| Frames pushed / inlined Python calls | 20 / 20 |
| General / exact Python call handlers | 10 / 10 |
| Method / shape loads | 20 / 20 |
| Dictionary subscriptions | 10 |
| Message-local snapshots | 10 |
| Global-module / common-constant loads | 10 / 10 |
| Boolean branches / returns | 20 / 20 |
| Retained result POPs / NOP boundaries | 9 / 10 |
| Range iterations / backjumps | 1 / 1 |

All listed differences scale exactly twofold. Raw total dispatches are
57 / 161,057 / 322,057 and frame counts are 3 / 20,003 / 40,003. These are
global interval counters, not a per-code-object trace. Empty subtraction and
exact scaling support workload attribution; warmed IR identifies matching
sites. Py_STATS changes code, dispatch state and register pressure. Its counts
do not estimate elapsed savings or prove release branch frequencies.

The ten static benchmark POP sites include the pre-loop `truncate_stream`
result. Only nine execute per loop: the tenth logging call already has
`IR_NO_DST` (4095) and is followed by the backjump. Earlier review wording
about ten *hot retained* POPs was inaccurate. The NOP at word 5 of
`Logger.isEnabledFor` preserves the `try` source-line boundary and accounts
for ten NOP executions per loop. These boundaries have observer semantics;
deleting them is not an admissible fusion.

The first host summary accidentally iterated only the empty interval's printed
counter keys, omitting handlers whose zero counts are not printed. This was
corrected before reporting: use the union of all three dumps' keys, treating
absent execution-count fields as zero. Raw dumps were unchanged. Independent
review confirms the corrected totals.

## What this establishes for the comparison

General call entry and the exact inner call both execute, so changing only
the general route does not cover every frame. Return/observer-boundary and
dictionary completion designs are real competing mechanisms. Nine POPs and
ten NOPs are about 11.8% of dispatches, **not** 11.8% of elapsed time or a
promised gain: their semantics and dispatch replacement have costs.

Frame inspection shows zero constant homes in both hot callees. Existing
known-NULL overlap reuse and locals-only clearing remain in place. Remaining
questions include actual call-route outcomes, dictionary leaf-hit coverage,
post-retirement observer states, and the generated GCC instructions that a
reviewed mechanism can remove. Those questions precede selecting a batch.
