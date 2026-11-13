# Coordinator handoff

Canonical documentation is this checkout on `weaver/python-jit`. **Its C files
are not the measured runtime.** Start with [goal](GOAL.md), [status](STATUS.md),
[active plan](issues/call-model55/plan.md), [execution ledger](issues/call-model55/implementation.md)
and [active report](issues/call-model55/report.md). The
[target design](design/proposals/register-call-model55.md) is reviewed, not the
as-built runtime. [Process](PROCESS.md) defines component and integration gates.

BASE=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
New evidence E=`BASE/pyjit-link-wave1/call-model55`.

## Selected runtime and immutable controls

Round54 remains selected at `61015a9452d9069646fb5fede9ff963896833b89`.
Published branch `pyjit/round54-golden` and tag
`pyjit-golden-20261112-frame-state` are unchanged. It is a cold constructor
correctness checkpoint, with no demonstrated general performance lift.

| Role | Source under BASE/worktrees | Build under BASE | Head |
| --- | --- | --- | --- |
| Selected54 | frame54-cold-construction | frame54-cold-construction-{release,debug} | 61015a9452d9069646fb5fede9ff963896833b89 |
| Prior51 | round51-golden | round51-golden-{release,debug} | 0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254 |
| Upstream | upstream-3a440b6 | upstream-release-devlibs-3a440b6 | 3a440b65695615e9fd9912342b79cdd54fced0d2 |

One GCC15.2 configuration, CPU15, hash0 and JIT disabled. Selected54 has
117 modules/79 shared; upstream 116/78. Control module logs are
`BASE/pyjit-link-wave1/frame-state54/cold-construction-initial-release-build.log`
and `BASE/pyjit-link-wave1/checkpoint-closure/checkpoint-6a0e4c12c17-final/upstream-build.log`.
Do not rebuild or modify controls during active55 evidence.

Selected54 full diagnostics ran 54,692 tests per role. Release retained nine
debugger-jump errors and an unstable GC-verifier failure. Debug retained nine
debugger errors, LLTrace and an unexplained full-run profiling failure. Targeted profiling repeats
passed both controls; that does not establish the original failure's cause.
Only the existing GDB pretty-print class is quarantined. The
[round54 qualification](issues/frame-state54/qualification.md),
[report](issues/frame-state54/report.md) and [tables](issues/frame-state54/benchmark-tables.md)
retain the exact evidence. Closed rejected experiments and their verified
archives remain historical; no performance variant entered golden54.

## Active55 integration

Root owns `BASE/worktrees/call55-integrated`, branch `pyjit/call55-integrated`,
and `BASE/call55-integrated-{release,debug}`. Worker sources are isolated
`call55-{pins,ir,gc}` worktrees/branches. These are experimental sources, not
new goldens. Workers own source/proposals/peers; root alone runs generation,
builds, tests, profiles, timing, commits and publication.

Foundation `0851fd51801` includes typed independent code/function pins,
partial admission/retirement proof, nested transition roots, staged unpack and
binding, provider handoff, classify-once retirement, constructor entry/fallback
and fixed-last heap-frame teardown. Both builds retain 117/79 modules. Focused
checks pass. Full foundation diagnostics completed 54,763 tests per role.
Both roles newly fail dictionary allocation injection; release also retains
GC-verifier instability and nine debugger errors, debug LLTrace and nine
debugger errors. The new crash is specialization's ambiguous zero success/
failure sentinel. `7c42db43224` requires positive execution readiness before
cache nomination. The original OOM fixture now passes both roles and selected54;
837 core cases pass per role. See E's `m3-null-foundation-*-full-commands.json`,
`m3-oom-*-commands.json` and logs for exact evidence. No green full suite is claimed.
NULL homes and conservative arena scans still authorize execution/collection.

