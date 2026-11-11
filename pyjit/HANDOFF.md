# Coordinator handoff

## Selected checkpoint and next work

The DeltaBlue round selects runtime 48 `5559f60093e8216d9399bea74d0f10c8cdfb9a6b`:
canonical guarded class values and owned classmethod capture. Both increments
are committed and pushed. Current source is restored to 48 on
`pyjit/deltablue-checkpoint-20261110`. Release/debug each build 117 modules;
both full diagnostic runs execute 54,697 tests with established debugger/tooling
limits. Stats builds 117 modules and passes its 150-test smoke run. Final six-pair ratios are DeltaBlue 0.739181/starting checkpoint 46 and 1.225425/upstream, and logging 0.872129/starting checkpoint 46 and 0.758002/upstream; the 29-kernel geometric mean is 1.179430/upstream. See the [report](issues/deltablue-call-model-20261110/checkpoint-report.md),
[tables](issues/deltablue-call-model-20261110/benchmark-tables.md),
[status](STATUS.md) and [next plan](issues/deltablue-call-model-20261110/plan.md).

Astra performed one grouped architecture review; do not call it repeatedly in
an inner loop. Sol owns design/debug/peer review, Luna/Terra bounded mechanical
work. The coordinator owns native builds, merged qualification, quiet timings
and landings. Propose → peer review → implement → focused test → post-review →
integrate. Full suites belong at coherent useful checkpoints.

C49 `24d97fa8c3b5747569bac1b31fe31b8a107245ff` passes focused tests/API but is
rejected on performance. Gate-off `af60e5eb43e` is diagnostic only. Neither is
selected. Their source commits remain on separate experimental refs; no agent
is implementing C2. The reviewed [C2 proposal](issues/deltablue-call-model-20261110/C2-proposal.md)
selects eligible leaf CALLs during specialization, retains validated feedback
units, consumes checked-prefix facts and makes common unary retirement scalar.
Performance is unproved. The [completion design review](issues/deltablue-call-model-20261110/C2-completion-review.md)
adds the primary shared committed-state/observer boundary before callback-capable
retirement. Logging's ordinary-path residual remains open. Do not
expand grammar or invent recipes to rescue the generic transaction.

## Source, builds and immutable controls

BASE=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
Canonical docs: `/home/power/code/cpython/.worktrees/python-jit`, branch
`weaver/python-jit`. Its C files are not the measured runtime.

| Role | Source below BASE | Build below BASE | Source HEAD |
| --- | --- | --- | --- |
| Selected release/debug | worktrees/execution-boundaries-20261108 | execution-boundaries-{release,debug} | 5559f60093e8216d9399bea74d0f10c8cdfb9a6b |
| Selected stats | worktrees/execution-boundaries-20261108 | execution-boundaries-stats | 5559f60093e8216d9399bea74d0f10c8cdfb9a6b |
| Frozen48 comparison | worktrees/deltablue-call-pair48 | deltablue-call-pair48-release | 5559f60093e8216d9399bea74d0f10c8cdfb9a6b |
| Frozen46 round baseline | worktrees/deltablue-baseline46 | deltablue-baseline46-release | b8e7e1bee6611eeda320ce163cd3ce89323da877 |
| Golden39 | worktrees/golden-20261109-runtime | golden-20261109-release | 4d666e1b482cb4983c7683f91c5922af14d0ab40 |
| Upstream | worktrees/upstream-3a440b6 | upstream-release-devlibs-3a440b6 | 3a440b65695615e9fd9912342b79cdd54fced0d2 |
| Previous golden | worktrees/pyvalue-rc-universal | pyvalue-rc-repr-{release,debug,stats} | 0765e9be47f55d97ba5d31d2ae1c02a55ea2e5ae |

Selected magic is 64418; ATTR/private-poly geometry remains 48/120 bytes. Release
and debug check 117 modules, upstream 116. Use the checked-module build logs;
physical historical DSO counts are not qualification gates. Never mutate the
historical golden/upstream builds. Golden39 runtime tag and combined
`cb906f9bc52` snapshot remain unchanged.

