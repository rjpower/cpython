# Coordinator handoff

Canonical documentation is this checkout; its runtime source is stale. **Do
not build it.** Start with [GOAL](GOAL.md), the
[active plan](issues/interpreter-performance56/plan.md), [STATUS](STATUS.md),
[architecture](design/runtime-architecture.md) and [process](PROCESS.md).
The current grouped design is [parity64](design/proposals/interpreter-parity64.md),
supported by [review64](issues/register-semantics59/review64.md).
The [trial report](issues/register-semantics59/report.md) owns all measured
outcomes, failures and selection decisions; this file owns execution logistics.

## Current work

Selected57 remains unchanged. Every round59–62 runtime packet and both role63
trials are rejected. PyValue integration is complete; parity is not achieved.
Compact64 has stopped at its native net-work gate. Continue complete deferred
await independently.
Source approval, native mechanism approval and measured selection are separate
gates. Never import a rejected executable as a prerequisite.

| Owner | Current source boundary |
| --- | --- |
| Sol IR, P | Compact source `6a889e40ba1ba3ca4e22ef922a3117f76493d9bb`, clean on `pyjit/parity64-compact-domain`; source GO. Whole-await source peer GO. Closing scoped compact documentation; no worker target jobs |
| Sol frames, W | `f98decf1109aac74c6254972b51e35250d33e5d5`, clean frozen complete await source; independent GO. Finishes compact native review, then repairs the await metadata-generation failure |
| Root, G | Await integration `47151110b00`, branch `pyjit/parity64-await-integrated`; full generation failed on the private output adapter. Partial generated products are uncommitted; reused builds still contain compact64 and are stale for G |
| Astra | Grouped64 complete on `weaver/interpreter-parity64-astra`, final `62f9718e9d74c30a4808610500e8b61ee0e4fc80`, Loom channel `zfr7ow1e`. Root integrates at `1b03334cab6` plus final entry/exit clarification `d1137161d4e`; review worktree is `/home/power/code/cpython/.worktrees/interpreter-parity64-astra` |

Use existing Sol workers for component cross-review. Sending a message to a
completed collaboration agent does not trigger a turn; use followup_task when
reactivating it. Astra is for grouped reviews, not the patch loop. Its Loom
channel can carry the next grouped request after this measured cycle.
The old collaboration node `parity62_astra` may remain pending_init; current
Astra work is the completed Loom session above, not that node.

