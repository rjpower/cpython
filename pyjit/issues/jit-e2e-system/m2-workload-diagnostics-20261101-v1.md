# M2 natural workload coverage diagnostic

2026-11-01. Six unchanged registered workloads completed in six fresh candidate
debug processes at accepted source `3174bc0ab726cb7b7b39f25d93f4ec9194c88d3a`,
image `f92c752a29d365b044b2603e376baedc4c9e864b7bd2f75d96ceee3c49a1e722`.
**No queried function has a new native-entry witness.** This is useful negative
coverage evidence, not a speed comparison or rejection of the JIT architecture.

The unchanged diagnostic runner uses the fixed six-program matrix, original
driver/kernel hashes, variants and loop counts. It requests no compilation.
There is no reference arm, warmup repetition, additional sample or scored value.
Extra Richards/deepcopy correctness work follows each after-snapshot. The
readiness review found a missing replay of actual current products; that check
and frozen CAPI hash equality were added before execution. Earlier held bindings
remain preserved. A separate prelaunch timer correction excludes cleanup from
the whole-process duration.

| Program | Queried function | After-snapshot observation |
| --- | --- | --- |
| Richards | `schedule` | Coverage refused: word35, `LOAD_COMMON_CONSTANT`79 |
| Richards | `Task.runTask` | Coverage refused: word19, `LOAD_COMMON_CONSTANT`79 |
| Richards | `Task.qpkt` | Coverage refused: word10, `LOAD_COMMON_CONSTANT`79 |
| DeltaBlue | `change` | Coverage refused: word31, `LOAD_COMMON_CONSTANT`79 |
| DeltaBlue | `Plan.execute` | Coverage refused: word10, `LOAD_COMMON_CONSTANT`79 |
| DeltaBlue | `ScaleConstraint.execute` | Coverage refused: word17, `LOAD_COMMON_CONSTANT`79 |
| deepcopy | `benchmark_memo` | Coverage refused: word29, `LOAD_COMMON_CONSTANT`79 |
| generators | `bench_generators` | Coverage refused: word16, `LOAD_COMMON_CONSTANT`79 |
| generators | `Tree.__iter__` | No nomination/compile attempt observed |
| coroutines | `bench_coroutines` | Loop heat4; no compile attempt observed |
| coroutines | `fibonacci` | No nomination/compile attempt observed |
| logging | `bench_silent` | Coverage refused: word67, `RAISE_VARARGS`102 |

All12 queried functions report zero compiler attempts, failures and compiler
nanoseconds. Nine were rejected by the cached coverage check before compiler
entry. The other three did not nominate compilation during this run. None
reports a quota failure. End snapshots show10–22 live/retired Units and roughly
60–123KB of charged process metadata/code, below the selected64-Unit/8MiB bounds.
These process snapshots are shared occupancy and must not be summed across
queried functions. They exclude unqueried per-code compilation totals and do not
measure complete compiler or allocator cost.

Each workload returned successfully. Richards' additional run returned True;
deepcopy preserved aliases without retaining the original list; DeltaBlue's
failure-output stream stayed empty. Other correctness evidence is limited to
the unchanged kernel/setup checks and successful process result. Start/end
records verify the exact executable/CAPI, isolated debug configuration, own
stdlib route, mapped-file hashes and unchanged verification state. Lifetime
seen bits are not execution counters; an inactive artifact alone would not
prove that no earlier artifact ran. This report does not infer dynamic helper
or exit frequencies, release performance, or broad semantic qualification.

Supervisor46593 exited0. Controller cleanup completed within its930-second
inclusive window; root independently rechecked native idle,24 available locks,
9,695 completion artifacts,587 actual current products,2,353 prior files and30
historical fingerprints at20:12:14 UTC. No candidate was rerun by the coordinator
or reviewer after these six processes.

Evidence directory: `build/jit-method-m2-workload-diagnostics-20261101-v1/`.
Its twelve before/after target snapshots live in the six program JSON files;
each row also retains the launch, log, origins, process result and cleanup.

- Manifest SHA256: `a209570980ba227e6d0c0488a6267a8364c322843d63b2633e88686ba302078c`.
- Completion SHA256: `2487194f2f0c6f7cfdf4d1da3b6933c31c705db8681bde8292b43a3ca3d426a5`.
- Controller cleanup SHA256: `80b296584813326753130cac9c7eed03a0abd6acfd450afb545214dbb6a8dc78`.
- Independent readiness: `build/jit-method-m2-workload-readiness-review-20261101-v1/`,
  review439a92c2/receipt9dd8eebc. The
  [independent outcome review](m2-workload-outcome-review-20261101-v1.md),
  receipt6100598e and [decision](m2-workload-decision-20261101-v1.json) accept
  the evidence as a valid negative inventory. The reviewer's initial timeout-argv
  prefix assumption failed and was corrected; both readers are preserved.

The immediate mechanism change is the independently reviewed
[two-helper coverage amendment](m3-coverage-amendment-20261101-v1.json): generate
`LOAD_COMMON_CONSTANT` and `RAISE_VARARGS` from the same analyzed semantics,
inside the existing M3 source deadline. Preserve immortal/common-constant
handling and exact raise-window consumption, error/unwind phases and callbacks.
An implicit-None-return native loop and raise0/1/2/invalid/callback tests must
accompany the change. The M2 image and this campaign remain frozen.

A separate later question is whether natural admission should reject a whole
method for an unsupported word on an unexecuted path, given that the baseline
already represents before-effect exits. That requires an explicit liveness and
retry-policy review; it is not an unrecorded expansion of the current M3 grant.
Observed-loop recording, shared SSA, register allocation and recovery remain
the active cohesive implementation, rather than another per-op timing vote.
