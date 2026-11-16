# Coordinator handoff

Canonical documentation is this checkout; its runtime source is stale. **Do
not build it.** Start with [GOAL](GOAL.md), the
[active plan](issues/interpreter-performance56/plan.md), [STATUS](STATUS.md),
[architecture](design/runtime-architecture.md) and [process](PROCESS.md).
The latest closed proposal is [parity69](design/proposals/interpreter-parity69.md),
under [review69](issues/register-semantics59/review69.md). The
[trial report](issues/register-semantics59/report.md) owns measured outcomes;
this file owns execution logistics.

## Current work

Round69 is stopped and unselected. Astra's grouped70 commit
`3067f7181b6fd237f77b12925bbb76d5d8dd62aa` is integrated as `07941d2b663`.
It owns only [proposal70](design/proposals/interpreter-parity70.md) and
[review70](issues/register-semantics59/review70.md). The proposal changes ordinary
CALL to A=B scratch plus explicit caller consumption; there is no legacy-word
translator. Implementation is not released.

Round70 is now stopped before implementation. The exact source proposal is
`E59/round70-unit-proposal.md`; final independent peer is
`E59/round70-unit-proposal-peer.md`. A typed continuation resolves implicit-child
routing, but adds work without a credible complete-consumer saving. No runtime
edits, builds or timings were performed. The trial report owns exact hashes and
the cost argument.

Grouped review71 is commissioned in Astra channel `zfr7ow1e`: only proposal71
and review71, no runtime work or agents. Sol-high producer supplies
`E59/round71-execution-architecture-input.md`; consumer supplies
`E59/round71-value-architecture-input.md`, both source/native readback only.
Root owns the final adoption and any implementation release. Selected57 stays
immutable/golden; preserve round69 branches, controllers and evidence.

## Closed round69 checkpoint

G is clean `52806aacedf5be0a89546344162bb53a8e40ec34`, branch
`pyjit/parity69-guarded-summary-integrated`. P is frozen producer
`b2780170943db23e16b97de2f40e084cc12ea3cc`; W is frozen consumer
`8a9dae105c2939f0db1f3b58664f6bcbfc903c68`. Reused build prefixes are
`BASE/reg59-results-{release,debug}`. Both `parity69-tests6` builds pass 117/79;
29 focused, 53 related and 350 observation/code/marshal methods pass per role
with 1+9 skips. Full generation/products and independent source/product reviews
pass; all 331 existing format rows remain unchanged.

Full behavioral qualification remains incomplete. The corrected finalizer
fixture records the actual external-ancestor rejection/supported outcome,
without changing debugger authority. Existing positive redirect tests run
ordinary fallback under tracing; they do not prove committed-summary
PENDING/APPLIED/same-PC/service-error paths. Root records this limit before
an early rejection screen in `E59/guarded-summary69-tests6-screen-admission.json`.

Actual projection/select native controls have no child execution, correct
results and consumed owners, but complete release instruction counts grow
395→437 and 537→612; reached stack 80→152/264B. Original stamp5 and current
stamp6 receipts are separately pinned in `E59/guarded-summary69-native-controls`.
Natural unchanged Delta has one real success and 23 static sites; comp has 0
selected sites. Receipts are `E59/guarded-summary69-natural/*-tests6-release-v2.native.json`.
The first natural launch's fixture preflight failure is retained; no kernel
entry or hit was claimed for it.

The stable 32-row `E59/summary69-hot8-selected57-screen` rejects the candidate:
GM 1.012667, Delta 1.034596, logging 1.176093, generators 1.018079,
raytrace 1.034177; coro 0.957014 and spectral 0.945332 improve. Root stops all
remaining qualification, source repair, confirmation/all29/PMU/full/timing
rescue. This was explicitly an early diagnostic, not complete behavioral GO.
No fresh upstream result or cause is claimed. The [trial report](issues/register-semantics59/report.md)
owns full outcomes, failures, exact receipts and historical source corrections.

Workers perform only the bounded review71 inputs above. Astra owns
broad grouped review only, with no target execution or inner-loop work. C's
runtime source remains stale; I/U remain immutable. Preserved68 binaries and
older captures are static evidence only after reuse of G/Lib/builds.

## Preserved round68 checkpoint

Rejected round68 is preserved on `pyjit/parity68-retained-delegation-integrated`
at `c02cebf80c48d47a9167dd86dce9bdc0fd11a623`. Its captured native files and
`E59/retained-delegation68-*` evidence remain sealed. G/Lib/builds have moved on;
never execute its captured ELF against the current Lib. The
[trial report](issues/register-semantics59/report.md) owns its complete
qualification and rejection; the
[contract](issues/register-semantics59/retained-delegation68.md) owns its protocol.

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
[design69 selection rules](design/proposals/interpreter-parity69.md#ordered-implementation-and-disposition).

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