The [compact proposal](design/proposals/interpreter-parity64.md#1-compact-domain-a-bounded-change-to-physical-execution)
requires one ordinary-C finite domain under one NCSR adapter, with ordinary CALL
cuts, current owning homes and at most one borrowed homed mirror. Finite entry
aliases preserve their ordinary formats; ordinary exits explicitly unwrap them.
Only list-FOR and APPEND aliases enter the adapter externally; method, argument
and jump aliases use ordinary external targets and remain internal cases.
Generator-family code retains its semantic plan without domain nomination.
The return is a word index plus exit kind, not an address-taken state structure.
Before generalization, root's actual native artifact must fit **128 bytes total
simultaneous adapter/domain stack** and **16 KiB hot domain text**, and remove
net whole-path work. Do not rescue a failed gate with flags, more opcodes or a
second compiler.

The [await unit](issues/register-semantics59/deferred-await63.md) owns its complete
contract. Stable active frames use the unchanged 112-byte header plus a 72-byte
header-side record, actual exception/name/identity roots and disjoint arguments
moved once. Existing parent SEND/return state supplies the continuation;
recursive private entries inherit the evaluator boundary in constant time.
Observation creates views, never arbitrary active relocation.
Terminal suspension is confined to the owning evaluator suffix. SEND_CHILD is
distinct from FOR_ITER and exact AWAIT_WAIT. Unsupported callee suspension,
including bare YIELD, must decline. Freeze the ordinary-reader/hot-cost ledger
before entry. Preserve owner, error, observer, debugger and rewrite closure;
source tests alone cannot enable a head.

Await snapshots and peer reports are under `E59/deferred-await-source-host`.
Complete `whole1.patch` is `440f95456513b07fa6ee5528e94071da5a2820dc1662529d2029c935b6379d0b`;
`whole1-freeze.json` owns its 34 paths and exact test list. Independent full-source
GO is `whole1-peer.md`: eleven host methods pass, thirteen runtime methods are
AST-only. Native entry-chunk OOM needs targeted VirtualAlloc failure injection;
ordinary allocator hooks do not cover it. Earlier bounded slices remain evidence,
not the complete-packet gate. The common interfaces are
`_PyIR_ReplaceHandler(co, words, i, handler, specialize)` and
`_PyCode_BuildSemanticPlan(co, &plan, words)` using final common replay.
Agree shared generator exits before dependent edits. Await reserves IDs 383–385;
compact IDs 393–409 are held for its bounded unit, emitting only needed IDs after
registry peer. Public images never admit private IDs.

## Current candidate and retained evidence

Preserved branch `pyjit/parity64-compact-integrated` contains compact source `6a889e40ba1` at `9733e2f5f1e`, generated products
`739ce231145`, and fixture correction `9de7005b670`. Both `parity64-compact1`
builds pass 117/79. New release methods ran seven passes and one fixture failure:
`tb_lasti` is already a semantic word index, so dividing by eight was wrong.
The shorter second synthetic image also needs fresh line/exception maps.
Both corrections are fixture-only and independently reviewed. Both `compact2`
refresh builds pass 117/79; eight new runtime methods and 21 control methods
pass per role. The native artifact fits 112 bytes simultaneous stack and 1665
bytes hot text, but nominal complete paths add work: 227 versus 173 and 280
versus 234 instructions; append singleton is 128 versus 38. Stop at that gate,
without timing or target coverage. Partial coverage preparation is unused and
unqualified. No compact runtime is selected.

`compact64-source-host/root-generated-checks.json` verifies all 1,128,105
ordinary generated bytes and 331 ordinary metadata formats are unchanged,
resolving reordered ownership-program indices to their actual event sequences.
The static selected baseline is `E59/compact64-baseline-static`; no runtime was
executed to prepare it. Native evidence is `E59/parity64-compact2-native`.
The complete await source is now integrated separately at `47151110b00`.
`parity64-await1-release-regen.log` records its first full-generation failure:
`handler_output_kind` rejects the new private window adapter. Repair in W,
independent source review, root regeneration and generated-diff review precede
builds. Reused release/debug builds remain compact64 and must not execute G's
changed Lib tree.

The frozen rejected attribute branch contains source `c1066265668` at `1fe09389531`, generated products
`18445aa21d5`, and fixture-only `035aa32e16c` at final `581c018a34a`.
Both `parity63-attributes2` builds pass 117 modules/79 shared extensions.
All 54 focused methods pass per role: corrected new seven under attributes2,
other 47 release under attributes1 and debug under attributes1-debug-controls.
The fixture failure/correction did not relax production admission.

`E59/parity63-attributes2-native` captures ELF SHA256
`0b827d770ba806326b82565bb916e8e19f2b5af7d5ead7e4e81cc71c714349a0`.
Independent native review and actual kernel first hits for all four new bodies
pass; `E59/attribute63-coverage` owns static nominations and bounded hits, not
frequency. Stable hot8 `E59/parity63-attributes2-eight-selected57` rejects:
comp .964506 and Delta .968592 improve, but logging 1.222917, spectral 1.063393
and generators 1.017802 lose both pairs. No confirmation/all29/upstream/PMU/full
campaign follows. Those old build contents have been replaced by compact64;
its standalone captured ELF remains static evidence only.

The list/FOR+append trial is preserved on `pyjit/parity63-role-integrated`, at
`034f2b22f87bb4e9e54e689bb5ead8a252e60a97`; its native evidence is
`E59/parity63-role2-native`, coverage `E59/role63-coverage`, and losing screen
`E59/parity63-role2-eight-selected57`. Its old build contents are no longer
current. Round59–62 branches, source and captures remain preserved. The report
owns their exact identities and outcomes; do not revive Invoke59's already
rejected duplicate-sweep cleanup, providers, scalar descriptors, continuation
actions, alignment searches or old binaries without a different reviewed consumer.

Captured standalone ELFs are for static inspection. **Never execute them against
a changed Lib tree.** After switching G, reused native directories are stale
until rebuilt. No existing candidate build is an additional immutable control.
A is frozen on `pyjit/parity62-delegated-resume`, at `d43f88d0402`, and has no job.
P preserves prior source branches; W preserves method source `882309dd897`.

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
