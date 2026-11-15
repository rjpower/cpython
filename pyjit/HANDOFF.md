# Coordinator handoff

Canonical documentation is this checkout; its runtime source is stale. **Do
not build it.** Start with [GOAL](GOAL.md), the
[active plan](issues/interpreter-performance56/plan.md), [STATUS](STATUS.md),
[architecture](design/runtime-architecture.md) and [process](PROCESS.md).
The current grouped design is [parity65](design/proposals/interpreter-parity65.md),
supported by [review65](issues/register-semantics59/review65.md).
The [trial report](issues/register-semantics59/report.md) owns all measured
outcomes, failures and selection decisions; this file owns execution logistics.

## Current work

Selected57 remains unchanged. Every round59–62 runtime packet and both role63
trials are rejected. PyValue integration is complete; parity is not achieved.
Round64 is closed: compact fails its native net-work gate; complete await
passes mechanism qualification but loses the broad screen. Execute the reviewed
round65 return/accessor unit, followed conditionally by its private SEND-yield
continuation. Preserve both round64 source branches and sealed evidence.
Source approval, native mechanism approval and measured selection are separate
gates. Never import a rejected executable as a prerequisite.

| Owner | Current source boundary |
| --- | --- |
| Sol A, W | Correction committed as `8ed034598b3`, independently reviewed and integrated; 20 bounded source checks and three affected generated-case methods pass. Source frozen |
| Sol B, P | `/root/boundary65_continuation`, branch `pyjit/parity65-yield-continuation` from G `0bbb42ef46a`; complete continuation proposal and independent A peer. No B implementation before proposal GO and revised A native gate |
| Root, G | Original `ee0e77046591f39079b663160e8011ac1b29df76` passes both 117/79 builds, 46 focused and 708 runtime/reader methods per role, but cold native review rejects it: EMPTY paths add three instructions (one for redirect-discard); GENERAL paths save five. Preserve its ELF and evidence. Corrected G is `a6695887e949c3019216120c544e171dfa7ff90e`; regeneration passes with no generated changes and both `parity65-residence3` builds pass 117/79, 46 focused and 708 runtime/reader methods per role. Corrected native gate is underway; no timing or selection claim |
| Astra | Source `374f8e3eadbac12be504f70885fe3a2a196f8fc0` plus `ceb46447bb0ef5a48f2478ba5f62f842d43e9550`, branch `weaver/interpreter-parity65-astra`, Loom `zfr7ow1e`; integrated C at `f18cefa4449` and `ba7316040e9`. Do not cherry-pick again |

Use existing Sol workers for component cross-review. Sending a message to a
completed collaboration agent does not trigger a turn; use followup_task when
reactivating it. Astra is for grouped reviews, not the patch loop. Its Loom
channel can carry the next grouped request after this measured cycle.
The old collaboration node `parity62_astra` was interrupted; it may still appear
pending_init and consume a slot. Current Astra authority is the completed Loom
session above. Do not retry that stale node.

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
Complete await final integration is **`0bbb42ef46ab9c55996e9b507a528a12705eedf0`**.
Both reused `reg59-results-{release,debug}` builds match it; await9 logs record
117/79. Both lifecycle runs pass all 16. The twelve runtime modules run 1,066
methods per role with only the six inherited sys_settrace debugger-jump errors;
release verbose diagnosis and debug verbose controls name the same six. Another
140 reader controls per role pass (three skipped): C API frames, remote
inspection and async graph. These are focused integration gates, not a fresh
full-system pass. No current runtime is selected: hot8 shows coroutine time .571378/selected57,
but logging 1.276929, generators 1.051852 and nqueens 1.024620 lose both pairs.

The correction chain is:

| Contract | Source → root integration / products |
| --- | --- |
| Initial complete source and first generated products | `f98decf1109` → `47151110b00`; products `045a09e4c65` |
| OUTCOME and reservation error; private cohort; feedback footprint; remote ABI | `0aa647a349b`, `7eeebe3eed8`, `9d310d96f83`, `7516dd9f98b` |
| Linear CALL prefix, separate SEND continuation | `c52a1a09293` → `35c4a38fc66` |
| Normal-only LOAD_GLOBAL result receipt | `cf9310fbd23e` → `ba603b73c1a`; products `791c48d0df5` |
| Lazy executing-copy fixture and valid bare-YIELD negatives | `90da5f9d471` → `0c9d1f544af` |
| Compatible SEND quickening keeps existing certificate | `afdbdf850b2` → `6ba198acf0f`; products `c539a4736eb` |
| Public-image and traceback fixture correction | `c82728bef30` → `7d83cf7670e` |
| Terminal parent phase preserved after checked alias switch | `aadc5859c0f` → `1034ad4926e`; products `0bbb42ef46a` |

Every source correction and generated diff has independent approval. The final
terminal correction has 14 passing host methods; generation removes exactly 16
parent phase clears from four RETURN/two YIELD variants, with no other products
changed. Earlier admission/native/test failures remain sealed, including debug
YIELD_DELEGATED abort and the missing late STOP_ITERATION observation. Both
are fixed by preserving the parent's continuation through the terminal switch,
not by relaxing their assertions. The OOM fixture is one-shot rather than a
persistent allocator failure, and passes without an unraisable cleanup warning.

