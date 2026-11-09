# Held performance decision

Prepared 2026-10-31. This preselects consumers and a decision rule before
measurements. It grants no execution: first complete the independently reviewed
[native feedback](feedback-plan.md), then freeze exact sources, images, literal
commands and a quiet reservation in a separately activated manifest.

## Consumer and controls

The primary affected workload is the existing registered **coroutines** kernel
(four loops). Its recursive `fibonacci` has `n - 1` and `n - 2` at line 17,
inside timed work. This addresses a measured weak workload without inventing a
replacement benchmark. The secondary affected workload is **spectral_norm**
(one loop): `eval_A` has two RHS `+ 1` sites at line 26. These are source
candidates, not proof that the compiler folds them or that they dominate cost.

Use **generators** (two loops) and **unpack_sequence** (4,000 loops) as unrelated
controls. The generator's `i + 1` occurs in untimed tree construction; its timed
`bench_generators`/`run` functions contain no eligible literal arithmetic. The
unpacking body repeats sequence assignments and has no selected arithmetic
consumer. Confirm emitted code and distinguish whole-process setup in
instruction results from timed calls before interpreting either control.

The host-only source inventory is
`build/constant-operands-planning/source-inventory.json`. It binds the coroutine,
spectral-norm and generator module bytes and candidate expression locations;
it does not execute a target or establish compiler eligibility. The unchanged
[registry](../../bench/run.py) owns names and loop counts.

## Prerequisites and bounded exposure

Require actual new words at both coroutine sites, removal of their separate
constant dispatch/slot work and RHS admission, retained dynamic-left/error
behavior, passing focused semantics and an evaluator-size comparison. Reject
or redesign a representation that does not remove the nominated work before
timing. Keep micro diagnostics separate from the workload decision.

Compare the accepted arithmetic parent with the exact operand candidate using
matched GCC O3 configuration, backend, verifier policy and workload bytes.
Existing sealed parent products may be reused only through a source/product
bridge and unchanged compiler/dependency/configuration checks. A concurrent GC
candidate is not part of this isolated attribution. A later combined landing
needs its own coherence and regression checks.

Propose one 45-minute quiet window, including admission and cleanup. Use the
existing benchmark driver and gate/CPU/sibling/lock policy, fixed counts,
seed zero, fresh writable caches and the current measurement contract. Run
two rounds in rotating baseline/candidate order for the four preselected rows,
pinning `--instr-runs 4 --time-runs 5`. Across sixteen row/build/round cells,
that is 64 low/high instruction pairs and 80 elapsed samples, apart from
probes and warmups. The current runner saves normalized instruction differences
but overwrites individual perf results. Before activation, require a separately
reviewed runner-level capture of both original low/high counts and their perf
records, together with the normalized differences. Do not claim those raw
pairs from the current JSON alone. Preserve every raw vector, spread,
normalization and failure; these serial samples are not independent randomized
pairs. The runner strips inherited `PYTHON*` settings and sets seed zero;
benchmark children earn OFF/N0 coverage only, not inherited checker or
root-statistics coverage. Bind literal
commands and finite remaining-time caps before activating the window. Stop
on a correctness, origin, load/admission or measurement failure; do not tune
counts or add repetitions after seeing results.

## Accept or reject

The proposed useful-increment gate is at least 1% fewer instructions and a
lower elapsed median for **coroutines in each round**. Report spectral_norm
separately; it cannot substitute for a failed primary gate. Investigate an
elapsed regression above 2% in either unrelated control before acceptance.
Noisy or inconclusive evidence does not accept the change and does not justify
more samples without a new bounded decision. Before viewing timing scores,
require at most 5% growth in the evaluator's native function text against the
matched parent; exceeding that cap rejects this increment before timing.
Report the actual growth alongside the savings. No full-kernel geometric-mean claim
comes from these four rows.

A passing decision selects the candidate for applicable coherent runtime
qualification. A failure rejects or narrows this source increment; it does
not add a permanent operand-form switch to preserve optionality.
