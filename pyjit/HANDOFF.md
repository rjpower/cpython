# Coordinator handoff

Canonical documentation is this checkout; its runtime source is stale. **Do
not build it.** Start with [GOAL](GOAL.md), the
[active plan](issues/interpreter-performance56/plan.md), [STATUS](STATUS.md),
[architecture](design/runtime-architecture.md) and [process](PROCESS.md).
The current direction is [execution-boundary73](design/proposals/execution-boundary73.md),
adopted for Sol 6.1 implementation after grouped and source cross-review. The
[trial report](issues/register-semantics59/report.md) owns measured outcomes;
this file owns execution logistics.

## Current work

Execution-boundary73 has grouped Astra, independent source and root adoption GO.
P and W implement from selected57 after cross-review of the runtime and compiler
contracts in `E59/execution73-{runtime,compiler}-proposal.md`. The subsequent
HOME amendment is in the canonical design: a live result can require its real
scratch home across another call or await; that necessary store earns no
home-elimination credit.

| Tree | Branch | Current responsibility |
| --- | --- | --- |
| G | `pyjit/execution73-boundary-integrated` | Clean candidate `d9a9cff0631`; both `execution73-boundary8` builds pass 117/79 |
| P | `pyjit/execution73-generator-transfers` | Runtime, generator and effect-retirement implementation |
| W | `pyjit/execution73-generator-compiler` | Compiler, public admission and focused tests |

P owns the initial shared boundary header. W consumes it, adds the bounded
common predicates and tests, and cross-reviews P. Root owns generation,
integration, builds, target execution and timing. Workers must not run target
jobs or alter immutable controls. Source integration includes the compiler,
runtime, rooted low-recursion entry and monitoring identity corrections.
Boundary8 generated products have independent peer GO: all ordinary contracts
and CALL/arithmetic bodies are preserved. Implementation corrections cover
virtual cursor publication, provider-result rooting, explicit STORE_FAST/POP_TOP
consumption, transparent monitoring target selection and interpreter-trampoline
retirement. The two matched observation probes distinguish PRE source attribution
from POST consumer attribution. No runtime behavior was changed to satisfy an
incorrect fixture.

Both builds match `d9a9cff0631`; magic is 64427. All 22 new boundary methods plus
the two observed crash regressions pass per role in `execution73-boundary8-fixes`.
The 24-module combined protocol check passes 803 release and 792 debug methods,
with eight skips each. Debug explicitly excludes 11 host parser checks. Eight
debug owner controls and eight release native cost controls pass. Unique
completion remains more expensive (884 versus 769 instructions); retained
completion is flat (536 versus 537). P proposes a guarded callback-free close
of an already-cleared unique receiver, with W independent peer before edits.
The memo is `E59/execution73-cleared-receiver-proposal.md`. Root will price the
whole corrected path before hot8. The first entry-recursion injection did not
reach its private breakpoint. The corrected control admits the actual ordinary
SEND entry and passes in `execution73-boundary8-debug-entry2`, preserving the
failed receipt and all root/error/recovery checks.
No73 benchmark performance result is qualified yet. Earlier build/test failures and
matched probes remain under `execution73-boundary{3,4,5,6,7}`.

Root's focused protocol driver is `E59/execution73-qualify-protocol.py`; it records
its exact selections and delegates identity-checked execution to run-checks.
The native controller now accepts a proved private GEN_SEND_INPUT producer for
the complete mode while retaining the ordinary SEND start and identical consumer
end. Its previous version and selected receipts remain intact.

Root completed and sealed the bounded frozen72 logging/raytrace diagnosis in
`E59/execution73-diagnostic`, with driver `E59/execution73-diagnostic-driver.py`.
The [diagnosis](issues/register-semantics59/execution73-diagnosis.md) owns its
findings. Eight selected57 native controls are ready, including retained versus
finalizing receivers and a two-await expression with a live intermediate result.
Controllers are `E59/execution73-native-{fixture,controller,run}.py`;
`execution73-native-control-peer.md` records independent readback.

The user authorized this corrective implementation and the bounded diagnosis.
Closed-trial stop rules do not prohibit73; they prohibit restarting a rejected
executable unchanged. Astra remains outside the component implementation loop.

## Closed trials and retained evidence

Selected57 remains golden. Both72 units and rounds59–71 are unselected. The
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
