# Coordinator handoff

Canonical documentation is this checkout on `weaver/python-jit`. Its C files
are not the measured runtime. Start with [goal](GOAL.md),
[plan](issues/call-model55/plan.md), [status](STATUS.md),
[execution ledger](issues/call-model55/implementation.md) and
[report](issues/call-model55/report.md). The reviewed
[target design](design/proposals/register-call-model55.md) remains distinct
from the as-built contracts. Follow [process](PROCESS.md).

BASE=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
E=`BASE/pyjit-link-wave1/call-model55`.

## Controls and active sources

Selected54 remains `61015a9452d9069646fb5fede9ff963896833b89`, branch
`pyjit/round54-golden`, tag `pyjit-golden-20261112-frame-state`.
No model55 candidate is golden. Preserve immutable controls:

| Role | Source under BASE/worktrees | Build under BASE | Head |
| --- | --- | --- | --- |
| Selected54 | frame54-cold-construction | frame54-cold-construction-{release,debug} | 61015a9452d9069646fb5fede9ff963896833b89 |
| Prior51 | round51-golden | round51-golden-{release,debug} | 0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254 |
| Upstream | upstream-3a440b6 | upstream-release-devlibs-3a440b6 | 3a440b65695615e9fd9912342b79cdd54fced0d2 |

One GCC15.2 configuration, CPU15, hash0, JIT disabled. Selected54 is117/79
modules/shared; upstream116/78. Control module logs:
`BASE/pyjit-link-wave1/frame-state54/cold-construction-initial-release-build.log`
and `BASE/pyjit-link-wave1/checkpoint-closure/checkpoint-6a0e4c12c17-final/upstream-build.log`.

| Active source under BASE/worktrees | Branch | Owner and next work |
| --- | --- | --- |
| call55-integrated | pyjit/call55-integrated | Root: generation, builds, native checks, measurements, integration |
| call55-pins | pyjit/call55-pins | Sol high frames: complete presence execution readers and stale-scratch policy |
| call55-ir | pyjit/call55-ir | Sol high IR: positive ERROR phases and full static authority |
| call55-gc | pyjit/call55-gc | Sol high GC: lifecycle review and ERROR/presence composition |
| call55-bitmap | pyjit/call55-bitmap | Root-owned frozen presence rival composition |

Reuse the three live workers; all four agent slots are occupied. Source-only
component peers are continuing cross-lane reviews, not fresh-context audits.
Astra broad review is complete and stays out of the inner loop. Root alone
runs generated/native commands. Workers may use host AST/source checks.
Never mutate a source while its native job or quiet measurement is running.

## Current static checkpoint

Preceding source/native I dc6bad8a625, both117/79 builds. It composes shared input schema
51360665d5f and positive ERROR source7a94d51e97d with the prior outlined
processor, IRSC1 recovery/IRSM2 positions, PRE observer ingress and checked CALL
drain. Generated code differs only at GET_ANEXT's two error checkpoint literals.
Bootstrap5+27,18 focused runtime cases and233 IR/code cases pass per role.
Source/non-native effect tests remain separate; no complete static map claim.

One counted ERROR suffix shares the ownership-event allocation. A selector
class byte in16-byte PhaseSite distinguishes ACTIVE/ERROR under an immutable
site. Pure explicit-origin query selects GET_ANEXT projection/helper failures;
actual RAISE/retirement roots have authority2 with clean GC and retained local
mutation/capture. Missing proof uses initialized control3. New handler roles
replace the view with control until their own transform is proved. Native
frame header112, HomeView16 and format44 remain unchanged.

Preceding4db runs1,093 lifecycle/monitoring cases per role: nine modules pass,
tracing retains six selected54 jump-error identities. Its fixture-only73ea
follow-up passes five ERROR witnesses per role. PRE d045/13b passes361 debug
cases plus143 disassembly cases per role; census687/687 sources,189stdlib.
Retain exact identities; these are not full/current-dc6 runtime qualifications.

Current two-pair54 screen in hot order DeltaBlue/generators/coroutines/Richards/
comprehensions/logging:0.980768/1.029420/1.017063/0.983703/0.986702/1.037997.
Logging range0.982866–1.096220; mixed, no promotion. The separate direct upstream
screen atdc6 gives1.167857/1.456962/1.278716/1.244322/1.365744/0.760757.
Both complete with stable identities and no failures. Do not combine campaigns.
No new PMU/all29 fordc6.

