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
| call55-pins | pyjit/call55-pins | Sol high frames: known-home commits, counted bitmap writers/readers |
| call55-ir | pyjit/call55-ir | Sol high IR: exceptional receipts/admission, semantic recovery container, fixtures |
| call55-gc | pyjit/call55-gc | Sol high GC: error effect summary/processor, active selection, observation |
| call55-bitmap | pyjit/call55-bitmap | Root-owned composition of isolated counted-bitmap rival |

Reuse the three live workers; all four agent slots are occupied. Source-only
component peers are continuing cross-lane reviews, not fresh-context audits.
Astra broad review is complete and stays out of the inner loop. Root alone
runs generated/native commands. Workers may use host AST/source checks.
Never mutate a source while its native job or quiet measurement is running.

## Current static checkpoint

`ff14b98be4b` is clean source and exact native build identity for both
`BASE/call55-integrated-{release,debug}`. Both builds complete117/79 with no
failed imports. It composes explicit CALL roles and delegated suspension,
checked fixed/loop/helper writers, conditional NULL outputs, general RETURN
and proved EMPTY, plus direct tuple/list steal-success receipts.

Six focused modules pass 67 tests per role: general/empty return, edge NULL,
completion, retirement plan and home writers. Initial four failures were
fixture errors: inherited EMPTY after geometry edits, old CALL naming and
missing RESUME in two synthetic frame/monitor images. The fixes preserve
forged-EMPTY negatives and actual owner lifetime/capture assertions. Expanded
`test_ir` runs123 cases with12 errors and3 failures: old CALL/EMPTY fixtures
plus a debugger jump regression. Matched identical-AST control passes both
selected54 builds and fails both candidate builds
(`E/bool-source-jump-control`); do not classify it as preexisting. Do not call full qualification green.
Evidence: `E/general-return-{handoff,fixture}-*` and
`E/general-return-ir-detail-release-focus.log`.

The actual source-admission census compiles 682/684 images. All189 stdlib
entry modules pass; remaining failures are nested async comprehensions in
`test_coroutines` (CALL44, virtual SELF7) and `test_ir_abrupt_operands`
(CALL29, SELF4). Earlier curses/long/random failures are closed. This is
compile-only, not execution. Evidence: `E/general-return-source-admission`.
The freezer passes 5 bootstrap and26/27 broader probes, failing the same
coroutine gap. Finish actual exceptional/redirect contracts, not successful
normal receipts, to preserve held absence there.

Retained ELF plus five handler/helper dumps: `E/general-return-native`,
SHA256 `50c271cb461bfcf2d3ea990bc6379657571b44b5391cdb7b92a185e0a7117ab8`.
Two-pair hot screens complete with stable clean identities at
`E/general-return-hot-screen-{golden,upstream}`. Candidate/54 ratios are
1.039263,1.043308,1.052147,.984842,1.007455,1.004977 in hot-set order;
direct-upstream1.230703,1.504706,1.326683,1.230958,1.356191,.728430.
Keep pair ranges in the report; no broad gain or selection. The preceding
writer50f and delegateda265 campaigns remain separate. I advances through known-home3b4547380d0 (both 117/79 builds,36 focused
cases per role, eight host source tests) and fixture002, exceptional-route
producer/consumer and CFG fix to current e90c6a4b7f9. Regeneration passes;
metadata adds the actual route byte. Both-role builds are running at
`E/exceptional-routes-scope-checkpoint-*`; do not mutate I until completion.
The known-home selected54 hot screen finishes with stable identities and no
broad gain; logging 1.199787 (range1.136662–1.266417), other ratios/ranges in
report. No isolated source effect is established.

Static NULL homes and arena scanning remain authoritative. M4 has not
switched. General RETURN uses a real full-N NULL-control retirement route;
EMPTY retains the small path. This does not permit full-N stale reads later.
Logical NO_HOME, physical NULL, initialized/readable and counted edge facts
remain distinct. Bound-method/constructor promotion can write virtual SELF;
its absence proof is still necessary unless that actual call ABI changes.

## Bitmap checkpoint

B `0ee3a61aba3` is clean and matches both
`BASE/call55-bitmap-{release,debug}` native builds,117/79 each. Eight focused
modules pass 176 tests per role, eight skips. Five bootstrap and26/27 broader
compile probes pass. Nine copied body guards are replaced by checked common
Storage operations. It is not a stale-scratch representation checkpoint.

The earlier bf7 two-pair selected54 screen loses all six cases: DeltaBlue 1.335421,
generators 1.222776, coroutines 1.333615, Richards 1.210015,
comprehensions 1.279180, logging 1.603326. Each pair loses. Evidence
`E/bitmap-home-writer-hot-screen-golden` and exact ELF/dumps
`E/bitmap-home-writer-native`. This prices bit maintenance plus retained NULL
work, not the final representation. Old-value/bit checks are redundant where
emission already holds the canonical owner; the bounded known-home packet
carries those facts without another frame field or record.

Keep counted bits: zero is not permission to read inline/control/no-count
homes. See [writer closure](issues/call-model55/writer-closure-ledger.md),
[authority assessment](issues/call-model55/bitmap-authority-assessment.md)
and [known-home proposal](issues/call-model55/known-home-capabilities.md).
The three matching known-home bodies are now composed and qualified under
NULL control. Their new B screen also loses every pair:1.327228,1.228186,
1.350701,1.217027,1.250380,1.587998 in hot order. Evidence
`E/bitmap-known-home-{checkpoint,focus,native,hot-screen-golden}`. Root selects
same-byte non-NULL occupancy as the next complete-rival direction; exact typed
view/partition interfaces still require peer freeze. No second plane, FT claim
or zero-bit read permission. Frames owns implementation after freeze.

The first B full build lacked UUID headers because pkg-config's sysroot was
missing. Reconfigure used exact I CONFIG_ARGS and
`PKG_CONFIG_SYSROOT_DIR=BASE/devlibs/root`; every MODULE_* compile/link flag
now matches I. The failed commands remain evidence, not runtime failures.

## Remaining work and diagnostic limits

M0 placement census/review selects shared storage and initialized named locals
as reference; actual dirty-local/chunk/copy work still needs measurement.
M1–M3 are partial. Complete actual callback/error/poll/observer producer and
reader coverage, semantic recovery off optional positions, and the real error
processor with explicit origin/view/cursor. Error route metadata/admission
and recovery-container proposals are in progress, not native authority.
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
