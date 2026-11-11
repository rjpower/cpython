# Coordinator handoff

## Current execution, 2026-11-11

Round 51 started at 22:28 UTC. Active packet:
`pyjit/issues/interpreter-round51`; evidence:
`BASE/pyjit-link-wave1/interpreter-round51`.
Astra owns five grouped options; Terra is simplifying current documentation;
Sol is preparing reusable subtree qualification and the narrow GDB diagnostic
quarantine. Coordinator is building immutable `round51-baseline` source/build
from `bf810ec1a33` using the existing release configuration. No new runtime
feature is integrated. The completed round-50 state below remains the baseline.

Canonical documentation: `/home/power/code/cpython/.worktrees/python-jit`,
branch `weaver/python-jit`. Its C files are not the measured runtime.
BASE=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
D=`pyjit/issues/deltablue-call-model-20261110`.
E=`BASE/pyjit-link-wave1/deltablue-call-model-20261110`.

Runtime S=`BASE/worktrees/execution-boundaries-20261108` is clean on
`pyjit/deltablue-checkpoint50-20261111` at
`bf810ec1a33cacb94102c73779d015c18f85aa8d`. This selects A's canonical method
owners plus explicit observer-error recovery and a general source-free execution
regression. Native release/debug/stats identify this commit and check 117
modules. Focus passes 804 tests in each release/debug (7/5 skips), stats smoke
passes 150, strict value API passes 306 units with zero failures/parse errors.
Final source/native, performance and diagnostic review admits this checkpoint
with the report's performance tradeoffs and established diagnostic limits.

**All jobs are closed.** Full release/debug diagnostics ran concurrently after
all quiet measurements. Logs:
`E/delta50final-rfull-release-full.log` and
`E/delta50final-dfull-debug-full.log`; journals use the same tags plus
`-full-checks.json`. Each ran 54,708 tests. Release reports 16 GDB display
failures and nine debugger-jump errors; debug reports 21 display failures, one
trace-format failure and nine debugger-jump errors. Both exit 2. Their failing
module groups and counts match checkpoint48; no additional modules fail. Quiet
logs do not establish individual failing-case identity. All workers are done.

## Completed measurements and decision

[Report](issues/deltablue-call-model-20261110/report50.md) →
[tables](issues/deltablue-call-model-20261110/benchmark-tables50.md) →
[plan](issues/deltablue-call-model-20261110/plan.md).
One grouped Astra review preceded implementation; Sol handled implementation
and independent review, Terra documentation, coordinator integration.

The source A commit is `e014965013bb11b5db400bb23e50a9a22fcae340`.
The adjacent method-to-frame experiment was committed, reviewed and tested,
then rejected. Final C `f267e9209cf9014441aaedbf10dd1c9ed88207dc` is pushed
on `origin/pyjit/deltablue-proof50-20261111`. Direct six-pair C/A gives
DeltaBlue 1.028054, logging 1.140367, nbody 1.008522. No C-specific handler,
helper or generator variant remains in selected source. Do not iterate C or
restart the deferred C2/leaf queue.

All three final 29-kernel campaigns use two balanced pairs per case:
`final50-all29-{golden39,upstream,frozen48}`. Means are
0.996588 / 1.177045 / 0.997971. Six-pair target campaigns are
`final50-targets-{golden39,upstream,frozen48}`:

| Case | / golden39 | / upstream | / frozen48 |
| --- | ---: | ---: | ---: |
| DeltaBlue | 0.685934 | 1.123299 | 0.927374 |
| Logging silent | 0.895185 | 0.804557 | 1.043134 |

The mean hides material losses: spectral_norm is 1.370828/golden39 (six pairs)
and 1.106127/frozen48 (screen); coroutines 1.110488/golden39 (six pairs).
Direct selected/A in `final50-canonical50a` gives logging 1.074052.
Keep these tradeoffs visible; do not claim broad parity or a generally faster
suite. `measure50c-call-pair48` overlapped debug testing and is excluded;
its ADMISSION-EXCLUDED note retains the reason.

