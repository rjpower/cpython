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

Round67 is closed: A and B qualify their mechanisms and lose their screens.
Selected57 remains unchanged. The grouped round68 proposal is delivered and
independent architecture and exact source-contract approval pass. Common loop
proof, inert materializer, generator consumers, engine ingress/error ports and
cold frame publication/copy/native readers have independent approval and are
integrated on G at `7a068c60410`. Both current builds pass 117/79 and all 23
focused methods per role; no root job is running. Sol-high owns hot lifecycle
and observer closure in W. Another Sol-high worker owns remote inspection and
debug offsets in P, branch `pyjit/parity68-delegation-remote-readers`, from
`0dee7ab83a1`; its exact placement proposal has root GO.
These changes do not allocate generator records or activate retained execution.
Activation waits for complete lifecycle and reader
closure. Preserve actual owning frames and retained ancestors, enter the current
leaf directly, and change only the suffix. No second unit is active.

Materializer source `41f68b9611a` plus actual-thread-owner correction
`125c5570e8e` integrates as `6c713b18e7b` and `2244d12e313`. Both
`parity68-materializer3` builds pass 117/79. Generator consumer source
`d1eb8eeda58` integrates as `ff64c134db7`; both `parity68-genobject4`
builds pass 117/79, and all 17 methods pass across
`test_ir_retained_delegation` (8) and `test_ir_delegated_yield` (9).
Logs use `E59/parity68-{materializer3,genobject4}-{release,debug}-build.log`
and `E59/parity68-genobject4-{release,debug}-focus.log`; command manifests
use the same tag and stage. Source reviews are
`retained-delegation68-source-host/{materializer3,genobject4}-source-peer.md`.
No runtime records exist yet, so these passes cover ordinary behavior, not
retained mechanism.

Engine source `51a09a254de` integrates at `517fb2dcff2`; cold frame source
`0dee7ab83a1` integrates at `79ee6079264`. Both new behavior methods were
inserted at class head; root resolved the overlap and compared all ten method
ASTs against both source heads. Products `7a068c60410` change only two SEND
created-state reads. Both readers6 builds pass 117/79; all 23 methods pass per
role: retained delegation (10), delegated yield (9), and four selected/created
copy methods in `test_frame`. Logs/manifests use `parity68-readers6`; generation
ran only in release. Independent `engine5-source-peer.md` and
`frame6-source-peer.md` are in the source-host and frame-host evidence directories.

The exact corrected hot publication protocol has independent GO; its ordering
is incorporated in the [contract](issues/register-semantics59/retained-delegation68.md#publication-ordering).
New memberships are fenced before new-member authority changes. All membership
clears and replacement-link publication are fenced before direct free. Cold
materialization retains its reviewed seq-cst begin marker and adds the lifetime
fence. No hot source, activation or native cost is approved by that design GO.

Proof2 source commits are `647652f293e230f2440e30bded3adbd11502cf10` and
`0f0093208ecf05d67f7d42170584e985a8344b36`; root integrations are
`8ecefca2fa5` and `348a1911975`, with generated products at `f4d7feb285b`.
Release/debug builds pass 117 modules/79 shared extensions and all 30 focused
methods per role across `test_ir_retained_delegation` (5),
`test_ir_created_plan`, `test_ir_delegated_yield`, and
`test_ir_transfer_admission`. Foundation1's earlier 16-method focus overlaps
and remains historical; do not add it to the proof2 count. Regeneration ran in
release only. Logs are `E59/parity68-proof2-release-regen.log` and
`E59/parity68-proof2-{release,debug}-{build,focus}.log`;
command manifests are `parity68-proof2-{regen,build,focus}-commands.json`.
Independent source and product reviews are in
`E59/retained-delegation68-source-host`. This establishes no generator record
allocation or activation, native mechanism, performance result, or broad
qualification.

Source-contract obligations include effective state/PC/home readers, all-member
materialization before teardown callbacks, numeric recursion accounting per
ingress, and remote committed-snapshot/lifetime safety. Nonblocking sampling
must remain nonblocking: bounded retry or explicit refusal without dereferencing
a nonowned record. Blocking snapshots require transition validation even after
all threads stop. The source peer must approve actual remote frames.c and
asyncio.c consumers, not only a schema description.

Root owns generation, builds, target/native jobs and selection. Workers run
bounded host checks and write discoverable runtime tests for root.

| Owner / retained state | Checkout / branch |
| --- | --- |
| Sol-high source | W, `pyjit/parity68-retained-delegation`; hot lifecycle and observer closure |
| Independent Sol peer | Inert ports and corrected ordering GO; complete hot/remote source review pending |
| Root runtime | G, `pyjit/parity68-retained-delegation-integrated`, `7a068c60410`; readers6 builds current, no job running |
| Sol-high remote | P, `pyjit/parity68-delegation-remote-readers`; debug offsets and remote inspection |
| Astra | Grouped review68 source `183447221537190a4ac1003026f554dbaf428a3d`, integrated in C `29a016e656d`; no runtime work |

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
