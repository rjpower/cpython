# Coordinator handoff

## Current goal: frame/call model and logging parity

User direction on 2026-11-09 supersedes the previous bounded-ablation plan:
continue until unchanged JIT-off logging reaches upstream parity and the broad
regressions are repaired. Follow Python's data model. The active
[plan](issues/frame-call-model-20261109/plan.md), frame/call proposal, revised
IR-position proposal and holistic review live in that packet. Loom goal/plan
started at revisions 8/18 at the initial reviewed-implementation milestone.

Continue the existing runtime checkout
`build/worktrees/execution-boundaries-20261108`, now branch
`pyjit/frame-call-model-20261109` from `4b3d739d867`. First engineering source is
committed at `6ef45e391aa8462be3d165d8d9056e4e06e0a634`.
The release ELF was rebuilt for that identity, checking 117 modules. It is not
qualified: the 22-module focus runs 2,694 tests, 18 modules pass; useful debugger
resumes and tight-loop LINE duplication remain open. The subsequent nine-module
1,009-test check passes GC, weakref, finalization, UUID, frame-call, boundary
and disposal modules. Source positions, fixtures and observed delegation crashes
are repaired. See `merge8-integrated-release-focus.log` and
`merge9-focused-release-focus.log`; no full qualification has run this packet.

Canonical documentation remains in this checkout. Evidence is
`build/pyjit-link-wave1/frame-call-model-20261109/`. Release/debug/stats candidate
builds still target the existing source; debug/stats have not been rebuilt this
packet. Golden/upstream builds remain untouched. First clean-source measurements:
`first-engineering-upstream/` gives logging 1.003646 (two rounds),
`first-engineering-golden-v2/` gives logging 0.742426 and unpack 0.833936, while
other broad losses remain. `first-perf-{stat,record}/`, `first-perf-summary.json`
and `first-profile/` retain the current counters/profile and metadata warnings.
The first golden attempt supplied a nonexistent module-log path and was rejected
before timing; preserve it. The [integration report](issues/frame-call-model-20261109/integration-report.md)
owns interpretation and limits; near parity is not the required final pass.

Sol 6.1 medium A/B workers are implementing the independently reviewed
`semantic-completion-proposal.md`. A owns runtime/generator/header/frame phases;
B owns compiler/assembly/outcomes, observers/masks/validation and bounded debugger
recipes. They agree shared interfaces first; root owns regeneration, builds,
merged checks, commits and timing. Runtime freeze was lifted after all first
captures completed; subsequent uncommitted edits are next-increment work, not
inputs to the retained 6ef measurements. B's tight-loop legacy tracing fix is
source-only until rebuilt. Shared exception/branch owner retirement and useful
protected/suspended debugger recipes remain open. No retained-owner bank has
been admitted. Preserve the three unrelated canonical changes below.

The revised design has one public compact instruction-offset domain. Removed
frontend bookkeeping creates no synthetic instruction events or recovery slots.
Sparse source-edge line metadata preserves source tracing; ordinary execution
does not interpret it. Lazy kwargs use a canonical NULL home plus explicit
pending metadata, compiler first-read materialization and frame-publication
barriers. Rebinding/captured/suspending cases initially use eager binding.

The task goal tool is active, without a requested token budget. Do not end at
implementation or a partial speedup: the measured parity and regression gates
in the plan remain required. Preserve the three unrelated canonical changes
listed in the previous packet's protection snapshot.

The 2026-11-08 golden runtime is published. Read [STATUS](STATUS.md), the
[manifest](issues/golden-20261108/README.md), [current IR](design/register-ir.md),
[interpreter](design/interpreter.md) and [next plan](issues/golden-20261108/next-steps.md).
The previous 435-line board is [archived](archive/20261108/HANDOFF.md).
Do not recover active assignments from its historical instructions.

## Previous qualified candidate