`final50-perf-record` has four valid captures, stable identities and zero lost
samples. `final50-perf-stat` has twelve valid 100%-running intervals. Its
normalized summary gives DeltaBlue instructions 1.353105/upstream and cycles
1.129640; logging instructions 0.957476 and cycles 0.799730.
`final50-canonicalA-stat` has twelve valid diagnostic intervals: logging
instructions 1.000000052/A but cycles 1.085878/A; DeltaBlue is approximately
unchanged. Thus the logging loss is not additional dynamic instructions;
its cycle-cost cause remains unproved. Do not infer a layout cause or promise
a padding fix. Native disassembly and retained controls are in
`native50{a,c,c4,final}` and `native50a-control`.

## Source/debug contract

Ordinary dispatch, method nomination and call admission do not decode or compare
source coordinates. The observer copies an explicit semantic handler recipe
into activation-local pending state; the unwinder consumes it once without
reading a source map. Frame-object source ordinals remain attribution only.
Compiler line restrictions and mixed serialized observer records remain a
separate, documented future observer-reconstruction workstream.

The no-position regression creates fresh functions through MAKE_FUNCTION from
stripped factory code. Assigning an existing function's __code__ permanently
disables its call specialization version; the first fixture exposed that
unrelated rule and was corrected without weakening specialization assertions.

## Builds and immutable controls

All paths below are relative to BASE. Magic is 64418; ATTR/poly geometry 48/120
bytes. One GCC 15.2 configuration, CPU 15, hash 0, JIT disabled.

| Role | Source | Build | Commit |
| --- | --- | --- | --- |
| Selected checkpoint | worktrees/execution-boundaries-20261108 | execution-boundaries-{release,debug,stats} | bf810ec1a33 |
| Frozen48 | worktrees/deltablue-call-pair48 | deltablue-call-pair48-release | 5559f60093e |
| Golden39 | worktrees/golden-20261109-runtime | golden-20261109-release | 4d666e1b482 |
| Upstream | worktrees/upstream-3a440b6 | upstream-release-devlibs-3a440b6 | 3a440b65695 |
| Earlier golden | worktrees/pyvalue-rc-universal | pyvalue-rc-repr-{release,debug,stats} | 0765e9be47f |

Frozen46/47 and all historical golden/upstream controls remain unchanged.
The task-created frozen A source/build (`deltablue-canonical50a`) was removed
after all matched comparisons; ELF/config and identity journals are retained.
Use `build-control.py` with A's full commit to reproduce it if needed.

## Resume from this checkpoint

Follow the next lanes in the current plan, not the completed C/leaf queues.
Selected runtime and canonical docs are published on their branches; golden
comparison tags and controls remain immutable. Loom mirrors the current goal,
plan, status, report, tables and IR/interpreter designs under
`http://127.0.0.1:7878/s/xzzlxx17/artifacts/`.

Task-created progress notes, frozen A checkout/build, staged source copies and
generated profile caches are removed. Reviewed proposals, raw results, native
ELFs and `E/source-error50-final.patch` are retained. Do not rerun qualification
or quiet campaigns unless a new change or unresolved failure warrants it.

Host scripts use `UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B`.
Native targets run explicitly through D/run-checks.py. Use unique tags.
E/run-final50-benchmarks.sh records the completed benchmark commands.
The reviewed object-call capture wrapper needs a complete matching all29
upstream screen. Propose → peer review → implement → focused test → post-review
→ integrate; reserve full suites for coherent batches.

## Preserve unrelated work

Never stage/reset/delete the preexisting checkpoint report/log/lock:
`pyjit/bench/results/checkpoint-6a0e4c12c17.md`,
`pyjit/issues/checkpoint-closure/ctypes-rawpin-build.log`,
`pyjit/issues/checkpoint-closure/w5-release.lock`.
Preserve other agents' controls, worktrees and caches. Universal PyValues are
integrated; native method/tracing JIT tiers remain unfinished. Repository docs
own execution state; Loom mirrors the clean goal → plan → status → report/design
chain. Git retains earlier operational snapshots.
