# Coordinator handoff

## Active round

User direction, 2026-11-10: target DeltaBlue and review ordinary calls
holistically.
Astra performs one broad grouped architecture review; Sol handles implementation,
peer review and debugging. Incremental mostly-beneficial landings are allowed with
explicit tradeoffs. CPI/annotation experiments, coroutine entry and keyword micros
are deferred. The single [Astra review](issues/deltablue-call-model-20261110/astra-architecture-review.md)
is complete: canonical class reads first, classmethod pair forwarding second,
conditional leaf-region elision next. First landing47 `986c65ee5ab` is pushed;
Release and debug builds check 117 modules and each pass 571 focused tests. The
API check covers 306 units and is clean. Basic two-pair results are
DeltaBlue/upstream 1.26303 and logging/upstream 0.91755. See
[integration47](issues/deltablue-call-model-20261110/integration47.md).
Sol is implementing A, classmethod forwarding, in the main runtime worktree and
is host-testing it. In isolated `BASE/worktrees/deltablue-leaf-prototype`, C, the
conditional leaf experiment, is under implementation at base47. Its proposal
review is complete. Root owns native jobs, including frozen intermediate
control builds. Stats remains46. Sol handles all subsequent component reviews.

## Closed checkpoint

Runtime `4d666e1b482cb4983c7683f91c5922af14d0ab40` is the frozen checkpoint.
Its original source branch `pyjit/frame-call-model-20261109` is now the active
object/call integration worktree, at committed candidate `b8e7e1bee66`; use the new
detached golden source below for immutable comparisons. Production changes are
`24f4d270ced`; the final commit adds reviewed debugger tests. No unfinished
coroutine, owner-bundle or GDB port is included. Root owns current native jobs; consult the active packet before running more.
The next round starts from the [current priorities](issues/deltablue-call-model-20261110/plan.md),
not historical rankings, numbered integration reports or unqualified drafts.

Logging confirmations pass at 0.906735/0.897500 upstream. Full29 means are
0.934192/golden and 1.174985/upstream. All 65 micro instruction and six timed memory
rows completed on upstream/golden/candidate. Current PMU 18/18 and profiles 6/6
pass identity/running/loss checks. See [report](issues/frame-call-model-20261109/closing-report.md),
[tables](issues/frame-call-model-20261109/closing-benchmark-tables.md) and
[independent audit](issues/frame-call-model-20261109/final-performance-audit.md).

Full release/debug each run 54,645 tests. Known failures: GDB 16/21 respectively,
nine frame-jump errors in each, plus one debug trace-format assertion. Pdb,
monitoring and collectors pass. Stats smoke 150; API 306 units/zero failing
findings/parse errors. Do not call the full suite green. GDB format is not a
throughput/Python-semantics gate. The historical GC negative-control failure
remains unexplained, despite passing these final diagnostics.

## Local paths and controls

BASE=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
Canonical docs are `/home/power/code/cpython/.worktrees/python-jit`, branch
`weaver/python-jit`; its implementation files are not the measured runtime.

| Role | Source below BASE | Build below BASE | Full HEAD |
| --- | --- | --- | --- |
| Candidate release/debug | worktrees/execution-boundaries-20261108 | execution-boundaries-{release,debug} | 986c65ee5ab6433ea80ab3d385abfe5ca8dd7675 |
| Stats, previous round | worktrees/execution-boundaries-20261108 | execution-boundaries-stats | b8e7e1bee6611eeda320ce163cd3ce89323da877 |
| Previous golden | worktrees/pyvalue-rc-universal | pyvalue-rc-repr-{release,debug,stats} | 0765e9be47f55d97ba5d31d2ae1c02a55ea2e5ae |
| Current golden | worktrees/golden-20261109-runtime | golden-20261109-release | 4d666e1b482cb4983c7683f91c5922af14d0ab40 |
| Upstream | worktrees/upstream-3a440b6 | upstream-release-devlibs-3a440b6 | 3a440b65695615e9fd9912342b79cdd54fced0d2 |

Golden and upstream source/builds remain immutable. The previous golden tag is `pyjit-golden-20261108-runtime`. Current runtime39
is published as `pyjit-golden-20261109-runtime`, with combined docs/runtime
snapshot `cb906f9bc52` at `pyjit-golden-20261109` and the existing moving golden
branch. [Publication receipt](issues/golden-20261109/publication.json) records
remote readback. New frozen release build checks 117 modules and contains 79
current DSOs; historical physical file counts are not admission gates. Candidate magic is
64417; all three builds identify the current full source and check 117 modules.
Upstream checks 116 modules. Physical DSO counts 78 upstream/213 value builds
include historical ABI files and are not checked-module counts.

Root owns native execution, builds, commits and measurements. Existing launchers:
`issues/object-call-20261109/run-checks.py` and its `measure.py` target the
current evidence directory and new runtime39 control. The closing
supplemental/PMU scripts retain their historical identities. Read their argument
requirements and preserve output directories. Host Python uses
`UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B`.
Measurements use GCC 15.2, CPU 15, JIT off, hash 0 and fixed inputs. No source/build
or benchmark mutation during timing. Full suites belong at coherent checkpoints.

