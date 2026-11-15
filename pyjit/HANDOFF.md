# Coordinator handoff

Canonical documentation is this checkout; its runtime source is stale. **Do
not build it.** Start with [GOAL](GOAL.md), the
[active plan](issues/interpreter-performance56/plan.md), [STATUS](STATUS.md),
[architecture](design/runtime-architecture.md) and [process](PROCESS.md).
Current proposal is [parity68](design/proposals/interpreter-parity68.md),
under [review68](issues/register-semantics59/review68.md). The
[trial report](issues/register-semantics59/report.md) owns measured outcomes;
this file owns execution logistics.

## Current work

Round67 is closed: both candidates qualified their mechanisms and lost their
screens. Selected57 is unchanged. Round68 has one unit: retain real owning
generator-delegation prefixes, enter the actual leaf directly and update only
the changed suffix. The grouped architecture and exact
[implementation contract](issues/register-semantics59/retained-delegation68.md)
have independent approval. No second unit is active.

G integrates the complete retained-entry, private native-resume, yield,
lifecycle and reader source at `bf86c69dca817739d03ffe2bd6ecc53a8572bb96`.
Test-only corrections advance G to `27521e23fab`; both tests14 builds pass117/79.
Independent source and generated-product review pass. Active13b release and
debug builds each pass 117 modules/79 shared extensions; all 26 focused methods
pass per role, with one debug-only skip in release. Counts are 16 delegation,
seven lifecycle and three remote methods. The stopped-reader method contains
27 cases, four valid and 23 refusals; do not count those as methods.

The first generated product was rejected for indentation drift and corrected
before compilation. Exactly seven of 329 concrete handlers change; the other
322, prefix and labels remain byte-identical. Register-IR metadata is unchanged.
The conservative PC effect removes one unsupported legacy YIELD_VALUE expansion
row; no supported tier2 qualification is claimed.

Debug native mechanism controls at depths2/8/32 and release depth8 observe39
retained entries/yields each, with zero intermediate delegate dispatch,
materialization or allocation while armed. Six actual direct-backend NULL
faults preserve behavior. VM SEND/FOR positive controls pass. Actual Tree prefix
and recursion-boundary qualification remains in progress. There is no performance
result or selection. Selected57 is unchanged.

| Owner | Current boundary |
| --- | --- |
| Sol-high source | W, `pyjit/parity68-retained-delegation`; complete source frozen, actual ELF structure/cost readback |
| Sol-high remote | P, `pyjit/parity68-delegation-remote-readers`; source/product approval, independent qualification coverage review |
| Root | G, `pyjit/parity68-retained-delegation-integrated`, `27521e23fab`; tests14 builds/focus pass, serialized native controls |
| Astra | Grouped review68 `183447221537190a4ac1003026f554dbaf428a3d`, integrated in C `29a016e656d`; no inner-loop work |

Build/focus manifests use `E59/parity68-active13b-*` and `parity68-tests14-*`.
Broader integration logs retain the six baseline trace errors, initial nonexistent
module selection and corrected parser/remote test failures. Thirteen related
modules pass per role; the ten affected tests pass both roles after correction.
The relocated source-parser method passes root's host execution. Source changes
since active13b are test-only; native captures retain their actual earlier ELF
identity rather than being relabeled.
Root native controls, receipts and exact commands live in
`E59/retained-delegation68-native-controls`. Preserve earlier failed compile,
fixture and generated-product attempts under their original tags. Corrected
closure9c's 17-method inert coverage overlaps this active checkpoint; do not
add counts. Seven lifecycle methods also pass immutable selected57 and upstream
release baselines through their own libraries.

Source receipts live in `E59/retained-delegation68-{source,frame,remote}-host`;
independent cross-review is in `E59/retained-delegation68-closure-peer`.
The [trial report](issues/register-semantics59/report.md) owns earlier increment
results rather than duplicating them here. Exact publication ordering and
count1 native setter/hook order are in the contract body. Materialization is
nonallocating and noncalling; successful entry publishes None only after real
links and debt are committed. Nonblocking remote readers refuse membership
without dereferencing it; blocking readers validate a fresh committed snapshot
under the existing all-thread stop. Ordinary THREAD paths gain no semantic
checks. No source-only checkpoint is eligible for selection.