Experimental source refs: `origin/pyjit/deltablue-leaf-experiment-20261110`
retains C49 (also local `pyjit/frame-call-model-20261109`);
`pyjit/deltablue-leaf-gate-off-20261110` retains the ablation. The task-owned isolated leaf-prototype worktree was archived as
`leaf-prototype-final-source.patch` in the round evidence and removed; corrected
C49 is committed. Frozen47 intermediate source/build is
retained under `deltablue-class-values47` as a read-only component comparison control. Do not
confuse any of these with the selected runtime 48 or canonical documentation.

## Reproduce and inspect evidence

Packet: `pyjit/issues/deltablue-call-model-20261110`.
Evidence: `BASE/pyjit-link-wave1/deltablue-call-model-20261110`.

- `run-checks.py`: established regeneration/build/focused/full/stats/API runner.
  Use unique tags; failed records are retained. Commit before rebuilding native
  identities. `--role` allows independent full diagnostics after a nonzero role.
- `measure.py`: balanced unchanged kernels with native/source/input preflight;
  `--control-role golden --control-head FULL` admits a frozen intermediate 117-
  module control. Upstream uses116 modules. Basic 2 pairs; target confirmations 6.
- `build-control.py`: task-owned frozen intermediate release builds using the
  same configuration and relocated module/sysroot flags.
- `census_calls.py`: profiler-altered semantic call frequencies, not optimized
  hit counts. `census_leaf.py`: actual unprofiled before/after leaf counters;
  valid only on experimental C, which exports `ir_leaf_info`.
- Current report records `delta48-final*` diagnostics and final matched timings.
  Earlier `delta47*`, `delta48*`, `delta49*`, `delta49b*`, `delta49-off*`,
  `measure47*`, `measure48*`, `measure49*` and `coverage49-leaf.json` remain distinct.
- `perf49-record-affinity` has four valid matched captures, stable identity,
  correct build IDs and zero lost samples. Decoded `*-flat.txt` reports retain
  rejected C's source costs. Never merge its samples with selected runtime 48.

Host commands use `UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B`.
Native targets run explicitly through the runner. GCC 15.2, CPU 15, hash 0, JIT off,
fixed inputs. Serialize mutations of shared build products. Independent builds may qualify
in parallel with frozen source; keep quiet timing separate from all heavy jobs.
The reviewed `object-call-20261109/capture-gap-perf.py` needs `taskset -c 15`,
`--candidate-head FULL` and a complete matching full 29-kernel, two-pair upstream screen.

## Qualification limits and documentation

Universal PyValues are integrated through the runtime and rebuilt extensions;
native method/tracing JIT tiers are unfinished. Python behavior and explicit
ownership govern admission, not exact CPython finalizer/debugger transcripts.
GDB format failures, nine unsupported debugger-jump cases and a debug trace
format assertion are established diagnostic limits; report actual current
failures separately. Do not declare full suites green or start an open-ended
GDB compatibility port. Full48 release/debug each run 54,697 tests. GDB display failures are 16/21,
frame-jump errors 9 per role, and the debug trace-format failure 1. Module names
and counts match the established limits; quiet logs do not identify individual
failing cases. Stale disassembly failures are absent. All native jobs are closed. Final full29 and six-pair target campaigns and
matched profile capture are complete.

One path: GOAL → current plan → STATUS → report/tables and IR/interpreter/frame/
value designs. Loom mirrors these; repository documentation owns execution state.
Historical reports and Loom revisions remain available rather than competing
with active instructions. WORKLOG records one concise concluded-round entry.

## Preserve unrelated changes

Never stage, reset or delete these preexisting canonical files:

- `pyjit/bench/results/checkpoint-6a0e4c12c17.md`, empty, SHA256
  `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
- `pyjit/issues/checkpoint-closure/ctypes-rawpin-build.log`, four appended lines,
  SHA256 `f37dca15ab40ec8eb18814673f351c0b545698b22611c0c025b64b4c196bb6fe`.
- Untracked empty `pyjit/issues/checkpoint-closure/w5-release.lock`.

Git retains earlier operational snapshots. Consult current Git, processes and
check journals before resuming; a prepared script is not an executed action.
