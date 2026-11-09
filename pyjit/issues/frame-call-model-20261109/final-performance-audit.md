# Final-source performance receipt audit

I independently recomputed the four closing39 campaigns from their retained
`commands.json` rows. Every measurement has seven positive samples, the
registered loop count and variant, the expected CPU-15 command, JIT-off/native
revision/hash-seed metadata, a successful exit, and a load sample below the
campaign limit. Before/after identity JSON files are byte-identical, status
records no failure, and the per-case ratios match `summary.json` to floating
precision. These receipts name committed `4d666e1b482` as the candidate.

| Comparison | Raw measurement rows | Candidate/control geometric mean |
| --- | ---: | ---: |
| 29 kernels / golden, two alternating rounds | 116 | 0.934191620 |
| 29 kernels / upstream, two alternating rounds | 116 | 1.174985034 |
| `logging_silent` / upstream, six balanced rounds, campaign 1 | 12 | 0.906734623 |
| `logging_silent` / upstream, six balanced rounds, campaign 2 | 12 | 0.897500352 |

All twelve logging pairs are below 0.924. The largest golden ratio is
`json_loads` at 1.008427; the original large golden losses no longer repeat
in this screen. Against upstream, `deltablue` is 1.619651,
`generators` 1.415179 and `nbody` 0.841104. The upstream 29-kernel aggregate
therefore remains slower despite meeting the specified logging gate. These
are final-image comparisons, not an attribution of any source change.

The first supplemental attempt stopped at preflight because upstream has 78
shared extensions, while the wrapper incorrectly expected 213. It measured
nothing. The corrected `closing39-supplemental-v2` receipt is complete: six
serial sweeps cover 65 registered micro instruction workloads and six timed
memory workloads on upstream, golden and candidate. Each micro row has four
low/high pairs at loops 200/600, one 1,000-iteration inner loop and its
registered skeleton; each memory row has one warm-up and five timed calls.
All three raw perf journals have 586 invocations (one probe and 65 times nine
workload invocations), and their SHA-256 values match the reports. Before and
after input/native snapshots have the same SHA-256. RSS-only startup/imports
were intentionally excluded. The single serial sweep gives diagnostic coverage;
it is not a balanced elapsed comparison or a 65-row parity metric.

Evidence directories under E11 are `closing39-kernels-golden`,
`closing39-kernels-upstream`, `closing39-logging-{1,2}` and
`closing39-supplemental-v2`. The initial supplemental preflight failure is
retained separately at `closing39-supplemental`.

## Current upstream hardware counters

The closing39 gap campaign has 18 valid `cycles:u`/`instructions:u` captures:
three rounds for each of `deltablue`, `generators` and the `nbody` control on
both upstream and candidate. Every event reports 100% running. I divided each
raw counter by that invocation's actual hot-loop count, took the three-round
mean for each role, then formed candidate/upstream ratios. The source/native,
CPU, JIT and hash-seed metadata match before and after each capture; the
campaign's identity JSON files are byte-identical.

| Workload | Cycles/loop ratio | Instructions/loop ratio | CPI ratio |
| --- | ---: | ---: | ---: |
| `deltablue` | 1.654886 | 1.869213 | 0.885338 |
| `generators` | 1.442485 | 1.415902 | 1.018775 |
| `nbody` | 0.849243 | 1.128690 | 0.752415 |

The large instruction excess in `deltablue` and `generators` is measured;
`nbody` shows why instruction count alone does not predict elapsed time. These
counters do not assign cost to a particular call, owner or dispatch helper.
Evidence: E11/`closing39-gap-perf-stat` `invocations.json`,
`identity-{before,after}.json` and `status.json`.

The separate E11/`closing39-gap-perf-record` campaign completed all six
five-second captures with stable, identical before/after identities. Each
report says `Total Lost Samples: 0`; the six retained executable build-ID
records match the captured binaries (candidate
`ddb967d967336a7fa04766d44ca2b6272946a65a`, upstream
`ff51c387c04ccd5a2664e5a572bc68e491ec0db1`). I read each existing
capture with `perf report --stdio --no-children --call-graph none --sort symbol
--percent-limit 1`; all six reports exited successfully and resolved symbols.

| Workload | Candidate's largest flat cycle-sample locations | Upstream's largest flat location |
| --- | --- | --- |
| `deltablue` | `CALL_PY_EXACT_ARGS` 10.24%; `LOAD_ATTR_SHAPE_LOCAL_TEMP` 7.00%; `LOAD_ATTR_METHOD_WITH_VALUES` 6.43%; `_PyTypeCache_Lookup` 6.18% | `_PyEval_EvalFrameDefault` 64.90% |
| `generators` | `SEND_GEN` 26.75%; `YIELD_VALUE` 12.36%; `LOAD_ATTR_SHAPE_LOCAL_TEMP` 6.55% | `_PyEval_EvalFrameDefault` 70.95% |
| `nbody` | `BINARY_OP_MULTIPLY_FLOAT` 14.38%; `BINARY_OP_SUBTRACT_FLOAT` 9.93%; `BINARY_OP_SUBSCR_LIST_INT` 9.13%; `LOAD_FAST_BORROW` 8.92% | `_PyEval_EvalFrameDefault` 75.36% |

The upstream loop is a single large symbol while the candidate exposes
individual handlers; these percentages are locations within separate
profiles, not comparable cost allocations. `generators` spends samples at
resume/yield sites, consistent with the workload's roughly one generator
creation per thousand yields. The deltablue profile warrants examining
call and attribute paths, while the measured instruction excess remains
unattributed to any single one. `nbody` is a negative control for a general
slowdown claim: it runs faster than upstream here despite more instructions.
