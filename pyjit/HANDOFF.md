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

Source4db1bb0b5e3 and both native builds complete117/79 modules. It contains
IRSC1 semantic recovery, IRSM2 optional positions, shared typed projections,
independent PRE observer ingress, checked CALL input cleanup and the actual
outlined ERROR processor. Its direct ingress passes the current handler's
bridge workspace; no successful-path record or extra handler argument is added.
Checkpoint0 is initialized-NULL control; positive ERROR authority remains open.

The preceding d045 composition passes361 cases in eight debug modules and
admits687/687 source files, including all189 stdlib entries. Fixture-only13b
passes143 disassembly cases per role. Current4db freezer passes five bootstrap
and all27 wider admission probes. Its ten-module runtime/monitoring group runs
1,093 cases per role: nine modules pass, tracing retains exactly six selected54
jump-error identities. Evidence: E/observer-pre-*, E/error-scope-*.

Five new ERROR witnesses initially fail both roles on fixture assumptions.
Outer kind2 transition records belong to the runner; require exact initial
chain restoration. assertRaises clears retired traceback frames, erasing the
captured local before inspection; inspect before explicit clearing. Reviewed
fixture3881c3a9a18 preserves actual owner/GC/phase/cursor assertions and checks
mutation both during cleanup and after capture. Root composes it and exact
emitter fixture43c1b5b049a at I73ea72ad45d. Both native identity rebuilds complete
117/79, and all five ERROR witnesses pass per role. The4db runtime is unchanged
by these fixture-only source updates; quiet hot6 campaigns complete.

Retained4db release ELF is E/error-scope-native/python, SHA256
1c8cb55439680d6f8c713ed07d4b6ccab8366d75cbdff99eb6915291dfcb0fd4.
CALL_NO_SELF442 bytes, EMPTY return780, instrumented LINE356; prior d045 has
402/747/311. The outlined processor is2,009 bytes. No code-size win is claimed.
Static frame header stays112 bytes before homes; HomeView16, format44.

The latest two-pair elapsed screen is I73ea72ad45d. Against54, hot order
DeltaBlue/generators/coroutines/Richards/comprehensions/logging gives
0.960034/1.023798/1.021070/0.993148/1.012176/1.183415. Direct upstream gives
1.155724/1.431449/1.285008/1.237290/1.382734/0.866682. Pair ranges and stable
campaign identities are in the report, E/error-scope-hot-screen-{golden,upstream}.
This is mixed, with four cases losing both54 pairs. No new PMU or promotion.

## Bitmap checkpoint

Current B0af2ff5b66a and both builds complete117/79. It adds the reviewed
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