`E59/parity64-await9-native/manifest.json` pins release ELF SHA256
`1c8f91ebf882e01250dd2c8a32a96039b76a4eff129d6b45208085412cbc68f8`.
Its independent native review finds no blocker and charges the actual private
helper, 184-byte fixed reservation, owner moves, metadata and ordinary
RETURN/SEND/YIELD/accessor costs. Ordinary generic CALL bodies normalize equal;
ordinary exact-call residence tests broaden without extra instructions. Those
facts establish neither a whole-cycle dynamic count nor a speedup.

`E59/deferred-await-native-controls` contains root controls and exact command,
ABI/ELF/script hashes and logs:

- `natural9b.native.json`: unchanged coroutine `run(4)`, one private leaf commit
  and its matching identity-free normal retirement.
- `terminal9b.native.json`: actual two-private-frame same-evaluator terminal
  move, followed by Python send/chain/GC checks.
- `arena-oom9d.native.json`: actual private VirtualAlloc forced to NULL; original
  CALL window/refcounts/PC/arena/exception state unchanged at selection and the
  child-NULL edge; helper error return; exact Python code/word error site;
  finally, successful retry and eventual payload release.

Natural/terminal use original controller `native_control.py` and quoted
`final9b-anchors.json`. OOM uses reviewed `native_control9d.py` and
`final9d-anchors.json`, checking helper PCs against its exact relocated 999-byte
ELF range. Initial GDB quoting and DWARF-clone-name driver failures remain
retained. These are matched transaction receipts, not frequency or timing data.
Controller source and ABI/receipt peers are retained beside them. Do not rerun
or overwrite these exclusive outputs. All target jobs are root-owned.

The completed campaign is `E59/parity64-await9-eight-selected57`, 32 rows,
explicit hot8, stable before/after identity and no errors. The candidate module
log is `parity64-await9-release-build.log`. Root rejects this standalone packet:
coroutines .571378, comprehensions .987018 (mixed pairs), Delta 1.003049,
generators 1.051852, logging 1.276929, raytrace .983750, spectral .924742 and
nqueens 1.024620. Preserve `pyjit/parity64-await-integrated`; no confirmation,
all29, direct upstream, PMU or full diagnostics follows this rejected screen.
No new upstream performance claim is made. G/builds currently remain coherent
and frozen; no target job is running. Before any next source change, preserve
these branch/head identities and refresh both builds after integration.

The final round65 design is integrated from the Astra source commits above.
Root independently reviewed actual source and the proposal. A redundant cold
classification pass/two private return aliases were removed because the public
PyFrame/PyGen ingress still requires the same THREAD guard; they added no
consumer/removal. Unit A changes existing complete RETURN recipes, takes and
unlinks once, branches THREAD, and uses one owned-result resumable helper.
It ports complete RETURN/YIELD source certificates and state-proved embedded
accessors. Keep GENERAL versus EMPTY clearing authority separate.

The original A ELF is native NO-GO; do not run target GDB or a timing screen
for it. Root and an independent peer approve a bounded correction that consumes
the reached THREAD proof in the two result-clear APIs and the AWAIT/GENERATOR
proof in ReturnResumable. Keep the generic reason-bearing dispatcher unchanged.
The wrong-residence source mutation and three affected generated-case consumers
are in scope. Independent actual-source review approves patch SHA256
`38d75f1e161979bc4f731b7bab41264092499d74e5e9b358ec5f1b4287b3b273`.
All 20 bounded source checks pass. Root's three affected generated-case methods
fail against the old fixtures and pass after correction. Source `8ed034598b3`
is integrated as `a6695887e94`; regeneration changes no products. Both refreshed builds pass 117/79, 46 focused
and 708 runtime/reader methods per role. Root owns final native qualification
and the unchanged hot8 screen; no corrected performance result exists yet.

`E59/boundary65-source-host/unit-a-proposal.md` records the source proposal;
its old pre-freeze classification dependency is superseded by the final design.
`continuation-reader-census.md` inventories actual B readers/writers. Direct
risks are completion arithmetic, copy, debugger/redirect and generated
return/yield/observer recipes. Remote frame/async graph readers use real
`instr_ptr` and `ir_words`, not `return_pc`; retain invariants/controls without
an unrelated layout port. Census alone does not prove support.

Unit B's private executable SEND-yield target must never become a semantic
instruction address or reach generic SAVE_STACK/debug/source attribution.
Port every actual return-PC reader/writer, preserve newer callback redirects
and observer-required terminal detach, and keep YIELD nonraising. No ordinary
header field, universal descriptor or per-code word mirror. Source contract
work proceeds alongside A; shared integration is A then B. A has its own
native and hot8 screen and may advance without B. A native failure stops A;
a broad loss permits only the already reviewed B isolation attempt. No other
lane is a prerequisite, and no new runtime result is claimed yet.

Before root integration: independent source peer, committed clean source and
full patch/path/hash receipt. Root reviews/generates/commits products, refreshes
both reused builds, runs minimal controls/native gates and measures the changed
candidate. Keep all immutable controls and selected design mirrors unchanged.
Only a winning coherent candidate advances to confirmation/all29/upstream/PMU
and full supported diagnostics. Source workers run no target/native jobs.

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
