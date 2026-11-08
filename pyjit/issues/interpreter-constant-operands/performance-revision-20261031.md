# Performance decides the operand experiment

This supersedes the hard code-size and instruction-count acceptance gates in
the [original held plan](performance-plan.md). The user challenged the size
gate before any timing was collected. The coordinator agrees: total evaluator
text is a cost observation, not evidence of a speed regression. Preserve the
original plan, reviews and initial rejection receipts as history. The current
decision is open.

Matched evaluator text grew from 142,471 to 168,721 bytes: 26,250 bytes, 18.4248%.
The two builds have matching configuration and toolchain; duplicate ELF symbol
aliases are counted once, with the same hot/cold family policy. That observation
remains valid. It neither proves increased hot working-set size nor determines
the effect on instruction-cache behavior, dispatch, layout or elapsed time.
No new size cutoff replaces the old 5% gate.

Keep the original registered workloads, fixed work, matched source/product/
configuration checks, two rotating-order rounds, four instruction pairs and
five elapsed samples per cell, raw-capture prerequisite and one 45-minute quiet
window. Coroutines remains primary; spectral_norm remains secondary; generators
and unpack_sequence remain controls. A secondary result cannot rescue failure
on the primary workload. No new workload or repetition is added after scores.

Use elapsed performance for acceptance: require at least 1% lower median elapsed
time on coroutines in each round, with variability small enough to support the
claim. Investigate any control regression above 2% before acceptance. Noisy or
inconclusive results do not accept the change and do not trigger automatic extra
samples. The 1% minimum concerns actual performance and the benefit required to
justify this implementation; it is not a code-size or instruction-count proxy.

Report original instruction pairs, normalized counts and evaluator text growth
alongside elapsed results as explanatory evidence. More instructions or larger
text does not automatically reject a measured speedup. Conversely, fewer
instructions cannot rescue a timing regression. Preserve causal word/native
work-removal review and semantic checks before measurement. If available,
cache/branch counters may inform a separately bounded diagnosis; they are not
new prerequisites or substitutes for the elapsed decision.

Independent review and exact quiet-window activation precede timing. Applicable
coherent runtime qualification still follows a successful performance decision.
No runtime operand-form switch is introduced to retain alternatives.
