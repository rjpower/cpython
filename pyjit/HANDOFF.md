# Coordinator handoff

Canonical documentation is this checkout on `weaver/python-jit`. Its C files
are not the measured runtime. Start with [goal](GOAL.md),
[plan](issues/interpreter-round51/plan.md), [status](STATUS.md),
[report](issues/interpreter-round51/report.md) and
[designs](design/runtime-architecture.md).

## Active round, 2026-11-11

BASE=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
D=`pyjit/issues/interpreter-round51`.
E=`BASE/pyjit-link-wave1/interpreter-round51`.

Astra completed one grouped review with five options. Sol implemented each in
an isolated subtree, obtained independent component review, and root compiled
and tested them. All five have clean release identities and passing focused
checks. Two-pair hot-set screens and selective six-pair confirmations are
complete; `E/run-screens51.sh` and `E/run-confirmations51.sh` record commands.
The unchanged hot set is DeltaBlue, Richards, comprehensions, generators,
coroutines and spectral norm, with silent logging as the mandatory guard.

Options 1 (cached layout offset) and 5 (normal generator cleanup) are rejected.
Option 2 (canonical iteration) is held for an incremental comparison. The trial
combination contains only 3 (shared exact-call setup) and 4 (canonical ADD/SUB).
It is not yet promoted. Source:
`BASE/worktrees/round51-golden`, branch `pyjit/round51-golden`,
HEAD `0e5e61f4c6b`. Builds: `BASE/round51-golden-{release,debug}`.
Both check 117 modules. Regeneration leaves the source clean. Independent merged
source review has no blocker; focused release/debug checks and the value API
gate are running. No timing may overlap those jobs. Full diagnostics have not
started for this candidate.

The strict value checker uses the reference release compilation database with
only source/build paths rebased to the identically configured new checkout.
Both candidate native builds are real, separate builds; the database is a
command plan, not a claim of a new compiler-interception run.

## Controls and component identities

All paths below are relative to BASE. Controls are immutable during comparisons.
One GCC 15.2 release configuration, CPU 15, hash 0, JIT disabled; magic 64418,
ATTR/polymorphic geometry 48/120 bytes.

| Role | Source under worktrees/ | Release build | Commit |
| --- | --- | --- | --- |
| Actual prior checkpoint | execution-boundaries-20261108 | execution-boundaries-release | bf810ec1a33 |
| Frozen round control | round51-baseline | round51-baseline-release | bf810ec1a33 |
| Option 1, rejected | round51-option1 | round51-option1-release | 22f4ce1508e |
| Option 2, held | round51-option2 | round51-option2-release | 0d0dbb5fbc9 |
| Option 3 | round51-option3 | round51-option3-release | d1353a16f3d |
| Option 4 | round51-option4 | round51-option4-release | 4bb16138d64 |
| Option 5, rejected | round51-option5 | round51-option5-release | e9886d5b1c3 |
| Golden39 | golden-20261109-runtime | golden-20261109-release | 4d666e1b482 |
| Upstream | upstream-3a440b6 | upstream-release-devlibs-3a440b6 | 3a440b65695 |

The earlier golden, frozen46/47/48 and all preexisting worktrees remain intact.
Root will remove task-created experiment controls/worktrees after retaining
necessary evidence. The final selected source/build stays available.

## Qualification and next gate

`D/run-checks.py` accepts explicit source/build paths, unique tags and optional
regrtest match globs. It quarantines only
`test.test_gdb.test_pretty_print.PrettyPrintTests.*`; the opt-in flag restores
those diagnostics. The other four GDB groups passed the filtered control check.
Debugger-jump and LLTrace checks remain enabled with their established failures;
they are not quarantined or silently counted as passes.

Next: finish merged focus, inspect direct combined hot-set results and decide
whether option 2 warrants an incremental test. Final admission requires direct
comparisons with the actual prior image and upstream, all 29 kernels, applicable
release/debug full diagnostics and independent review. Never multiply separate
campaign ratios. Keep DeltaBlue/generator losses visible. No further Astra
inner-loop call or rejected-option rescue queue is planned.

Host scripts use `UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B`.
Root owns builds, tests, timing, integration, commits and publication. Workers
leave changes for review. All project state is in canonical docs; Loom mirrors
it under `http://127.0.0.1:7878/s/xzzlxx17/artifacts/`.

## Preserve unrelated work

Never stage/reset/delete the preexisting checkpoint report/log/lock:
`pyjit/bench/results/checkpoint-6a0e4c12c17.md`,
`pyjit/issues/checkpoint-closure/ctypes-rawpin-build.log`,
`pyjit/issues/checkpoint-closure/w5-release.lock`.
Preserve other tasks' source, controls, caches and artifacts. Universal PyValues
are integrated; native method/tracing JIT tiers remain unfinished.