Rejected B67 integration remains preserved on
`pyjit/parity67-anchored-self-integrated` at `65fa10014885d139a47099ed384aefe59d26b943`.
The [implementation contract](issues/register-semantics59/retained-delegation68.md)
and [source peer](issues/register-semantics59/retained-delegation68-peer.md)
integrate frozen contract `836d0a93bc6406392153f7c7233fc1f197dea703c424cf1b25d4344ca594a30a`
and its independently approved ENTERING noncalling addendum.
Materialization never calls the recipient setter: a successful entry continuation
publishes None exactly once after valid links/debt; early C-entry failure clears
without publishing unused input. No extra universal frame field is introduced.

## Closed experiments

Rounds59–67 are closed and unselected. The [trial report](issues/register-semantics59/report.md)
owns their exact sources, native captures, failed checks, qualified mechanisms
and rejected screens. Do not restart a rejected packet or treat historical
qualification as current build identity.

Preserved round67 branches include `pyjit/parity67-native-callout`,
`pyjit/parity67-callout-integrated`, `pyjit/parity67-anchored-self` and
`pyjit/parity67-anchored-self-integrated`. Earlier branches and sealed evidence
remain intact. Runtime worktrees P/W/G have moved on; their old captured
standalone ELFs are static evidence only. **Never execute a capture against a
changed Lib tree.** Reused builds become stale after source changes until both
are refreshed. No candidate build is another immutable control.

Only the complete current unit advances to native mechanism/ABI evidence and
one unchanged hot8 screen. A losing screen stops without confirmation, all29,
PMU, full suite or repair. A promising coherent winner advances under the
[design68 selection rules](design/proposals/interpreter-parity68.md#ordered-handoff-and-selection).

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
tag `pyjit-golden-20261114-dictionary-readers`. Selected54
`61015a9452d9069646fb5fede9ff963896833b89` and other controls remain untouched.
The selected report/tables own current upstream comparisons and limits.

## Native gates and publication

Use `UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B` for host
scripts. Root helper: `pyjit/issues/call-model55/run-checks.py TAG STAGE`,
with explicit `--source G --build-prefix BASE/reg59-results --evidence-root E59`.
Its default candidate is immutable I: **never rely on that default**. Root owns
full generation, generated diff review/commit, then builds. Do not run host
parser/generator source suites under the debug interpreter.

Both reused builds retain the selected compiler/core/module configuration from
`E59/prepare-parity61-builds.py` and `parity61-build-setup-commands.json`:
matched module/core flags, byte-identical pyconfig.h, NCSR, no backend
or CFLAGS_CEVAL override. Latest both 117/79 passes establish that setup. Keep
G/builds/controllers frozen during native probes and balanced quiet windows;
workers pause CPU checks, but may edit their own source.

Use the [measurement recipe](issues/register-semantics59/measurement.md).
The default case list is logging only: pass hot8/all29 cases explicitly.
Qualification is source peer → root generation/build/focused tests → native
mechanism/actual hits → balanced hot8. A winner advances to confirmation,
explicit all29, direct upstream, matched PMU and full supported diagnostics.
Do not broaden testing or repeat timings for a loser. GIL-only results do not
qualify FT. Only GDB PrettyPrintTests is quarantined; six baseline debugger-jump
errors stay explicit at the full gate.

The active Loom mirrors are `goal`, `plan`, `status`, `performance-report` and
`performance-design`; the last points to the current grouped proposal after
publication. Seven selected design/benchmark mirrors stay unchanged until a
winner changes their contracts. Push canonical documentation before linking
artifacts to its commit; use guarded writes and exact readback. Origin SSH
lacks an agent; use the authorized HTTPS GH credential helper without exposing
credentials or changing global config.
