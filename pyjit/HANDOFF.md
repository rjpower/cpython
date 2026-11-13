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

`979f624cac1` is the exact native identity in both
`BASE/call55-integrated-{release,debug}` builds; each completes 117/79.
The preceding e90 composition passes 167 focused cases per role, including all
123 `test_ir` cases. Its semantic-base projection in five CFG readers closes
the new bool source-jump regression; setter permissions and fixture semantics
are unchanged. Current monotone exceptional receipts pass 13 cases per role.
The initial 8 source-generator methods are expensive under debug: run pure
producer tests once on the host and native metadata/runtime controls separately.
Evidence: `E/exceptional-routes-scope-*` and `E/monotone-absence-*`.

Current source `7a70dc35b75` additionally composes reader84e81 and main7a70:
IRSC1 semantic recovery, IRSM2 optional positions, common admission and all
execution/observation/dis/remote adapters. Source peers and host checks pass;
both-role 117/79 builds pass, but focused integration finds fixture and real
admission gaps; see E/irsc-send-* logs. W source main commit is6dccb599c17;
GC reader source486b406189d. Root resolved one absence-fixture conflict by
preserving normalized physical positions and the new separate recovery table.
SEND is composed at25585. Current I9be99bb3d3f adds the reviewed typed
projection grammar after the actual async trace; freezer/native gates pending.

The freezer still passes five bootstrap and 26/27 broader probes. Remaining
async admission loses absence across protected SEND at word24; GC owns the
shared entry/error/completion receipt and guard family closure, with IR/frames
actual peers. No getter or test-specific exception. The earlier compile-only
census passes682/684 images, including all189 standard-library entries.

Source coverage at979:328 handlers,284 nonempty normal phase programs,
12 COMPLETE exceptional contracts, four ordinary abrupt and four suspend abrupt.
These counts do not establish complete ownership/readability authority.
`E/authority-census-e90` retains its initial wrong-field script failure and
corrected result; `E/authority-census-979` is the current successful census.

The latest two-pair selected54 screen is
`E/monotone-absence-hot-screen-golden`: DeltaBlue1.130007,
generators1.008836,coroutines1.017062,Richards1.032109,
comprehensions1.023607,logging1.278120. Ranges are in the report. This is no
broad gain and no isolated A/B attribution. There is no new979 direct-upstream
or PMU campaign. `E/logging-code-inventory-979` finds matching hot-frame
geometry and warm families, with no hidden constant locals in logging's hot
functions. Do not attribute that gap to constant initialization.

Exact ELF/dumps at `E/monotone-absence-native`, SHA256
`d5aa77f5015bee3ff846525f76272067e4b607e8c801718d964501c17073b4c9`.
Format metadata stays44 bytes, new error_routes uses padding byte27;
HomeView stays16 bytes; fixed frame header is owned_slots offset112
(`sizeof`120 includes the first value cell). Static NULL homes and conservative
arena scanning remain authority. General RETURN uses real residual-owner
cleanup; EMPTY retains the small path. M4 has not switched.

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

That checkpoint uses counted bits: zero is not permission to read
inline/control/no-count homes. See [writer closure](issues/call-model55/writer-closure-ledger.md),
[authority assessment](issues/call-model55/bitmap-authority-assessment.md)
and [known-home proposal](issues/call-model55/known-home-capabilities.md).
The three matching known-home bodies are now composed and qualified under
NULL control. Their new B screen also loses every pair:1.327228,1.228186,
1.350701,1.217027,1.250380,1.587998 in hot order. Evidence
`E/bitmap-known-home-{checkpoint,focus,native,hot-screen-golden}`. Root selects
same-byte non-NULL occupancy as the next complete-rival direction. Its typed
16-byte HomeView now distinguishes immutable ranges from positive presence;
capture rebinds the view to destination storage. Local active-child lookup
clips overlap, and unsupported foreign queries return NONE explicitly. Pending
unpack cells remain externally owned until callback-free publication. GC and
private drains consume presence; conservative arena enumeration is removed
in the draft. Native stack/register/PyMem discovery policy is unchanged.

Presence packet4a1a39fa9e3 and I25585 compose cleanly at B20576b58f4b.
Both builds complete117/79; the presence/provider focus passes17 cases per role.
Four lifecycle modules pass25 release cases, but that driver also requested a
nonexistent copy module and fails overall. Corrected debug selection pending.
Frames has found a real raw CloseStack CALL writer; its shared checked drain
must land before presence qualification. Keep B frozen during native work. Execution frontier, copy gap
initialization and physical Take NULL writes remain: raw nullable readers still
need readable NULL. This is an ownership authority change, not yet a complete
stale-scratch candidate. No second plane, FT claim or zero-bit load permission.

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
