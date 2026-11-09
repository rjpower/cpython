# Nbody work ledger

The third engineering image retains a repeated 5.6% elapsed regression against
golden. Warmed PMU ratios are instructions 1.03325, cycles 1.05381 and CPI
1.01990, with branch misses essentially unchanged. Four checked numeric final
stores did not close the observed instruction gap. Sampling identifies hot
arithmetic and subscription paths; it does not attribute the regression.

At the next coherent source freeze, rebuild the existing candidate stats build
and capture unchanged warmed nbody at 0, 2 and 4 outer iterations, matched to
the immutable golden stats build. Use existing handler counters only, with no
source instrumentation. The prepared driver is `capture-nbody-census.py`.
It checks JIT-off, affinity, ELF/workload identity and observer absence, records
selected IR, dumps immediately after the interval, subtracts the empty harness
and tests linear scaling. Host orchestration must additionally record clean
source/build identities and sanitized environment before and after capture.
Keep raw counters and nonlinearity visible; do not normalize away deopts or
attribute global counts to a particular site.

Compare executed arithmetic, subscription, unpack, move and completion handler
counts first. Then inspect matching uninstrumented release routes and their
actual operand/destination categories. A conditional machine-code count can
exclude a hypothesis but is not a cycle prediction. The shared completion
helper and discard-recipient branches remain hypotheses, not selected fixes.

If existing counters cannot distinguish a material route, propose one bounded
diagnostic counter at its actual semantic selection point, including outcome
accounting, and peer it before implementation. Retire temporary diagnostics
before the balanced release timing and PMU repeat. Never time a stats binary or
credit a source-level simplification with a measured component gain.
