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

The latest broad measured provider head is `de95475accb`: all29 versus upstream
1.162061, six faster, with matched interval profiles. Constructor `9136c202e8e`
and heap foundation have hot screens only. Keep campaign scopes separate;
[report](issues/call-model55/report.md) records ratios/ranges and failed commands.
Retained ELFs and native dumps live under E. No candidate is promoted.

M0 selects shared storage with initialized NULL named locals as the M4 reference;
[placement costs](issues/call-model55/placement-cost-model.md) document the choice.
M2 shared-emitter phase events and meaningful nonempty shadows are integrated
at `6ff13875cfe`, including GC collector/probe/redirect ports and generated
products. Builds and merged validation are running. Source GO alone is not a
runtime pass. Heap teardown resets through InitClearRoots before callbacks.
Workers are syncing to this base for virtual CALL roles and copy/escape ports.
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