Evidence E/positive-error-snapshots-*; exact release ELF SHA256
95e69c53868255c6883d03a4298887ae7e1d2bfbaec3d67f808fa96c9fc053fd.
Earlier captures/campaigns remain at their recorded source heads in the report.

Current I source/native8232dd8adde adds the shared scalar storage-action graph and
native-only admission. E/scalar-storage-host passes20 source methods and full
cases/metadata/contracts emission; full regeneration and both117/79 builds
pass. E/scalar-storage-runtime-* passes23 cases per role; admission passes233
per role, one skip. Its stable two-pair54 screen is1.017884/0.997913/1.022717/
0.978000/0.981699/0.981101; mixed, no promotion. Its separate direct upstream
screen is1.177991/1.430017/1.292308/1.242869/1.332579/0.756278. Both campaigns
finish stable and without failures. No all29/PMU for8232 yet.
E/scalar-storage-native retains its ELF, SHA256
2aba9d15fc2092083917d8998083a59cf4e3d038c131801084b2c9b49827223a.

## Bitmap checkpoint

Last completed native B0af2ff5b66a and both builds complete117/79. It adds the reviewed
PRE/ERROR cohort732c7cbe2b7, four literal fixture ports and generated products.
Bootstrap5+27,13 ownership/error cases and617 runtime cases per role pass.
Retained ELF E/bitmap-error-scope-native, SHA256
e36785fe8bb3f238adbfbccd36996c6670e8f92853555770f3f3fc0fa0895ba6.
Preceding B0c47a4616ef also completes117/79. All27 ownership/provider/writer
cases and510 cases across eight lifecycle/runtime modules pass per role.
The one non-NULL presence plane includes inline, control, immortal and counted
values; zero never permits loading a stale word. HomeView has typed range and
presence forms; heap capture rebinds the latter. Local child lookup clips
waiting parent overlap, pending unpack prefixes own edges externally until
callback-free publication, and checked CALL drains detach before finalizers.
GC/private drains use presence. Native conservative discovery remains policy;
conservative arena enumeration is removed only in this rival.

Old execution frontier, copy-gap and Take NULL stores remain in the measured
checkpoint. Its two-pair54 screen loses every pair: hot order
1.252746/1.203232/1.291231/1.140563/1.239636/1.723286 at0af2. This prices presence
plus old execution work, not the complete rival. Evidence:
E/bitmap-error-scope-*, E/bitmap-presence-gc-integration-*, E/bitmap-presence-drain-*.

Frames develops complete GIL stale-scratch execution policy in P, merged from
exact B0c47. Shared input-only required/nullable roles and positive array reads
must close every C/generated/observer/remote consumer before fill removal.
Bindings stay physically initialized. Under Py_GIL_DISABLED the existing
physical-NULL policy remains; concurrent stale storage is a separate gate.
GC completed ERROR/presence composition in G and now peers P/IR; it owns
a disjoint matching runtime-query hunk in W by explicit IR coordination. Root alone mutates B and generates/builds it.

