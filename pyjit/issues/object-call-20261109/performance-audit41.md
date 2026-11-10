# Candidate 41 short micro campaign audit

The two completed six-round campaigns support large improvements over golden39 in these two microbenchmarks. They do **not** establish upstream parity. The upstream class-load result is sensitive to pair order; the keyword result is slower in all six pairs. Longer measurements remain necessary before making a close upstream comparison.

This is a read-only audit of saved evidence. No runtime, benchmark, build, or native workload was changed or executed. Lightweight host-side JSON parsing recomputed the results below.

## Evidence and identity

Evidence root: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/object-call-20261109/`.

Audited directories: `measure41-micro-golden39` and `measure41-micro-upstream-v2`. In each, inspected `commands.json`, `summary.json`, `status.json`, both identity snapshots, the saved driver, and every command's raw stdout and stderr.

Both campaigns have `failure: null`, `postflight_error: null`, and `stable_identity: true`. Their before/after snapshots are equal with empty error lists. Source worktrees were clean at the captured identities:

| Role | Captured revision | Modules |
| --- | --- | --- |
| Candidate | `0a35a382909ca169ce3f381cb38206b1725001b3` | 117 |
| Golden39 | `4d666e1b482cb4983c7683f91c5922af14d0ab40` | 117 |
| Upstream | `3a440b65695615e9fd9912342b79cdd54fced0d2` | 116 |

The identity records include executable/build identity, module-log hashes, and driver/benchmark source hashes. The saved drivers and the two benchmark bodies still match their captured hashes at review time. This audit checks recorded identity consistency; it does not independently rebuild the interpreters or certify machine isolation.

Each campaign contains exactly 24 successful timed invocations: six rounds, two cases, two roles. All stderr files are empty. Raw stdout matches the recorded metadata and seven durations in every row. All rows use CPU 15, JIT disabled, hash randomization disabled, `-S -B`, 200 loops, and seven samples. Executable revision metadata matches the corresponding preflight record.

For each case, odd rounds run control then candidate; even rounds reverse this. Thus each case has three pairs in each order. The seven-sample arithmetic mean in every command and all six paired ratios and geometric means recompute exactly (up to floating-point comparison). No samples or rounds were dropped.

## Results

Ratios are candidate time / control time; lower is faster. These are geometric means of six paired ratios, each based on seven within-process durations.

| Control | Case | Paired geometric mean | Pair range | Interpretation within this campaign |
| --- | --- | ---: | ---: | --- |
| Golden39 | `class_attr_load` | 0.157339 | 0.156256–0.158135 | 6.3557× speedup; 84.27% less time |
| Golden39 | `call_py_kw` | 0.635497 | 0.620984–0.641594 | 1.5736× speedup; 36.45% less time |
| Upstream | `class_attr_load` | 1.032405 | 0.971137–1.105259 | 3.24% more time in aggregate; order-sensitive |
| Upstream | `call_py_kw` | 1.092138 | 1.035407–1.124799 | 9.21% more time; all six pairs slower |

The golden class result is large and consistent: control-first and candidate-first geometric means are 0.157324 and 0.157355. Keyword order splits are also close, 0.634444 and 0.636551. The short sample duration limits precision, but the observed 6.36× class improvement is far larger than the order variation seen against upstream. It is strong evidence of a real improvement in this case, not evidence of a general object-model speedup.

Against upstream, class order splits are 1.092104 when upstream runs first and 0.975969 when candidate runs first. This is a substantial association with order, not a diagnosis of its cause. Three class pairs are faster and three slower. Balanced ordering is useful but does not establish that its aggregate removes all bias. Neither “parity” nor a precise 3.24% regression is justified yet. Upstream keyword order splits are 1.086272 and 1.098036: the saved evidence consistently points to a remaining cost, pending confirmation.

## Timing interval and scope

The timed region is `module.run(loops)` after one untimed warmup call. Setup, imports, process startup, and metadata printing are outside it. Each loop executes 1,000 operations, giving 200,000 class loads or keyword calls per measured sample. This is whole benchmark-body time, including its loops and assignments, not isolated opcode latency.

| Campaign / case | Control sample range (ms) | Candidate sample range (ms) |
| --- | ---: | ---: |
| Golden39 / class | 10.781–11.571 | 1.681–1.809 |
| Golden39 / keyword | 19.380–20.745 | 12.306–13.163 |
| Upstream / class | 1.609–1.930 | 1.700–2.366 |
| Upstream / keyword | 11.055–12.896 | 12.306–13.551 |

Class sample means against upstream span 1.624–1.868 ms for upstream and 1.715–2.001 ms for candidate. The candidate's 2.366 ms individual sample must not disappear behind a shorthand “1.6–2.0 ms” description. Process wall intervals are much longer (roughly 38–40 ms upstream class and 60–66 ms candidate class), but these are not the reported timing intervals.

Golden capture recorded 14 quiet-wait entries; maximum invocation load before/after was 2.9981. Upstream-v2 recorded no waits; maximum was 1.6592. These satisfy the helper's recorded load gates. Load averages and CPU affinity do not prove that the pinned CPU, SMT sibling, or shared resources were otherwise idle. Seven successive samples within one process are not seven independent machine-level trials; no confidence interval is claimed here.

The class case repeatedly reads integer `K.k` through one instance. The keyword case repeatedly calls one three-parameter function with three keywords. These results do not establish performance across descriptor kinds, mutable dictionary layouts, keyword fallback routes, or the broader kernel suite. They also do not themselves prove which specialization executed; separate IR/counter evidence is needed for that attribution.

## Required follow-through

Freeze the helper before the planned class 20,000-loop and keyword 2,000-loop confirmations. Keep bodies, timing boundaries, and paired roles unchanged; record actual counts and inspect order strata again. Treat these as separate campaigns rather than pooling short- and long-loop durations. Longer intervals reduce sensitivity to brief disturbances but still require balanced order and a quiet machine.

The parent reports that `basic94738` began before the helper was edited. Its eventual identity comparison must determine rejection, and a failed identity capture must remain diagnostic evidence. Do not restore the helper to conceal the change or admit a summary independently of failed status. This audit covers only the two completed micro directories above; it does not admit that basic campaign or any pending full-suite results.
