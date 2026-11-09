# Coordinator handoff

## Current goal: frame/call model and logging parity

Continue until unchanged JIT-off logging reaches matched upstream parity and
broad regressions are repaired. Follow Python's data model; source boundaries
are metadata, not empty executable operations. The active
[plan](issues/frame-call-model-20261109/plan.md) owns admission. Golden remains
immutable and selected. The goal tool is active, without a token budget.

Runtime source is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/execution-boundaries-20261108`,
branch `pyjit/frame-call-model-20261109`, clean engineering HEAD
`192cfbe8aa0d0b593dcc40f26e1c8f70e84b02ae`. Cache magic is 64416. Canonical docs
are in this coordinator checkout. Candidate builds are
`build/execution-boundaries-{release,debug,stats}`; current evidence is
`build/pyjit-link-wave1/frame-call-model-20261109/`, relative to python-boil-the-ocean.

The integrated model includes overlapping owning call frames, lazy kwargs,
compact words with sparse source edges, semantic loop/send/throw completion,
initialized arena frontiers, activation-local source errors, explicit debugger
entry and stdlib consumers, cold context/owner transfer plans, completion payload
identity and a narrow legacy adapter. The latest commit narrows LINE invalidation
to real LINE changes, reused tool identity and restart. It resets stale LINE
DISABLE masks without replaying LINE for an INSTRUCTION-only change.

Read the [frame ABI](design/call-and-frame-lifetimes.md),
[debugger transfer design](design/debugger-frame-transfers.md) and
[repair log](issues/frame-call-model-20261109/qualification-repairs.md).
The latter owns exact source/build/evidence identities and all retained failures.

### Executed checks and limits

- `37a67b9d155`: fused source coverage, exact real-LINE identity and format/layout/
  frozen-embedding repairs. `ownership24-*` checks 117 modules and passes 1,027
  tests per profile; 2,152 valid library files compile without rejection.
- `9f677103e00`: entry consumers, shared EH decoder, inherited/same-caught plans,
  payload/legacy repair. `ownership26-small-*` passes 131 tests per profile;
  fifty payload probe rows each have zero identity mismatches. The corrected
  `test_profiling.test_tracing_profiler` gate passes 28 each. The first wide driver
  mistakenly selected obsolete test_cprofile; keep that import failure as a
  harness error. All 2,154 valid library files compile; eleven intentionally
  invalid parser fixtures remain SyntaxError.
- `192cfbe8aa0`: `ownership27-*` checks 117 modules and passes 21 related plus
  51 existing tests per profile. The faithful unchanged class-init probe passes
  both base and opcode variants on both builds; previously the opcode case failed.
- The last wide check belongs to native26: 1,572 tests per profile, with four
  actual failed modules (monitoring, pdb, peepholer, sys_settrace) plus the bad
  selector above. Trace reports 65 errors, 21 diagnostic mismatches and the
  now-repaired epoch duplicate. Pdb has six failures/two errors across three
  repeated backend cases. Do not relabel these as current full qualification.
- The last full release diagnostic belongs to measured `87f13087ed9`: 54,504
  tests, 13 failed modules, 126 failures and 99 errors. No final release/debug
  qualification or performance promotion. No new external cold reviewer could
  be spawned: thread limit persists; the two workers provide reciprocal reviews.

All three candidate build identities now match clean committed `192cfbe8aa0`.
`fourth-engineering-{release,debug}-identity.log` each checks 117 modules; stats
build/smoke passes 146 tests. Root serializes native builds/tests and quiet
measurements. Workers do not run native jobs.

### Performance and next execution

Clean `192cfbe8aa0`, GCC 15.2, JIT off, CPU 15. The matched two-pair
logging/upstream screen is 0.817460. The four-pair golden repeat has logging
0.592805, nbody 1.000054 and pickle 1.017928. Nbody's earlier 87-image 5.6% loss
does not repeat; pickle varies 0.997762–1.039830. Do not infer a cold-repair gain
or combine ratios across controls/campaigns. Final parity needs two independent
six-round logging/upstream campaigns after coherent qualification.

`fourth-nbody-census-v2` has stable selected routes before/after all six 0/2/4
captures. Warm twenty separate run(1) calls: the first census's single run(20)
left setup specialization active. All material counts match except removed
POP_ITER (40,005/outer) and POP_TOP (2/outer): 17,600,702 vs 17,560,695 entered
handlers. POWER adaptive retry counts are nonlinear in both controls; retain that
limit. These are stats counters, not native instructions or timings. Older 87 PMU
and flat profile limits remain in the third report; no new hot optimization is
admitted from them.

The compile-only thirteen-case cancellation inventory succeeds under 192:
`ownership28-cancellation-inventory/inventory.json`. B now implements the
peer/root-admitted bounded PRE cancellation with derived positively empty saved-
local contexts. Origin loss keeps an obligation active; only matching TOP
restoration pops it; caught implicit roots remain preserved. A prepares a scoped
cold POST phase proposal and independently reviews B's eventual source. Runtime
freeze is released after the completed screen/census. Root alone builds/tests.

Caught exit needs a separately proved target restoration link. Matched upstream
clobbers a callback-owned exception in this case; do not reproduce that behavior.
Protected FOR, source-only destinations, generator phases and comprehension
restoration remain open. Remote/GDB source-cursor display and semantic owner
retirement also remain. Normal thread clear asserts scratch NULL; a proposed
free terminal cleanup was rejected. No retained-owner bank is admitted.

After coherent repairs, qualify full release/debug, rebuild stats, run API lint,
the unchanged 29-workload screen and final logging campaigns. Repair material
regressions/findings before promotion. Seven canonical diagrams now include the
new rendered/inspected debugger transaction; six prior diagrams are retained.
Preserve the three unrelated canonical changes listed below and sealed evidence.

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

Build and minimally validate the peer-approved identity/format repairs, then
continue the coherent context and source-event/debugger-entry work. Capture
actual final IR and observer phases before implementation; do not weaken old
behavior expectations or synthesize empty executable instructions. Keep shared
cold operand/outcome contracts and no per-dispatch liveness map. Recheck nbody
with dynamic work/compiled-path evidence before another performance change.

After coherent repairs, run full release/debug qualification, API lint, a stats
rebuild, the 29-workload screen and two independent six-round logging/upstream
campaigns. Repair material regressions and review findings before promotion.
Keep golden source/builds/tags and sealed evidence immutable. Use GCC, JIT off,
CPU15, sanitized native environments and uv host drivers. Canonical docs and
Loom artifacts must follow actual source and executed evidence.

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
