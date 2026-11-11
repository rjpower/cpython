# Coordinator handoff

Canonical documentation is this checkout on `weaver/python-jit`. Its C files
are not the measured runtime. Start with [goal](GOAL.md),
[plan](issues/interpreter-round51/plan.md), [status](STATUS.md),
[report](issues/interpreter-round51/report.md) and
[designs](design/runtime-architecture.md).

## Runtime and evidence

BASE=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
D=`pyjit/issues/interpreter-round51`.
E=`BASE/pyjit-link-wave1/interpreter-round51`.

The selected round-51 source is `BASE/worktrees/round51-golden`, branch
`pyjit/round51-golden`, commit
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`.
It is pushed to origin with tag `pyjit-golden-20261111-runtime`.
Builds are `BASE/round51-golden-{release,debug}`. Both check 117 modules,
regeneration leaves source clean, focused checks pass 821 tests with nine skips
plus five generator tests in each role, and value API checks pass 306 units.
No stats-layout change is claimed. The value checker uses a reference release
compilation database with paths rebased to the identically configured new
checkout; both native candidate builds are real, separate compilations.

Astra gave one grouped review of five options. Independent Sol proposals,
reviews and subtree implementations are complete. Only options 3 (shared exact
call setup) and 4 (canonical ADD/SUB publication) are selected. Options 1 and 5
failed measured breadth or regression gates; adding option 2 to 3+4 regressed
multiple cases despite a comprehensions gain. Do not revive rejected variants
or call Astra in the inner loop.

## Controls

All paths below are relative to BASE. Controls are immutable. One GCC 15.2
release configuration, CPU15, hash0, JIT disabled; magic64418,
ATTR/polymorphic geometry48/120 bytes.

| Role | Source under worktrees/ | Release build | Commit |
| --- | --- | --- | --- |
| Prior checkpoint50 | execution-boundaries-20261108 | execution-boundaries-release | bf810ec1a33 |
| Golden39 | golden-20261109-runtime | golden-20261109-release | 4d666e1b482 |
| Upstream | upstream-3a440b6 | upstream-release-devlibs-3a440b6 | 3a440b65695 |

The earlier golden, frozen46/47/48 and all preexisting worktrees remain intact.
Task-created option/control/trial worktrees and builds were removed. Local
branch commits and `E/retired-experiments` retain patches, native binaries,
configurations and the removal manifest. Do not confuse a removed experiment directory with selected source.

## Completed qualification and next work

All29 two-pair campaigns against upstream, actual prior50 and golden39 are
complete with stable source/native identities. Their elapsed geometric means
are 1.156535, 0.985700 and 0.982829 respectively. Six-pair direct target
comparisons and the 2+3+4 increment are complete. Keep DeltaBlue/generator
regressions and logging pair spread visible; do not multiply campaign ratios.
Fresh post-warmup PMU records 24 valid counter intervals and four zero-loss
sampled profiles. `final51-perf-stat` is an excluded pre-interval failure from
an old hardcoded extension count; `final51-perf-stat-clean` is the accepted
campaign. New clean builds have 79 current-ABI shared extensions; upstream has
78. The independently reviewed wrapper checks exact counts and hashes every
extension before/after, retaining all prior module/config/native gates.

Full diagnostics and deepcopy_memo confirmation are complete. Both builds run
54,689 filtered tests; release retains nine debugger-jump errors, debug those
nine plus one LLTrace failure. No additional module fails. The other four GDB
groups pass. Exact current case IDs are retained in verbose logs and independent
review; prior50 quiet logs establish only groups/counts. Independent final
review approves this incremental checkpoint with its explicit performance losses
and diagnostic limits. No native job or runtime edit remains pending.
Timing/profiling finished before full tests; never overlap quiet campaigns with
builds, tests or bulk cleanup.

`D/run-checks.py` quarantines only
`test.test_gdb.test_pretty_print.PrettyPrintTests.*`; the explicit opt-in restores
it. Other four GDB groups passed the filtered control check and remain enabled.
Debugger-jump and LLTrace retain their established failures; report actual
remaining failures rather than calling filtered full diagnostics green.

Next round first attributes selected-image generator creation/resume/yield/
completion costs. Constant operands need a weighted hidden-home cost census
before proposal. Feedback already uses a directly addressed scaled byte offset,
not an entry-pointer table. The source assessment records rejected premises.

Host scripts use `UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B`.
Root owns builds, tests, timing, integration, commits and publication. Workers
leave changes for review. Loom mirrors canonical docs under
`http://127.0.0.1:7878/s/xzzlxx17/artifacts/`.

## Preserve unrelated work

Never stage/reset/delete the preexisting checkpoint report/log/lock:
`pyjit/bench/results/checkpoint-6a0e4c12c17.md`,
`pyjit/issues/checkpoint-closure/ctypes-rawpin-build.log`,
`pyjit/issues/checkpoint-closure/w5-release.lock`.
Preserve other tasks' source, controls, caches and artifacts. Universal PyValues
are integrated; native method/tracing JIT tiers remain unfinished.
