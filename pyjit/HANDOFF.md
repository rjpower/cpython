# Coordinator handoff

Canonical documentation is this checkout; its runtime source is stale. **Do
not build it.** Start with [GOAL](GOAL.md), the
[active plan](issues/interpreter-performance56/plan.md), [STATUS](STATUS.md),
[architecture](design/runtime-architecture.md) and [process](PROCESS.md).
The selected runtime is golden85, the [hot85](issues/register-semantics59/hot85.md)
integration at `2a9ea8c4f69`, selected by the user on 2026-11-22. Golden84
(`4be2eb5d31f`, [parity84](issues/register-semantics59/parity84.md)) and
golden82 (`5194e957c5d`, [gap82](issues/register-semantics59/gap82.md)) are
earlier goldens and frozen controls. The earlier
selection, [execution-boundary73](design/proposals/execution-boundary73.md),
was selected as golden73 on 2026-11-16 with the logging regression accepted. The
[trial report](issues/register-semantics59/report.md) owns measured outcomes;
this file owns execution logistics.

## Completed checkpoint and retained worktrees

Boundary73 and its bounded return-isolation correction have completed source
review, integration, generation, protocol/native qualification and separate
protected screens. Final `44f066f5fd5` is golden73; earlier boundary10 stays
unselected. The user accepted the final logging tradeoff, not a general change
to future protection rules. Preserve both original screen verdicts.

