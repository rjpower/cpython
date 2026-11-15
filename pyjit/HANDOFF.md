# Coordinator handoff

Canonical documentation is this checkout; its runtime source is stale. **Do
not build it.** Read [GOAL](GOAL.md), the [active plan](issues/interpreter-performance56/plan.md),
[STATUS](STATUS.md), [architecture](design/runtime-architecture.md) and
[process](PROCESS.md). The current grouped design is
[interpreter-parity61](design/proposals/interpreter-parity61.md), supported by
[review61](issues/register-semantics59/review61.md). The
[trial report](issues/register-semantics59/report.md) owns measured decisions;
this file owns current execution paths.

## Current execution

All five round59 screens, the round60 engine and the round61 producer-only
screen are unselected. Golden remains selected57. PyValue integration is
complete. Do not revive rejected providers, scalar descriptors, continuation
actions, alignment sweeps or a second compiler without a new reviewed consumer.

G is on `pyjit/parity61-common-constant`, at `fe38bad0b50` after TEMP generation.
Both native builds pass 117/79 under tag `parity61-constant2`. The first release
focused run fails one scratch-owner admission test; the other five modules
pass. Baseline selected57 rejects this constructed image while fe38 admits it
(`E59/constant2-vacancy-baseline.json`); no malformed code was executed. The
generator worker is fixing the shared normal-map/admission consumer. Debug and
timing await correction. Source packet `8db82c67bd4` had actual
peer GO and six focused host methods pass. It adds one executing-only TEMP
variant, chosen in the cold executing-copy walk before the no-feedback return.
Public images retain ordinary words; observation may restore the generic form.
No frame/exec field, allocation or per-word lookup is added.

The first producer candidate `8bb861d357eaa8349a0fd89c0ef11df5dd72818d` is retained
on `pyjit/parity61-producer-only`. Both builds completed 117 modules/79 shared
extensions; all 43 focused runtime methods passed per role, and four host source
methods passed. Native handler size fell 620→356 bytes with DUP and normalization
removed. Its stable two-pair screen still lost DeltaBlue 3.62% and logging
13.92%; it is not selected. Native capture `E59/parity61-constant1-native` has
ELF SHA256 `28326a4e7c99114d090d7ea8d849a998fae3b276f3097d03546ec203eab063c8`.
`parity61-constant1-counter-diagnostic` shows fewer whole-process instructions
but more cycles for both losing cases. Its scope differs from kernel-only PMU.
No exclusive cache/layout mechanism is established.

The current build directories are `BASE/reg59-results-{release,debug}`. Root
restored their compiler/core/module flags and byte-identical pyconfig.h to
selected57 using `E59/prepare-parity61-builds.py`; the ledger is
`parity61-build-setup-commands.json`. The old directory name does not identify
its current native revision. Always validate source/native/Lib identity before
execution. The two role builds run serially under the shared root helper.

## Parallel workers

| Worker | Source tree / branch | Current boundary |
| --- | --- | --- |
| Generator | P / `pyjit/parity61-common-constant-temp`, from `8db82c67bd4` | Correct the native-discovered scratch-owner admission regression; creation post-review resumes afterward |
| Frames | W / `pyjit/parity61-created-generator`, from selected57 | Implement direct exact heap-caller creation after root proposal GO; actual source peer before integration |
| Ascent | A / `pyjit/parity61-ascent`, from selected57 | Actual source diff has root peer GO; commit and native integration pending |
| Coordinator | G / `pyjit/parity61-common-constant` | TEMP builds, focused native checks, capture and quiet screen; merge only qualified measured winners |
| Astra | Documentation only | Round61 grouped review complete; next review after the coherent measured cycle if parity remains unmet |

Creation's [proposal](issues/register-semantics59/created-generator-proposal.md)
uses existing EXTERNAL roots, takes scratch inputs before allocation, pins the
bound function/code/arguments, initializes the final heap frame and shares the
return writer. A separate snapshot still moves each argument twice; do not
claim an argument-copy saving. Allocation callbacks can enable origin tracking,
mutate function code or redirect: initial guards alone are insufficient.

