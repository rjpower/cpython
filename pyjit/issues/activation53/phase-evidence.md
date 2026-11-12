# Selected activation phase evidence

E=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/activation53`.
Native selected runtime is golden51 `0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`;
upstream is `3a440b65695615e9fd9912342b79cdd54fced0d2`. No candidate exists yet.

## Diagnostic phase census

[phase-census.py](phase-census.py) loads unchanged full kernels, runs setup and
one unobserved warmup loop, then one loop with code-local monitoring callbacks.
It counts observable phases only for code defined in each kernel. This perturbs
execution and is not a timing/fast-path measurement or a native eligibility/hit
census. It excludes stdlib work and setup. Native commands, per-code geometry
and counts are in E/`census-commands.json` and `census-{role}-{case}.json`.

| One unchanged kernel loop | Starts | Resumes | Yields | Returns | Peak observed depth |
| --- | ---: | ---: | ---: | ---: | ---: |
| Generators | 100002 | 1568946 | 1568946 | 100002 | 19 |
| Coroutines | 242787 | 0 | 0 | 242787 | 27 |
| DeltaBlue | 51422 | 0 | 0 | 51422 | 13 |

Selected and upstream counts match exactly, with balanced final observed depth
zero and no PY_UNWIND events. Tree.__iter__ contributes 100000 starts/completions
and every generator yield/resume; coroutine Fibonacci contributes 242785
starts/completions and no yields. DeltaBlue's leading call counts are
BinaryConstraint.output 13717, BinaryConstraint.input 11707 and
EqualityConstraint.execute 10200. Its hot functions have zero hidden constant
homes; Fibonacci has one and Tree.__iter__ has zero. This does not measure
physical non-NULL homes or constructor/resume specialization hit rates.

The data ranks repeated resume/yield work for generators, creation/completion
for coroutines and small ordinary calls for DeltaBlue. It does not assign a
cycle budget to each phase or prove all starts take exact VM admission.

## Fresh selected-image coroutine sampling

E/`baseline-coroutines-perf` uses the established FIFO post-warmup protocol,
calibrated from the sealed golden51/upstream all29 campaign. Both profiles are
valid, zero-loss, with stable source/native/extension/configuration identities.
The selected flat report has about 2533 samples: exact CALL 9.28%,
RETURN_GENERATOR 7.62%, CompleteGeneratorReturn 6.45%, GET_AWAITABLE 6.19%,
gen_dealloc 6.02%, ClearLocals 5.58%, SEND_GEN 4.86%, RETURN_VALUE 4.47%.
Shares are sample locations, not removable budgets. No new elapsed comparison
is inferred. Upstream's collapsed evaluator means per-handler symbol shares
cannot be compared directly.

Existing selected51 generator/DeltaBlue profiles and PMU counts remain sealed
under the interpreter-round51 evidence root. Prior all29 upstream elapsed mean
is 1.156535; generators/coroutines/DeltaBlue six-pair ratios are
1.445513/1.267666/1.159832. Those older campaigns are context, not fresh timing.

E/`census-provenance.json` retains source heads, native ELF hashes and script, kernel, command and result hashes. The profile status records its full independent identity census.