The [execution-boundary packet](issues/execution-boundaries-20261108/plan.md)
is integrated at `4b3d739d867b06ea4c9ed936982cfeb3503e93be`. Its source is
`build/worktrees/execution-boundaries-20261108`, branch
`pyjit/execution-boundaries-20261108`; candidate builds are
`build/execution-boundaries-{release,debug,stats}`. Golden source and builds
below remain unchanged controls. Source proposals and reciprocal reviews are
in the packet. The coordinator owns native validation and commits; workers
use Sol 6.1 medium per the user's latest instruction. Both component worktrees
were retired after preserving their complete patches and identities in
`component-source-archive.json`; the integration source and controls remain.

Candidate evidence lives in `build/pyjit-link-wave1/execution-boundaries-20261108/`.
Build preparation initially omitted the development-library pkg-config sysroot;
the retained retry restores it and verifies effective flags and pyconfig match
golden. A retained early focus attempt used the old runner's golden executable;
the runner now asserts candidate executable and stdlib paths before testing.
Operand increment `f2c5ac3052b` passed 465 focused tests per profile. Combined
increment `5bfd1e7e457` passed its repaired boundary/contract checks, 40 tests per
profile. The first full release run found the cache/table pin and one hidden-array
GC-verifier failure. Cache magic is now 64411; `4b3d739d867` also fixes the missing
import.o/header dependency that initially left a stale native token. The import/GC
focus passed 155 tests per profile, stats smoke 142 and API lint 306 units with
zero failing findings. Final release full qualification passed 54,421 tests,
zero failures. Debug's full run completed 54,421 tests with one profiling failure:
`ProfileTest.test_calling_conventions` missed the starred-keyword max call and
its exec-body entry. Two isolated pure/cProfile method runs each on golden and
candidate, release and debug, all passed. Matched whole profiling-module controls
and 24,000 actual shared-code profiling statements across those roles also
passed. No runtime repair followed and the cause remains unexplained. The debug
full retry passed 54,421 tests in 10m23s, with explicit seed 1284068202 and
randomized test order; the original run used default order. Retain the failure
as an unresolved transient, not a demonstrated repair. Final elapsed campaigns
are complete: logging/golden 1.00142, logging/upstream 1.39308; 29-workload
geomeans 1.00428/1.24457 respectively. Unpack, pickle, float, deepcopy_memo and
nbody regressions repeat over four more rounds; spectral's 16% improvement also
repeats. Performance promotion is withheld. Golden remains selected; this clean
reviewed source branch is a qualified candidate, published to origin.
Final pinned diagnostics are `perf-{stat,record,empty}-v2`: all pass, counters
100% running; instructions/upstream 1.40283, cycles 1.39877, branches 1.52614,
branch misses 1.00045. The first unpinned attempt was rejected before the hot
region. Profiles have zero lost samples and a retained HEADER_EVENT_DESC warning.
`perf-identities/input-hashes.json` remains valid. See `elapsed-summary.json`,
`perf-summary.json`, `profile-summary.json` and original captures for evidence.
See the [report](issues/execution-boundaries-20261108/report.md) and
[next plan](issues/execution-boundaries-20261108/next-steps.md).
Do not relabel earlier failed or wrong-executable receipts as candidate passes.

Logging's warmed census is 142 entered handlers versus golden 151, zero POP
entries, with 20 frame entries unchanged and linear empty/N/2N deltas. This is
executed-work evidence, not an elapsed speedup. The operand-only screen did not
improve logging; combined measurements establish no logging gain. Canonical
specifications describe the reviewed candidate, not a newly promoted golden.

Treat [language and lifetime policy](design/lifetimes-and-language-semantics.md)
as the correction to overly broad finalizer-timing constraints in historical
reviews. The golden return protocol is an implementation description, not a
Python language mandate. Preserve supported observation APIs and owner/GC safety.

## Golden source and evidence