## Evidence and deferred source

The active packet is [DeltaBlue calls](issues/deltablue-call-model-20261110/plan.md),
evidence `BASE/pyjit-link-wave1/deltablue-call-model-20261110`.
The prior packet's evidence remains `BASE/pyjit-link-wave1/object-call-20261109`.
Source46 adds lookup/keyword/default guards, two shared-frame inline
declarations, independent guarded cache storage, magic64417 and the disassembly
fixture repair. All three builds identify46/check117 modules. Focused45 passed
564 tests each; API45 covers identical production C,306 units/zero failing
findings/parse errors. Full45 release/debug each ran54,674 tests with known GDB
16/21, nine frame-jump errors each, debug trace-format1, and disassembly8. The
fixture-only46 successor passes143 disassembly tests each; stats46 smoke150passes.
No full46 suite was run. All native jobs are closed; source/builds are frozen.

Current [report](issues/object-call-20261109/checkpoint46-report.md),
[tables](issues/object-call-20261109/checkpoint46-tables.md) and
[prior plan](issues/object-call-20261109/plan.md) record the historical HOLD
decision. The active incremental policy supersedes its blanket performance veto.
Full29 means1.00382/39 and1.18120/upstream; coro/spectral/comprehension
regressions repeat. Matched39 counters now prove higher aggregate CPI with
near-equal instructions/branches and near-equal branch-miss rates; exact mechanism
remains open. Golden stays39. Do not add a broad object-layout change or another
compiler to diagnose it.

Evidence: measure46-all29-{golden,upstream}, measure46-regressions-golden39,
four measure46-*-long, instructions46-micro-v2, gap46-perf-{stat,record},
gap46-golden-{cycles-instructions,branches}, gap46-profile-decoded, coverage46-*.
Two closing46 command journals distinguish the first load-guard rejection from
accepted retry/profile/counter captures. All seven elapsed campaigns and accepted
counter groups have stable identity. Earlier41/43 records stay historical;
43 failed the union API rule. Initial44/45 stale-format build failures and the
mistaken first focus module name remain. Build45b succeeds after magic64417.
Do not delete failed evidence or silently pool it with accepted measurements.

E11=`BASE/pyjit-link-wave1/frame-call-model-20261109`.

- Final timing: closing39-kernels-{golden,upstream}, closing39-logging-{1,2}.
- Full coverage: closing39-supplemental-v2. Initial supplemental preflight
  measured nothing and is retained separately.
- Current counters/profiles: closing39-gap-perf-{stat,record}, matching retained
  ELFs and calibration in each directory. Older36 counters are historical.
- Full diagnostics: closing39-full-release-release-full.log and
  closing39-full-debug-debug-full.log. Build/stats/API logs start closing39-.
- Deferred complete source: deferred-after39-source/{all-pending.patch,
  owner-and-coroutine.patch,gdb-tooling.patch,manifest.json}. These apply on
  committed38; the first includes test ports already landed in39.
- Authoritative isolated coroutine packet: coroutine40-frozen-source.patch.
  Release minimal passed; debug asserted on its synthetic handler-to-RESUME
  negative. Exact patch reversed. It is not admitted; owner draft is unreviewed
  natively and partial GDB port unfinished.

The task-owned null-pre-20261109 worktree and branch were removed only after
verifying complete archive hashes. Do not recreate them from stale agent task
messages. Historical source/evidence worktrees remain untouched.

Old417 executable moved from build-local python-canonical-before to
`E11/retained-controls/python-canonical-before`. SHA256:
`f010ea12dd5b93cea78b670639d340029331df7ae1f0ac39993d239600fb47fe`.
Its relocation.json maps the old path; sealed historical manifests are unchanged.

## Documentation and Loom

Repository pages own current state. GOAL → current plan → STATUS → core designs
and closing report is the navigation path. Loom mirrors these, with the top-five
plan and benchmark tables linked from the report. Six old one-revision artifacts
are preserved in `archive/20261109/loom-artifacts.json` before active removal;
current goal/plan/report histories remain intact. Earlier goal, handoff and
PyValue pages are archived, not competing active instructions.

## Preserved unrelated work

Never stage/reset/delete these preexisting canonical changes:

- pyjit/bench/results/checkpoint-6a0e4c12c17.md, empty; SHA256
  e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855.
- pyjit/issues/checkpoint-closure/ctypes-rawpin-build.log, four appended lines;
  SHA256 f37dca15ab40ec8eb18814673f351c0b545698b22611c0c025b64b4c196bb6fe.
- Untracked empty pyjit/issues/checkpoint-closure/w5-release.lock.

The prior [handoff](archive/20261109/HANDOFF-before-round-closure.md) preserves
exact historical state. Use current Git, processes and evidence when resuming.
