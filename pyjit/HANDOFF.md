# Coordinator handoff

## Closed checkpoint

Runtime `4d666e1b482cb4983c7683f91c5922af14d0ab40`, branch
`pyjit/frame-call-model-20261109`, is clean and frozen. Production changes are
`24f4d270ced`; the final commit adds reviewed debugger tests. No unfinished
coroutine, owner-bundle or GDB port is included. No native job remains queued.
The next round starts from [the five priorities](issues/frame-call-model-20261109/next-steps.md),
not the older numbered integration reports or unqualified drafts.

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
| Candidate | worktrees/execution-boundaries-20261108 | execution-boundaries-{release,debug,stats} | 4d666e1b482cb4983c7683f91c5922af14d0ab40 |
| Golden | worktrees/pyvalue-rc-universal | pyvalue-rc-repr-{release,debug,stats} | 0765e9be47f55d97ba5d31d2ae1c02a55ea2e5ae |
| Upstream | worktrees/upstream-3a440b6 | upstream-release-devlibs-3a440b6 | 3a440b65695615e9fd9912342b79cdd54fced0d2 |

Golden and upstream source/builds remain immutable. Golden is published as
`pyjit-golden-20261108-runtime`; no new promotion is implied. Candidate magic is
64416; all three builds identify the current full source and check 117 modules.
Upstream checks 116 modules. Physical DSO counts 78 upstream/213 value builds
include historical ABI files and are not checked-module counts.

Root owns native execution, builds, commits and measurements. Existing launchers:
`issues/frame-call-model-20261109/run-checks.py`, the preceding packet's
`measure.py`, and the closing supplemental/PMU scripts. Read their argument
requirements and preserve output directories. Host Python uses
`UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B`.
Measurements use GCC 15.2, CPU 15, JIT off, hash 0 and fixed inputs. No source/build
or benchmark mutation during timing. Full suites belong at coherent checkpoints.

## Evidence and deferred source

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