The latest measured phase head is `ed08ab64c7e`: all29 versus upstream
1.165467, six faster, with matched interval profiles. Evidence prefixes are
`m2-phase-{hot-screen-golden,all29-upstream,perf-cycles-instructions,perf-record}`.
675 core, nine phase and nine selected generated-contract cases pass per role;
groups overlap. Keep campaign scopes separate;
[report](issues/call-model55/report.md) records ratios/ranges and failed commands.
Retained ELFs and native dumps live under E. No candidate is promoted.

M0 selects shared storage with initialized NULL named locals as the M4 reference;
[placement costs](issues/call-model55/placement-cost-model.md) document the choice.
M2 shared-emitter phase events and meaningful nonempty shadows are integrated
at `6ff13875cfe`, with fixture-only corrections `ed08ab64c7e`. Source/native
bounded post-review is GO under NULL control. Heap teardown resets through
InitClearRoots before callbacks. IR home authority, GC probe/record union and
frames copy producers at `197fd05cb27` pass 84 precise and 588 lifecycle cases
per role. `8f6567840e7` adds pinned-code generator allocation: the new fixture
crashes pre-fix197fd release/debug, then passes four mutation subcases and 399
focused cases per role after the fix. Both builds retain 117/79 modules.
Created-copy source `9f28c0fa16d` composes IR event `9db7ea96578`, frames
consumer `514fb9af661` and emitter punctuation fix `2e620b73bf3`, with generated
products. Initial compile failed on the marker's missing semicolon; the fixed
version builds both roles and passes 580 lifecycle plus five generator-contract
tests per role. Its raw admission fixtures used invalid RETURN_GENERATOR A
fields and a one-past observer target; test-only correction `76630a14599`
retains the coverage. Created selector composition `44383f6baa7` passes all four
raw methods per role and 594 lifecycle cases per role; the negative multisite
fixture explicitly retires its branch-local owner before joining. Native copy
helper has no stack allocation/query; the GC visitor has 72 local plus 48
saved bytes and a canary. Hot6 still loses versus54. `91269fba2ce` adds vacant
return-recipient handling and passes 288 focused cases per role, still losing
the hot screen. CALL merge `aad17cd9c1c` composes runtime/compiler f7b47 and IR
03be plus generated products. Release compilation succeeds, but module freezing
fails on unproved holes and a FBARG normal-clear decoder crash. Repair44424 and
regenf3477941eb0 fix FBARG replay and unused-name aliasing that also corrupted
emitted exact CALL argument geometry. The freezer-only build succeeds and the original
probes reject unproved holes without crashing. Through74d91398881, generated
post-promotion fallback, fused branch cleanup, consumed tails and loop exit
proofs are integrated. The latest five-image freezer check passes builtins
and rejects four other images normally. Full decoded-image evidence identifies
single-exit observer, local loop-output and folded unpack proof gaps; IR and
GC own the reviewed disjoint repair packet in call55-ir. Do not time this
partial build. The root-owned check-freezer.py records all five probes before
a full build, reducing failed full-build churn. GDB evidence is retained under
E/call-roles-external-gdb*. The bitmap source worktree starts at9f28 and needs
complete writers/readers before authority switching or rival measurement.
Workers implement virtual CALL roles and copy/escape/created-frame ports.
The [reader census](issues/call-model55/lifecycle-readers.md)
queues copy/escape, unwind, results/resume, GC and observation ports.
Complete static authority, the full bitmap rival, optional lowering and final
M7 qualification/selection remain open. Partial shadow metadata is not permission
to remove scratch clearing or arena enumeration.

## Execution and recovery

Use `UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B` for host
scripts. [run-checks.py](issues/call-model55/run-checks.py) writes exclusive
command manifests and checks source/native identity. Build mode rejects missing
or failed-import extensions even when make returns 0. It forces getbuildinfo
refresh so native identity follows the committed source.

