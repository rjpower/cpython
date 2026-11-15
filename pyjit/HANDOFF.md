# Coordinator handoff

Canonical documentation is this checkout; its runtime source is stale. **Do
not build it.** Read [GOAL](GOAL.md), the [active plan](issues/interpreter-performance56/plan.md),
[STATUS](STATUS.md), [architecture](design/runtime-architecture.md) and
[process](PROCESS.md). The current grouped design is
[interpreter-parity63](design/proposals/interpreter-parity63.md), supported by
[review63](issues/register-semantics59/review63.md). The
[trial report](issues/register-semantics59/report.md) owns measured decisions;
this file owns current execution paths.

## Current execution

G is on `pyjit/parity62-method-integrated`, at `ab4cb5f49fb`, after root and
independent review of W packet `a67bdfc0b1d`. Generation required only the
registry's removed-reservation spelling for unused IDs 370–374, corrected in
`2e7c277dde5`. Generated products are committed at `665fee2062d`; a subsequent
fixture-only correction builds synthetic images with fresh source maps and
checks cold non-nomination rather than a rejection absent on selected57.
Matched candidate/selected release and debug probes are retained under
`E59/method-fixture-diagnostic`. The malformed images are never executed.

Both method3 builds complete 117/79. All 227 focused methods pass per role:
new eight under method3, other 219 release under method2 and debug under
`parity62-method3-debug-protocol`. Native capture and independent review pass
at `E59/parity62-method3-native`, ELF SHA256
`a39e077632782a8d7538fa7134305b241f63ef150691db58c53d01bbba556624`.
Actual coverage is retained in `E59/method-coverage`; kernel first-hit probes
confirm native entry on Delta and ray. The stable hot-eight screen rejects:
Delta 1.032009 and logging 1.228535 despite comp .965035. No all29/upstream/full
campaign follows. All four round62 candidates are rejected.

No native job currently runs. G/builds still hold this rejected method trial;
do not mistake them for golden. The next reviewed cycle begins from selected57,
with P's fixed-role unit and W's deferred-await design. Preserve all old branches
and captures; their standalone ELFs must not execute against G's changed Lib.

All five round59 screens, the round60 engine and every round61 runtime packet
are unselected. Golden remains selected57. PyValue integration is complete.
Do not revive rejected providers, scalar descriptors, continuation actions,
alignment sweeps or a second compiler without a new reviewed consumer.

Prepared source is preserved on `pyjit/parity62-prepared-integrated`, at `a3cbad0637e`, after root
generation from source `49105087f2b` and two fixture corrections, directly on
selected57. Both builds complete 117/79 under `parity62-prepared4`; all 227
focused methods pass per role across the prepared module and nine protocol/
observation modules. The synthetic cursor retirement abort also reproduced
on selected57; the fixture now uses its complete POP_ITER protocol. Native
capture `parity62-prepared4-native` has ELF SHA256
`017551836bf38f5a614d19d91d0d47294174989847c55c69060f5a212e44a619`.
Independent native review passes. Actual probes in `E59/prepared-coverage`
show three active comprehension sites and 41 DeltaBlue sites, with bounded
kernel first hits. Screen `parity62-prepared4-eight-selected57` rejects the
packet: comprehensions .948255 and coroutines .964114 improve, but logging
1.182918 and DeltaBlue 1.021921 lose in both pairs. Probe and quiet windows
are released. The short whole-process branch diagnostic has logging
instructions 1.000317/cycles 1.147451, with flat branches/slightly fewer misses;
no exclusive cause is established. The fetch diagnostic is exploratory with
83% multiplexed counters. No more native jobs are running.

Delegated resume is preserved on `pyjit/parity62-resume-integrated`, at
`17139aa0166`. Both corrected builds complete 117/79 and all 397 focused
methods pass per role. The original failures were a LINE expectation also
failing on selected57 and an overbroad optional source-analysis rejection;
exact matched probes are `E59/resume62-exact-observation-*`. Native capture
`parity62-resume2-native` has ELF SHA256
`ddb5b6dd027091a1863575182a5ca020900037669c943feaab13baf0da66cacb`.
The screen improves generators 4.3% but loses DeltaBlue 6.5%, logging 20.3%
and spectral_norm 9.3% in both pairs. Root rejects the standalone packet.
Do not execute its old build against G's changed Lib.

Delegated resume uses private ID 375. Prepared CALL reserves IDs 370–374;
method CALL reserves 376–378 and creation uses 368/369. Reserved gaps are not
executable or publicly admitted, and do not imply a source dependency.

Allocation62 is preserved on `pyjit/parity62-allocation-integrated`, at
`15edce610d09ffcc37f7b7ed3465852101397b49`. Both builds completed 117/79 and
526 focused methods passed per role. Six-pair confirmation shows coroutines
.955934, but logging 1.033865 and spectral 1.049700. Explicit all29 has stable
identity, 116 rows, mean 1.004907 and 12 wins/29; deepcopy_memo also loses 7.9%.
Root rejects the standalone candidate. Evidence is
`E59/parity62-allocation1-{confirm,eight}-selected57`,
`parity62-allocation2-all29-selected57` and capture/review
`parity62-allocation1` (ELF SHA256
`c51546d9df97d8397d8e511eae4d77f5ce5afd2f18498b0db217cef6ae879685`).
The misnamed `parity62-allocation1-all29-selected57` contains only logging
because --cases was omitted; it is not broad evidence. Do not execute its
old build against G's changed Lib.

Staged creation61 is retained at `4ac529577e2`, on
`pyjit/parity61-created-integrated`: both builds 117/79, 525 focused methods
per role, screen loses DeltaBlue 8.9% and logging 30.2%. The short whole-process
PMU diagnostic finds near-identical instructions but more cycles. Ordinary
GENERAL initializer scheduling changes are concrete but do not establish the
cause. Keep `created-generator-diagnostic/native-review.md` and
`parity61-created4-counter-diagnostic`. No full/upstream campaign follows.

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
| Generator | P / `pyjit/parity63-role-lowering`, from selected57 | Root/peer-approved list/FOR and append implementation; prepared branch preserved |
| Frames | W, preserved method branch `pyjit/parity62-method-call` at `882309dd897` | Deferred-await63 unit design and independent role peer; new source branch starts from selected57 |
| Continuation | A / `pyjit/parity62-delegated-resume`, at `d43f88d0402` | Frozen rejected source and evidence |
| Coordinator | G / `pyjit/parity62-method-integrated`, at `ab4cb5f49fb` | Rejected method trial frozen; next source integration starts from selected57 |
| Astra | Final grouped63 architecture review | Complete; root and Sol peers own component loop |

The rejected creation [proposal](issues/register-semantics59/created-generator-proposal.md)
uses input EXTERNAL roots and moves each argument twice. The
[allocation62 proposal](issues/register-semantics59/creation-allocation62.md)
removes that staging: supported allocator/reftracer hooks cannot execute Python,
and GC allocation schedules a later collection. Existing owning CALL homes
therefore retain inputs until one final transfer. Publication/redirect and
unknown-owner release still require result roots and reached callback state.

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
`performance-design`; the latter points to round63 after publication. Seven selected-design/
benchmark mirrors remain unchanged. Push canonical documentation before pinning
relative artifact links to its commit, then use guarded writes and read back.
Origin SSH lacks an agent; the working authorized push is HTTPS with the GH
credential helper. Do not expose credentials or change global git config.