Ascent's [proposal](issues/register-semantics59/ascent-proposal.md) distinguishes
cold admitted geometry from real per-hop guards. Preserve all exception,
recursion, profile-anchor, phase and suspended-state commits. The first fast
loop is GIL release without DTrace; debug uses the ordinary fallback. Root must
prove actual multi-hop admission separately from fixture outputs. A is a
new task-owned source worktree; retain its branch/evidence and remove the tree
when this round is finished if no active job needs it.

Workers run focused host source checks only. Root owns full generation, native
builds/tests, profiles and serialized quiet timings. Before each quiet window,
stop worker CPU checks and freeze G/builds/benchmark controllers. Do not mutate
immutable I or any upstream/golden control. No new full-system run is claimed
for an unselected candidate.

## Paths and immutable controls

`BASE=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`;
`R=BASE/worktrees`; G=`R/call55-gc`, P=`R/call55-pins`, W=`R/call55-ir`,
A=`R/parity61-ascent`, I=`R/call55-integrated`.
`E59=BASE/pyjit-link-wave1/register-semantics59`,
`E57=BASE/pyjit-link-wave1/comprehensions57`,
`E55=BASE/pyjit-link-wave1/call-model55`.

| Control | Source commit / tree | Release build | Module record |
| --- | --- | --- | --- |
| Selected57 | `0dcba75bb3df40c32137db453b001f7e8f101de0`, I | `BASE/call55-integrated-release` | `E55/comp57-dictionary-release-build.log`, 117/79 |
| Golden56 | `883524c180e4486b47df77ee2276c9276c0ec32c`, `R/round56-golden` | `BASE/round56-golden-release` | `E57/control-retry-build.log`, 117/79 |
| Upstream | `3a440b65695615e9fd9912342b79cdd54fced0d2`, `R/upstream-3a440b6` | `BASE/upstream-release-devlibs-3a440b6` | `BASE/pyjit-link-wave1/checkpoint-closure/checkpoint-6a0e4c12c17-final/upstream-build.log`, 116/78 |

Keep I's debug build frozen too. Selected57 is branch `pyjit/round57-golden`,
tag `pyjit-golden-20261114-dictionary-readers`; its report/tables and qualification
limits are in [comprehensions57](issues/comprehensions57/report.md). Selected54
`61015a9452d9069646fb5fede9ff963896833b89` and older controls remain untouched.
Round59 branches and round60 `pyjit/parity60-integrated` at `ec230e55641` remain
preserved; exact captures and rejected results are in the trial report. Their
old build paths are not current controls and must not run against a changed G/Lib.

## Native gates and publication

Use `UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B` for host
scripts. Native helper: `pyjit/issues/call-model55/run-checks.py TAG STAGE`,
always with explicit `--source G --build-prefix BASE/reg59-results
--evidence-root E59`. Never use its default candidate tree. Generation is
root-owned; inspect semantic changes and commit products before native build.
Do not rerun host parser/generator source suites under the debug interpreter.

Use the [measurement recipe](issues/register-semantics59/measurement.md) and
balanced eight-case screen after each candidate. A coherent winner advances
to confirmation, all29, direct upstream, PMU and full supported qualification.
Keep six baseline debugger-jump errors explicit; only GDB PrettyPrintTests is
quarantined. GIL-only qualification does not establish free-threaded support.
No parity claim follows from focused tests or static instruction removal.

The active Loom mirrors are `goal`, `plan`, `status`, `performance-report` and
`performance-design`; the latter should point to round61. Seven selected-design/
benchmark mirrors remain unchanged. Push canonical documentation before pinning
relative artifact links to its commit, then use guarded writes and read back.
Origin SSH lacks an agent; the working authorized push is HTTPS with the GH
credential helper. Do not expose credentials or change global git config.
