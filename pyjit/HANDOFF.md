# Coordinator handoff

Canonical documentation is this checkout; its runtime source is stale. **Do
not build it.** Start with [GOAL](GOAL.md), the
[active plan](issues/interpreter-performance56/plan.md), [STATUS](STATUS.md),
[architecture](design/runtime-architecture.md) and [process](PROCESS.md).
The latest completed checkpoint is [execution-boundary73](design/proposals/execution-boundary73.md),
selected as golden73 by the user on 2026-11-16 with the logging regression
accepted. The
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
| `worktrees/values76` | `pyjit/values76` | Universal value-operation rules from round 2: callers own and callees borrow, internal entry points, cold inline paths; then one value-slot table. Builds `BASE/values76-{release,debug}`; `pyjit/values76-draft` holds a stopped run's unverified edits |

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
