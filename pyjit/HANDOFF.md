# Coordinator handoff

## Current goal: frame/call model and logging parity

Continue until unchanged JIT-off logging reaches matched upstream parity and
broad regressions are repaired. Follow Python's data model; source boundaries
are metadata. The active [plan](issues/frame-call-model-20261109/plan.md) owns
admission. Golden remains immutable and selected. The goal tool is active,
without a token budget.

Runtime source is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/execution-boundaries-20261108`,
branch `pyjit/frame-call-model-20261109`, committed engineering HEAD
`4a1f70fcef49bccf210180fa69cfc4fb409a80cd`. It now has live uncommitted worker
changes. Do not build, regenerate or measure until all workers finish and root
freezes source. Cache magic is 64416. Canonical docs are in this coordinator
checkout. Candidate builds are `build/execution-boundaries-{release,debug,stats}`;
current evidence is `build/pyjit-link-wave1/frame-call-model-20261109/`, relative
to python-boil-the-ocean. Release/debug identify clean 4a; stats still identifies
192. Workers never run shared native jobs or actual regeneration.

The integrated model includes overlapping owning call frames, lazy kwargs,
compact words with sparse source edges, semantic loop/send/throw completion,
initialized arena frontiers, activation-local source errors, explicit debugger
entry, bounded context/owner plans, per-named-local save obligations and PRE
cancellation. Same real-LINE identity preserves the current PC and phase,
including a later with-exit operation on the header's line.

Read the [frame ABI](design/call-and-frame-lifetimes.md),
[debugger transfer design](design/debugger-frame-transfers.md),
[fifth report](issues/frame-call-model-20261109/fifth-integration-report.md) and
[repair log](issues/frame-call-model-20261109/qualification-repairs.md).

### Executed checks and limits

- 4a release/debug each check 117 modules and pass 125 focused tests with one
  skip. Identity/large-window checks pass two tests each. The latter ports an
  obsolete capacity expectation into one successful 101-argument call.
- The last wide check, before that test-only port, runs 999 tests per profile.
  Four modules fail: monitoring, pdb, peepholer and sys_settrace. Trace errors
  fall from 65 to 38; 21 diagnostic assertions and the now-ported capacity
  expectation also fail. Pdb has two failures/two errors. Do not call this full
  qualification. The last full release remains 87: 54,504 tests, 13 failed
  modules, 126 failures and 99 errors.
- Native28's first global save-stack proof rejected independent local restores;
  the committed per-local model fixes it. Retain that failure and two fixture
  retries: code.replace requires distinct equal co_linetable bytes, not the
  same object. The first wide check hung by restarting an earlier operation on
  the same with-header line. Root captured SIGUSR1 faulthandler output, stopped
  only its worker, and fixed identity. The bounded probe has 16 passing rows
  per profile with no cap, errors or nonidentity redirects.
- The corrected tracing-profiler module passes 28 tests per profile at26;
  the obsolete test_cprofile selector was a harness error. Compile corpus at26
  accepts 2,154 valid files; eleven invalid parser fixtures remain SyntaxError.
  Stats192 passes 146 smoke tests. API lint and current full qualification remain
  open; do not transfer these old passes to live worker source.

### Performance and live assignments

Clean4a, GCC15.2, JIT0, CPU15: two balanced pairs give logging/upstream
0.838496 (0.812193, 0.865650). The separate two-pair golden campaign has logging
0.585975, nbody0.999512, pickle1.014001. Stable identities hold pre/post. Nbody's
old87 loss does not repeat. Do not combine campaigns or infer cold-fix causality.
Final parity needs two independent balanced six-round campaigns after coherent
qualification, mean at most1.0 and every pair at most1.02.

All three Sol6.1 medium workers are active; the earlier third-agent thread-limit
problem was resolved. Reconcile actual agent states after restart:

- A (`boundary_design_impl`): generator terminal output publication,
  activation-owned observer scope, exact-frame source-error invalidation,
  instrumentation and private entry masking. Owns generator/bytecodes/ceval/
  frame.c/instrumentation/sysmodule and focused observer tests. No Clinic
  signature change. Finite composed/direct provider census must be migrated
  explicitly; a provider cannot advertise POST before all its outputs commit.
- B (`boundary_cold_review`): frameobject saved-local restore actions and
  test_code. Root read the actual draft; A source peer and native checks remain.
  Full-exit PRE only, same handled context, ordinary locals, available acyclic
  saved generations. Source POST/yield/context-exit require separate proof.
- H (`holistic_assessment`): constructor geometry and shared cold output proof,
  codeobject/header/test_ir under A's exact enum interface agreement. The direct
  geometry application was initially rejected by automatic review for missing
  root admission. After actual root/A/B patch review and explicit source/test
  admission, the same direct tool succeeded; no workaround was used. Broader
  POST schema/worklist is separate and must compose with A's generated facts.
- Root: integration source review, actual regeneration, native release/debug,
  focused tests, post-review, commits, quiet screens, full/broad/final gates,
  canonical/Loom state. No worker commits or native jobs.

The [cold review](issues/frame-call-model-20261109/cold-checkpoint28-review.md)
has eight ranked actions, mapped to these streams in the fifth report. Ordinary
frame-exit scratch ownership remains an explicit integration audit. The FOR
observer-error probe retains three unpublished results after collection on both
candidate profiles and upstream. Do not copy upstream's bug. A's terminal
transaction must publish state before callbacks, preserve callback exceptions
through closes, and never overwrite a newer redirect.

The stable192 nbody census warms20 separate run(1) calls and captures0/2/4.
All material routes match except removed POP_ITER40,005/outer and POP_TOP2/outer;
17,600,702 vs17,560,695 entered handlers. POWER retry counts are nonlinear on both
roles. This is not native-cost evidence. Earlier PMU belongs to87 and supports
flat locations only despite zero lost samples. Current-image profiling follows
coherent correctness. Perf attach to a separate trace worker was denied at
kernel level2, whereas the existing launched perf protocol works; no GDB is
installed. Do not ask for the same perf setup again.

Remaining: source-aware remote/GDB cursor display, protected owner/context
transfers, complete terminal provider census, frame-exit audit, full release/debug,
current stats/API lint, unchanged29-workload screen with repeated material losses,
both final logging campaigns and fresh cold source/result review. Preserve seven
canonical diagrams (new debugger transaction rendered/inspected; retained blocks
unchanged), golden controls, sealed evidence and the unrelated paths below.

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

The current-goal section above owns executable next steps. Keep golden source,
builds, tags and sealed evidence immutable. Use GCC, JIT off, CPU 15, sanitized
native environments and uv host drivers. Canonical docs and Loom artifacts
follow actual source and executed evidence.

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