- Canonical documentation: `/home/power/code/cpython/.worktrees/python-jit`,
  branch `weaver/python-jit`. This checkout's CPython implementation is not the
  measured runtime. Use the published combined golden snapshot for a coherent
  source/documentation checkout; see the manifest.
- Runtime: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/pyvalue-rc-universal`,
  branch `boil/pyvalue-rc-universal`, clean HEAD `0765e9be47f55d97ba5d31d2ae1c02a55ea2e5ae`.
  Origin has this branch and immutable tag `pyjit-golden-20261108-runtime`.
  Runtime implementation is frozen. Its old `boil/` documentation is historical;
  the combined publication replaces it with canonical `pyjit/` documentation.
- Golden functional qualification and its final timing captures: `0267d428808`.
  Runtime successor `0765e9be47f` moves one existing comment. Debug `.text`
  matches exactly. Release differs only in four address-relocation bytes for
  equivalent `f` literals caused by build-identifier suffix merging. Do not
  claim whole ELF identity. The exact measured ELF is retained.
- Build root: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
  Builds: `pyvalue-rc-repr-{release,debug,stats}`. Current binaries identify
  `0765e9be47f`, GCC 15.2.0, JIT off, 117 modules each; rebuilt stats smoke
  passed 142 tests. Matched upstream: `upstream-release-devlibs-3a440b6`
  at `3a440b65695`; historical interim: `eba351fee7d`.
- Evidence under build root: `pyjit-link-wave1/interpreter-parity-20261108/`.
  Its `run-checks.py`, `run-performance.py` and receipts contain exact commands.
  Final captures include `final-qualified-*`, `comment-cleanup-*`,
  `final-target-{1,2}`, `final-broad`, `final-vs-r5`, `final-perf-*`,
  `spectral-perf-*`, `profile-analysis` and `warmed-shape-comparison`.
  Retain failures and do not reuse output names. The
  [report](bench/results/interpreter-parity-20261108.md) records evidence limits.
- Temporary route/hit counters were retired byte-for-byte; stats was rebuilt
  after restoring the header layout. Current runtime has no diagnostic overlay.
  R5's ELF remains at `baseline-identities/candidate-python` under the evidence
  root. Its old build-local control copy and the prior task worktree were
  removed; recreate that copy before reusing the direct-comparison launcher.

## Next execution

Start with the candidate's [report](issues/execution-boundaries-20261108/report.md)
and [next plan](issues/execution-boundaries-20261108/next-steps.md): review a
bounded component ablation and build the call/ownership cost ledger before
selecting another runtime change. The golden review remains historical input.
Calls use frame stacking/overlap; logging has
no consuming-binder fallback and its hot callees have zero constant homes.
Its retained message is mortal. Do not repeat obsolete argument-copy or
constant-home hypotheses. Keep spectral in integration screens. Its CPI
regression has no established cause.

Use Sol 6.1 medium for this packet, per the latest user direction. Workers do not commit. The
coordinator owns integration, native builds/tests and quiet measurements. Use
`uv run` for host Python drivers, sanitize `PYTHON*`/`LD_PRELOAD` before native
children, and use the existing shared caches. One compiler: GCC. JIT off must
be checked in native children, including the fork's own method-JIT admission.

## Preserved local work

These canonical changes predate this checkpoint and remain outside its commits:
emptied `bench/results/checkpoint-6a0e4c12c17.md`, appended
`issues/checkpoint-closure/ctypes-rawpin-build.log`, and untracked
`issues/checkpoint-closure/w5-release.lock`. The published tree uses their
committed content. Do not stage, reset or delete them incidentally.

The [cleanup audit](issues/golden-20261108/cleanup-audit.md) found dirty legacy
worktrees, cited ignored evidence and an active bulk-build source dependency.
Keep those paths; clean Git status alone does not make a worktree disposable.
Historical branches, ABI and build names retain their identities. Canonical
docs own current state; Loom artifacts mirror them and retain prior revisions.
