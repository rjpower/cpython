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

G is on `pyjit/parity61-created-integrated`, at `f8ecd806cdb` after generation.
Its source packet is `b10667203fe`: root and independent actual source GO,
three host source checks pass. Two new generated learned-only CALL handlers
use the existing-size feedback payload; ordinary handler bodies are unchanged.
Both native builds pass 117/79 under tag `parity61-created1`. First release
new-fixture module runs ten methods and fails three assertions: two expect
creator nomination after code/default mutation and one expects immediate
weakref death during a displaced-local callback. Frame worker owns serialized
read-only native diagnosis; root holds further native jobs. No debug or timing
result yet. The former constant composition is preserved on
`pyjit/parity61-common-constant` at `53e9a57cfa2` and is rejected.

TEMP source `8db82c67bd4` added one executing-only scratch variant. Native
checks found a constructor rejection regression; root's comparison is
`E59/constant2-vacancy-baseline.json` and no malformed code was executed.
Cold repair `65c99464e0b` preserves known-owner conflict diagnostics through
selected normal replay. Both corrected builds pass 117/79 and all 161 methods
across seven focused modules pass per role, including valid SAME_VALUE stores.
The real TEMP handler is 69 bytes with no ownership/normalization/destination
checks, but logging loses 14.4% and spectral_norm 5.8% in both screen pairs.
Other wide pair ranges are not reliable gains. Evidence is
`E59/parity61-constant3-{native,eight-selected57}`. No all29/full/upstream
campaign follows this loser; creation/ascent do not depend on it.

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

Ascent integrated `410bea36190` is preserved on
`pyjit/parity61-ascent-integrated`. Both 117/79 builds and 397 focused-method
coverage per role pass after one targeted release fixture correction. GDB
histograms prove actual eight-frame and bounded 16 composition, profiling
fallback and custom-iterator split. The native handler grows 556→1,076 bytes
with operand spills. Screen rejects generators +7.9% despite coroutines −4.8%.
Evidence: `E59/parity61-ascent2-{native,eight-selected57}` and
`parity61-ascent-diagnostic`; optional reject-site counters are invalid as
rejection evidence, while independent hop histograms pass. Do not compose it
as a selected dependency or run it against G's changed Lib.

The current build directories are `BASE/reg59-results-{release,debug}`. Root
restored their compiler/core/module flags and byte-identical pyconfig.h to
selected57 using `E59/prepare-parity61-builds.py`; the ledger is
`parity61-build-setup-commands.json`. The old directory name does not identify
its current native revision. Always validate source/native/Lib identity before
execution. The two role builds run serially under the shared root helper.

## Parallel workers

| Worker | Source tree / branch | Current boundary |
| --- | --- | --- |
| Generator | P / `pyjit/parity61-common-constant-temp`, at `65c99464e0b` | TEMP correction qualified but composition loses; independent creation post-review |
| Frames | W / `pyjit/parity61-created-generator`, at `b10667203fe` | Frozen source actual peer GO; native gates root-owned |
| Ascent | A / `pyjit/parity61-ascent`, at `53df1c14deb` | Native qualification/hop diagnostics complete; screen loses generators, preserve source |
| Coordinator | G / `pyjit/parity61-created-integrated` | Creation build/focused qualification, native capture and quiet screen; select measured winners |
| Astra | New round62 documents only | Broad review underway with rejected61 native evidence; freeze after creation outcome |

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