Current B source is1ee9cae4ece, with the complete GIL stale-reader packet,
terminal read-route correction and explicit native-entry read. The host gate
passes nine focused methods plus full cases/metadata/contracts at frozen
20b830; corrected generated metadata matches0af2. Bootstrap5+27 admission
passes at8aa. Full native build exposes old arena-probe fields/API, fixed in
7e692/688, then crashes on native INTERPRETER_EXIT reading a nonexistent
presence map. Exact GDB/ELF evidence is retained. Native read495/productsd0
removes that crash. Grouped deletion protocols now include STORE_NAME,
large-name SETATTR and a shared nullable-value deoptimization guard for all
specialized STORE_ATTR handlers. Full emission passes atade9. The next native
startup crashes in LOAD_FAST_BORROW: a hardware watchpoint proves that the
overlapping reservation initializes bindings through a reused frame header's
old base before installing the new base. Frames is correcting the construction
ordering across all entry paths in sourcef1b. The repeated failure atf1b uses
an old ceval.o: the inline bitmap header is missing from PYTHON_HEADERS.
Root1ee9 corrects that dependency and full rebuilding passes frozen-module
execution, then fails during regex compilation at FOR_ITER_LIST's required
cursor check. The failing activation is entering this loop for the first
time after GET_ITER_VIRTUAL deoptimizes. LIST/TUPLE need the existing nullable
cursor deoptimization guard before their required cursor projection, as
FOR_ITER_VIRTUAL already does. Frames is implementing that grouped correction.
Evidence E/bitmap-base-bind-deps-* and E/bitmap-base-bind-required-gdb. Raw
FFF8...2 is stale inline integer2, not an index; early unsuccessful watchpoint
attempts used the wrong tag and establish nothing. iteration-trace.log and
getiter-trace.log identify the first-entry/deoptimization route. No current B full build or timing
pass exists. Its shared builds are in development and must pass strict module
and native identity gates before reuse. Evidence E/bitmap-stale-reader-*,
E/bitmap-stale-bootstrap-freeze-gdb, E/bitmap-native-entry-*,
E/bitmap-native-absence-gdb. The latter homes.log pointer inspection is excluded
for a reserved GDB-register assignment; its original error backtrace and source
lowering independently establish the deletion-role finding.
Current crash ELF, backtrace and valid handoff watchpoint are retained at
E/bitmap-delete-closure-gdb; the release build failure is
E/bitmap-delete-closure-release-build.log. The earlier watch.log follows a
successful invocation's ordinary clear; handoff-watch.log identifies the bug.

## Remaining work and diagnostic limits

M0 placement census/review selects shared storage and initialized named locals
as reference; actual dirty-local/chunk/copy work still needs measurement.
M1–M3 are partial. Semantic recovery and actual cold ERROR extraction are
integrated; complete positive callback/error/poll/read authority remains open.
A normal event program is not complete lifecycle coverage. Do not assign
positive ERROR checkpoints until their actual transformed home shape is proved.
M4 static switch/non-stack allocation, complete M5 rival, remaining bounded
M6 and M7 selection/qualification remain open. Keep working; a preparatory
source packet or reviewed design is not the full user checkpoint.

Latest broad55 evidence remains `ed08ab64c7e`: all 29/upstream1.165467,
six faster; matched DeltaBlue instruction ratio1.368099. Do not extrapolate
new hot screens to all 29 or multiply independent comparisons. Latest full
foundation0851 runs54,763 tests per role and discovers an allocation-injection
crash. Fix7c42 requires positive execution readiness before specialization;
the original OOM and837 focused cases pass afterward, but no green full55
suite follows. Prior debug jump, LLTrace, GC-verifier instability and the
unexplained full-run profiling failure remain explicit. Only the existing
GDB pretty-print class is quarantined. Selected54 limits and rejected
experiments are in its [report](issues/frame-state54/report.md) and
[qualification](issues/frame-state54/qualification.md).

## Execution and publication

Host scripts: `UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B`.
Use [run-checks.py](issues/call-model55/run-checks.py) for exclusive command
manifests and source/native/magic/module gates. Regeneration must include global
labels: handler-only contracts miss optional emitter-state initialization.
[check-freezer.py](issues/call-model55/check-freezer.py) screens all probes
before a full build. Failed broader admission is not a module/test pass.
[capture-native.py](issues/call-model55/capture-native.py) retains exact clean
ELFs for integrated or bitmap variants.

Serialize builds, tests, GDB, profiles and quiet timing. No native or bulk
cleanup jobs during timing. Preserve driver/workload/control identity before
and after. Save ELF before advancing shared builds. Do not reuse frozen PMU
wrappers for a new head. Full suites belong at coherent representation gates.
Record unsupported/failing cases instead of new quarantines.

Push canonical documents before Loom mirrors. Use fresh guarded revision and
exact readback at `http://127.0.0.1:7878/s/xzzlxx17/artifacts/`.
Goal → plan → status/ledger/report → current/proposed designs is the chain.
Authenticated publication uses gh's HTTPS credential helper without changing
origin; preserve golden tags/controls. Never stage/reset/delete unrelated
preexisting files `pyjit/bench/results/checkpoint-6a0e4c12c17.md`,
`pyjit/issues/checkpoint-closure/ctypes-rawpin-build.log`,
`pyjit/issues/checkpoint-closure/w5-release.lock`.