| Tree | Branch | Current responsibility |
| --- | --- | --- |
| G | `pyjit/execution73-boundary-integrated` | Frozen golden73 `44f066f5fd55947edf709bbcf9bf17c607307e4b`; both boundary11 builds pass 117/79 |
| P | `pyjit/execution73-return-isolation` | Clean source `557c225602c`; all source integrated |
| W | `pyjit/execution73-export-contract-fix` | Clean `526157c52ab`; prior compiler source and peers complete |
| `worktrees/call55-ir` | `pyjit/handlers75` | Handlers75 round 1 `91c9baa9600`; builds `BASE/handlers75-{release,debug}`; keep clean while it is a measurement control |
| `worktrees/handlers75-r2` | `pyjit/handlers75-r2` | Round 2 word flags `f78c3902c1b`, from round 1; builds `BASE/handlers75r2-{release,debug}`; keep clean while it is a measurement control |
| `worktrees/calls76` | `pyjit/calls76` | Round 3 call path `ba2d7938541` (pushed), from round 2; builds `BASE/calls76-{release,debug}`; qualified (`E59/calls76-*-focus`); keep clean while it is a measurement control |
| `worktrees/values76` | `pyjit/values76` | Value rules 1–3, first pass, `0d18fff7a07`, from round 2; builds `BASE/values76-{release,debug}`; `pyjit/values76-draft` holds a stopped run's unverified edits |
| `worktrees/checkpoint76` | `pyjit/checkpoint76` | Round 3 + values76, frame-chain and self-test fixes, `013fb42c922` (pushed); builds `BASE/checkpoint76-{release,debug}`; [record](issues/register-semantics59/checkpoint76.md); measurement control, keep clean. Variant builds `BASE/{calls76fp,calls76fpfull,checkpoint76nofp}-release` are attribution copies only |
| `worktrees/values77` | `pyjit/values77` | Remaining rule 1–3 rows and the borrowing native scope, `35b4666037f`, from `76fc2b68918`; builds `BASE/values77-{release,debug}`; merged into checkpoint77 |
| `worktrees/slots77` | `pyjit/slots77` | Complete value-slot table, `d9054a5b0e4`, from `76fc2b68918` (`E59/slots77-notes.md`); merged and reverted ([record](issues/register-semantics59/values77.md)); builds `BASE/slots77-{release,debug}` |
| `worktrees/checkpoint77` | `pyjit/checkpoint77` | checkpoint76 + values77, `3c8abb64776` (pushed; `feb572b0541` merged and `3c8abb64776` reverts slots77); builds `BASE/checkpoint77-{release,debug}`; [record](issues/register-semantics59/checkpoint77.md); measurement control, keep clean |
| `worktrees/slots78` | `pyjit/slots78` | Paired value fields and cold out-of-line adapters, `99a04733128` (pushed), from `3c8abb64776`; builds `BASE/slots78-{release,debug}` predate the frame-chain flag (attribution only) |
| `worktrees/checkpoint78` | `pyjit/checkpoint78` | checkpoint77 + slots78, `99a04733128` (pushed); builds `BASE/checkpoint78-{release,debug}`; [record](issues/register-semantics59/checkpoint78.md); latest measured checkpoint, keep clean. `BASE/checkpoint78clang-release` is a clang-21 attribution build of the same source ([execution79](issues/register-semantics59/execution79-instruction-gap.md)) |
| `worktrees/handlers80` | `pyjit/handlers80` | Call-free handler bodies, `db39c583a6c` (pushed), from `99a04733128`; builds `BASE/handlers80-{release,debug}`; evidence `E59/handlers80/`. **Parked** 2026-11-19: kernels −0.4 to −2.4% instructions, hot handler text +15%; the periodic tail livelocks `test_capi` pending-call tests. Superseded by the [handlers81 proposal](design/proposals/handlers81.md) |
| `worktrees/handlers81` | `pyjit/handlers81` | [Handlers81](design/proposals/handlers81.md) rollout H0–H5, from `99a04733128`; builds `BASE/handlers81-{release,debug}` (copies of checkpoint78's, paths rewritten); evidence and agent progress logs `E59/handlers81/`. Root integrates each step: build both roles, the step's test ladder, kernel instruction counts; full suites (release then debug, never in parallel) after H2 and H4 |
| `worktrees/handlers81-h4` | `pyjit/handlers81-h4` | H4 (computed-goto evaluator), landed as `744d4ad992a`. Worktree and builds `BASE/handlers81h4-{release,debug}` stay as the base for the layout follow-up: logging_silent's front-end stalls first appear here ([record](issues/register-semantics59/handlers81.md#logging_silent)) |
| — | `pyjit/handlers81-h5s2bc`, `pyjit/handlers81-h5s3`, `pyjit/handlers81-h5c` | Calls80 steps 2b/2c and 3, merged into `pyjit/handlers81` (H5 landed at `d6d1ed5ac93`). H5c (`4f431e2a5f3`, pushed) is parked. Their worktrees and builds are removed. Evidence `E59/handlers81/{h5s2bc,h5s3,h5-merge,h5-tests,h5c,full-h5,bench}/`; [record](issues/register-semantics59/handlers81.md) |
| `worktrees/gap82` | detached at `pyjit-golden-20261120-gap82` | **Frozen golden82** `5194e957c5d` ([gap82](issues/register-semantics59/gap82.md) integration, pushed as `pyjit/gap82` and `pyjit/round82-golden`): H5 + TX, SL, CS, K1, N34, ST, N56, G, K2b, K2a and two test follow-ups. Builds `BASE/gap82-{release,debug}`, aliased `BASE/golden82-{release,debug}`; never rebuild or relink them. Evidence `E59/gap82/` (`coordinator.md` is the log; `int/` holds integration builds, smoke, instruction counts, full suites and probes; `bench/` holds the campaigns). New work branches from the tag in a new worktree |
| `worktrees/parity84` | detached at `pyjit-golden-20261121-parity84` | **Frozen golden84** `4be2eb5d31f` ([parity84](issues/register-semantics59/parity84.md) IP2, pushed as `pyjit/parity84` and `pyjit/round84-golden`). Builds `BASE/parity84-{release,debug}`, aliased `BASE/golden84-{release,debug}`; never rebuild or relink them. Evidence `E59/parity84/` (`coordinator.md` is the log). New work branches from the tag in a new worktree |
| `worktrees/hot85` | detached at `pyjit-golden-20261122-hot85` | **Frozen golden85** `2a9ea8c4f69` ([hot85](issues/register-semantics59/hot85.md) IP3, pushed as `pyjit/hot85` and `pyjit/round85-golden`): golden84 + AT, FL, CP, DT, XP, HN, EV and two root fixes. Builds `BASE/hot85-{release,debug}`, aliased `BASE/golden85-{release,debug}`; never rebuild or relink them. Evidence `E59/hot85/` (`coordinator.md` is the log; `int/` and `bench/`). New work branches from the tag in a new worktree |
| `worktrees/tight86-<unit>` | `pyjit/tight86-<unit>` | [Tight86](issues/register-semantics59/tight86.md) units HT, GE, PC, NC, NE, CM, EX, IA, from golden85, made by `E59/tight86/mkbuild.sh UNIT COMMIT` (`mkbuild-all.sh`); builds `BASE/tight86<unit>-{release,debug}`. Evidence `E59/tight86/` (`coordinator.md` is the log; `common.md` and `brief-*.md` are the briefs) |
| `worktrees/hot85-<unit>` | `pyjit/hot85-<unit>` | [Hot85](issues/register-semantics59/hot85.md) units, from golden84, made by `E59/hot85/mkbuild.sh UNIT COMMIT`; builds `BASE/hot85<unit>-{release,debug}`. All merged except RT (`6eb44a40113`, rejected: no instruction gain). `hot85-hc` is phase A's prototype. Evidence `E59/hot85/` (`coordinator.md` is the log) |
| `worktrees/parity84-{nb,gl,it,rm}` | `pyjit/parity84-<unit>` | Parity84 phase-1 units, from golden82; all merged into `pyjit/parity84` (IP1, `5a51846ce16`). Builds `BASE/parity84<unit>-{release,debug}` |
| `worktrees/parity84-{bd,gn}` | `pyjit/parity84-<unit>` | Parity84 phase-2 units, from IP1; merged into `pyjit/parity84` (IP2, `4be2eb5d31f`). Builds `BASE/parity84<unit>-{release,debug}` |
| `worktrees/gap82-{cs,tx,sl,g,k1,k2a,k2b,n34,n56,st}` | `pyjit/gap82-<unit>` | Gap82 units, all merged into `pyjit/gap82`. Builds `BASE/gap82<unit>-{release,debug}` are per-unit attribution builds (the cycles probes use them). H6 was parked with no commits and removed |

Magic remains 64427. The return correction changes no public layout, metadata or
ordinary target table. Source commit `557c225602c` integrates as `e668764b766`;
`44f066f5fd5` commits its two generated products. The design records the exact
private route contract and independent source/product adoption.

Final boundary11 qualification: 803 release / 792 debug methods in 24 modules,
eight skips each; debug omits 11 host parser methods. Six additional retirement
methods pass per role. All eight debug owner controls and the rooted recursion
failure control pass. HOME and retained controls explicitly require the new
GEN_RETURN_ROUTE entry. All eight release native controls pass, including the
extra private hop. Ordinary arithmetic/exact counts match selected57; unique,
coroutine and HOME completion remain lower; retained completion costs 11 more
instructions and yield ten more. See the report for the exact table.

`E59/execution73-boundary11-hot8-selected57-screen` has 32 successful rows and
stable identities. GM is 0.985010; logging 1.137318 fails protection in both pairs.
The independent verdict and whole-path audit are in
`E59/execution73-boundary11-native-peer.md`, with screen recomputation JSON.
Winner confirmation, all29, full suite and a fresh upstream comparison have not
run. These
remain broader qualification work. No target job remains; G and its build
directories are frozen. Use another worktree for successor source changes.
Golden refs `pyjit/round73-golden` and
`pyjit-golden-20261116-generator-boundaries` are pushed at `44f066f5fd5`.

Earlier boundary10 is preserved at `pyjit/execution73-boundary10-measured`,
`dee619e80abbcab1c2a69a07411e17cc0b32ccfa`. Its ELF copies are
`E59/execution73-boundary10-{release,debug}.elf`, with hashes and omitted native
source-file digests in `execution73-boundary10-retained-identity.json`.
They are static evidence after G advances, not runnable controls against G's new
source. Its screen is `execution73-hot8-selected57-screen`; its finite PMU
comparison is `execution73-boundary10-diagnostic`. Do not attribute those PMU
ratios to boundary11. The earlier frozen72 comparison remains separately in
`execution73-diagnostic` and the canonical [diagnosis](issues/register-semantics59/execution73-diagnosis.md).

Reusable root-only controls are `E59/execution73-{native,owner,entry}-*.py` and
`execution73-qualify-protocol.py`. Every new invocation needs a fresh tag/manifest.
The HOME controller's initial effect-only expectation and the first unreached
entry injection remain preserved as failed evidence; their corrected controls
retain callback, owner and reached-entry checks. No prior receipt is overwritten.

Astra owns grouped architecture review, Sol workers source and independent peer,
and root integration/target jobs/selection. Preserve earlier refs and evidence.

## Closed trials and retained evidence

Selected57 is the prior immutable control. Both72 units and rounds59–71 are
unselected. The
[trial report](issues/register-semantics59/report.md) owns their source identities,
qualification, measurements and rejection decisions. Do not repeat those
chronologies here or make historical experiments prerequisites for current work.

Important retained refs:

| Trial | Preserved integration |
| --- | --- |
| Execution72 ABI | `pyjit/execution72-abi-integrated`, `c6d54fd3b5d` |
| Execution72 layout | `pyjit/execution72-layout-integrated`, `240f4c2b977` |
| Round71 terminal split | `pyjit/parity71-terminal-integrated`, `e8be22ec1e1` |
| Round71 canonical provenance | `pyjit/parity71-canonical-integrated`, `ff1618987bd` |
| Round69 guarded summaries | `pyjit/parity69-guarded-summary-integrated`, `52806aacedf5` |
| Round68 retained delegation | `pyjit/parity68-retained-delegation-integrated`, `c02cebf80c48` |

Preserve these refs and sealed `E59` receipts. Old captured ELFs are static
reference material after their source/Lib trees or build directories move.
Magic 64424 identifies rejected layout72; 64425 identifies rejected ABI72;
64426 identifies the initial boundary73 shapes; 64427 adds explicit completion consumers. Never reuse an old format identity.

## Paths and immutable controls

`BASE=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`;
`R=BASE/worktrees`; G=`R/call55-gc`, P=`R/call55-pins`, W=`R/call55-ir`,
A=`R/parity61-ascent`, I=`R/call55-integrated`.
`E59=BASE/pyjit-link-wave1/register-semantics59`,
`E57=BASE/pyjit-link-wave1/comprehensions57`,
`E55=BASE/pyjit-link-wave1/call-model55`.

| Control | Source commit / tree | Release build | Module record |
| --- | --- | --- | --- |
| Golden85 | `2a9ea8c4f69902ab398c6e872a54a25290f96e90`, `R/round85-golden` | `BASE/hot85-release` (alias `BASE/golden85-release`) | `E59/hot85/int/ip3/build-release.log`, 79 extensions; use with `--control-role golden --control-head` |
| Golden84 | `4be2eb5d31f3e06e39d158740b46666a3b8c2810`, `R/round84-golden` | `BASE/parity84-release` (alias `BASE/golden84-release`) | `E59/parity84/int/ip2b/build-release.log`, 79 extensions |
| Golden82 | `5194e957c5d573ef2b6112508a7b0990b7de656d`, `R/gap82` | `BASE/gap82-release` (alias `BASE/golden82-release`) | `E59/gap82/int/k2a/build-release.log`, 79 extensions |
| Golden73 | `44f066f5fd55947edf709bbcf9bf17c607307e4b`, G | `BASE/reg59-results-release` | `E59/execution73-boundary11-release-build.log`, 117/79 |
| Selected57 | `0dcba75bb3df40c32137db453b001f7e8f101de0`, I | `BASE/call55-integrated-release` | `E55/comp57-dictionary-release-build.log`, 117/79 |
| Golden56 | `883524c180e4486b47df77ee2276c9276c0ec32c`, `R/round56-golden` | `BASE/round56-golden-release` | `E57/control-retry-build.log`, 117/79 |
| Upstream | `3a440b65695615e9fd9912342b79cdd54fced0d2`, `R/upstream-3a440b6` | `BASE/upstream-release-devlibs-3a440b6` | `BASE/pyjit-link-wave1/checkpoint-closure/checkpoint-6a0e4c12c17-final/upstream-build.log`, 116/78 |

Keep I's debug build frozen too. Selected57's golden ref is `pyjit/round57-golden`; I's checkout branch is
`pyjit/comp57-dictionary-candidate`. Its tag is `pyjit-golden-20261114-dictionary-readers`. Selected54
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
