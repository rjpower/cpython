# Coordinator handoff

Canonical documentation is this checkout; its runtime source is stale. **Do
not build it.** Start with [GOAL](GOAL.md), the
[active plan](issues/interpreter-performance56/plan.md), [STATUS](STATUS.md),
[architecture](design/runtime-architecture.md) and [process](PROCESS.md).
The current adopted proposal is [parity71](design/proposals/interpreter-parity71.md),
under [review71](issues/register-semantics59/review71.md). The
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

Grouped review71 from Astra channel `zfr7ow1e`, commit `4ee0ab177f9`, is
integrated as `2c186e0fb6f` and adopted for exact source review. The two serial,
independently selectable units are canonical fact transport (A), then generated
hot/body-only terminal slow lowering (B). A need not win for B to proceed; both
are selected57-relative. Root has released A implementation only.

Source proposals are `E59/round71-canonical-source-proposal.md` and
`E59/round71-split-source-proposal.md`. Opposite Sol-high workers independently
review them into `round71-canonical-source-peer.md` and
`round71-split-source-peer.md`. Both exact contracts now have independent GO;
the trial report owns hashes and resolved findings. A's worker creates branch
`pyjit/parity71-canonical-provenance` in W from immutable selected57. Source/tests
only: root owns full generation, products, builds and target execution. A froze
as W `d33f7b2d9bd`; independent source post-review gives GO. Root integrates it
as G `2cc957b53c8`, generates successfully under `parity71-canonical1`, reviews
the sole product change and commits it as `ff1618987bd`. Exactly five of 329
handlers change; all metadata and other products remain unchanged. Independent
product review gives GO in `E59/round71-canonical-products-peer.md`. Both
`parity71-canonical1` builds pass 117/79. The `parity71-canonical1-runtime` focus
passes all 24 methods per role: CanonicalProvenanceRuntimeTests, PyValueTests
and OperandModeTests. The host source class is excluded. Static native review
confirms eight Canonicalize edges removed; the early protected screen then
rejects A with GM 1.011186 and logging 1.121887. All 32 rows succeed with stable
identity. Complete dynamic native paths and actual HINT coverage remain
incomplete and are stopped. Both prepared native-control packages remain
unexecuted. No confirmation/all29/PMU/full or A repair follows.

B implementation is released in P, branch `pyjit/parity71-terminal-hot-cold`
from immutable selected57. It covers standalone ADD/SUB only, without A's reader
facts or executable. The source contract explicitly covers all hot
close sites, canonical arithmetic, once-only stats and the borrowed-reader cut.
The opt-in one-pair confirmation capability in the canonical host measurement
helper has passed root proposal/diff review and three host test methods; see
the measurement recipe. It has not launched a confirmation. The two
`round71-*-architecture-input.md` memos are static source/native input only.
Root owns implementation release, runtime jobs and integration. Selected57 stays
immutable/golden; preserve round69 branches, controllers and evidence.

## Closed round69 checkpoint

Round69 is preserved at `52806aacedf5be0a89546344162bb53a8e40ec34`, branch
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

The B worker implements source/tests in P; the A worker independently reviews
its frozen diff and host receipts. G remains frozen rejected A until root B
integration. Preserve A/69 branches and sealed evidence.
Astra's broad review is complete; it stays out of the component loop. C's
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
[design71 selection rules](design/proposals/interpreter-parity71.md#ordered-qualification-and-selection).

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