Never overlap quiet timing/profiles with builds, tests, running GDB or bulk
cleanup. Keep source, native build, workload/driver and configuration identities
stable before and after measurement. Save the exact ELF before advancing the
shared candidate builds. Do not reuse a frozen profile wrapper for a new head.
Full suites belong at coherent integrations; targeted failures need bounded
repeats and control diagnosis, not extra quarantines.

Canonical docs are mirrored with guarded revisions and exact readback under
`http://127.0.0.1:7878/s/xzzlxx17/artifacts/`. Push canonical commits before
publishing pinned links. Keep goal → plan → ledger/status/report → selected and
proposed designs explicit. Authenticated GitHub publication uses gh's HTTPS
credential helper without changing origin. Preserve old golden tags and controls.

Never stage, reset or delete unrelated preexisting files:
`pyjit/bench/results/checkpoint-6a0e4c12c17.md`,
`pyjit/issues/checkpoint-closure/ctypes-rawpin-build.log`,
`pyjit/issues/checkpoint-closure/w5-release.lock`.

## Latest normal-transport qualification boundary

Integrated `e75b7088b12` has actual normal disjoint-child, exceptional-prefix
and async-loop completion receipts, plus fixture repairs. Both release/debug
builds complete 117/79 with no import failures; native build identity is e75.
Evidence: `E/call-async-exit-checkpoint-{build-commands.json,*-build.log}`.
The earlier 5cc composition also completes both builds. Its 11-module release
focus runs 88 cases and fails 3 modules; separately 8 modules/41 cases pass debug.
Post-repair four-module release runs 51 cases and fails only the general-return
contract and a malformed generator-negative fixture. No full CALL qualification
or new timing is established. See `E/call-disjoint-eh-{focused,failure-detail,
eight-debug}*` and `E/call-async-exit-repaired-release*`.

The current freezer passes 5 bootstrap and 26/27 broader sources. The coroutine
module first rejects a nested async comprehension CALL 44 with held SELF 7:
GET_ANEXT 22 at B10 can raise to END_ASYNC_FOR 35 but has no general abrupt-prefix
receipt. Earlier held-yield/CLEANUP_THROW and matched async-exit cases now pass.
Evidence: `E/call-async-exit-next/{commands.json,gdb.log,decoded.txt}`.
Normal-only output/drain receipts cannot prove that exceptional ingress.

GC/IR/frames are freezing general RETURN plus positively selected EMPTY under
[general-return-proposal.md](issues/call-model55/general-return-proposal.md).
Actual THREAD Try/association retirement currently clips to L, so removing only
the constructor's leftover-owner rejection would leak in release/assert debug.
Port the actual full-N general route and root/capture extent. Preserve EMPTY's
hot body and keep physical NULL distinct from logical absence.

Delegated source is W 6063d7c6385 (contains isolated 30a3 and EH composition),
frames P da0c629bfeb, consumers G 16f465dc0e5 plus remote follow-up 159ada57ad2.
Merge matching packets together after current normal/abrupt fixes; preserve
case1 result-bit1/disjoint-bit2 handling and exceptional UNKNOWN tuple wrappers.
Review has closed SEND suffix/helper-source, instrumentation remap/twin shadow,
unused C and discarded resume-recipient lookup mismatches. Root host smoke
predates the final fixes; regenerate and test the actual composition.

Bitmap source/builds remain `BASE/worktrees/call55-bitmap` and
`BASE/call55-bitmap-{release,debug}`. Source 853bd363ce1 regenerates unchanged,
native freezer passes 5+25/26 broader images. No complete module/test pass,
stale-scratch switch or performance comparison exists. Root's earlier merge
restored emitter ports caught by peers before generation. The exact AST/IO
bridge must be replaced by actual emitter capabilities; P now implements the
fixed-index/helper packet with ordinary callback effects retained. No new
bitmap source/native result is claimed. Absorb current static ABI before
comparison. Do not copy old runtime-worktree documentation over canonical docs.
