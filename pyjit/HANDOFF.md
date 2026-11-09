# Coordinator handoff

## Current goal: frame/call model and logging parity

Continue until unchanged JIT-off logging reaches matched upstream parity and
broad regressions are repaired. Follow Python's data model; source boundaries
are metadata. The [active plan](issues/frame-call-model-20261109/plan.md) owns
admission. Golden remains immutable and selected. The goal tool is active,
without a token budget.

Runtime source:
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/execution-boundaries-20261108`,
branch `pyjit/frame-call-model-20261109`, committed engineering HEAD
`c1424be11db46b5d9e2aca84bcc96ddad6a7ea62`. Live next-packet edits are admitted
only under explicit worker file ownership. Root alone regenerates, builds,
tests, commits and measures after freezing source. Cache magic is 64416.
Canonical documentation stays in this checkout; runtime worktree copies are
historical. Build/evidence paths below are relative to python-boil-the-ocean.

Candidate release/debug builds `build/execution-boundaries-{release,debug}`
identify c142 and check 117 modules. Stats still identifies192 and needs a
current rebuild. Evidence: `build/pyjit-link-wave1/frame-call-model-20261109/`.
The [sixth report](issues/frame-call-model-20261109/sixth-integration-report.md)
and [repair log](issues/frame-call-model-20261109/qualification-repairs.md)
record exact scopes and failed attempts.

### Executed checks and limits

- Native29 debug: 363 tests across six core modules, one skip, all pass.
  Release selected gate:22 pass. Eight additional observer/frame modules:
  64 tests pass per profile. Owner probes leave zero survivors after collection
  from660 values, and zero from three FOR items on both normal/error paths.
- Dependency-correct release accepts2,156 valid library files;11 intentionally
  invalid parser fixtures remain SyntaxError. Missing Makefile dependencies on
  four IR headers are fixed; retain the earlier stale-object v2 failure.
- Native29 wide release:999 tests,14 skips, four failed modules. Monitoring has
  one error; peepholer three; sys_settrace38; pdb two failures/two errors.
  The21 obsolete diagnostic mismatches are gone. Last full release remains87:
  54,504 tests,13 failed modules,126 failures and99 errors. Full release/debug,
  current stats and306-unit API lint remain open.
- c142 is independently source/result reviewed. Its five canonical design
  pages are synchronized; two changed Mermaid diagrams rendered and inspected
  under `design-diagrams29/`. Historical qualification does not transfer to live
  worker edits.

### Current measurements and assignments

Clean c142, GCC15.2, JIT0, CPU15: two balanced pairs logging/upstream0.936868;
separate golden logging0.653035,nbody1.041296,pickle1.026258. Four-pair repeat:
nbody1.019904,pickle1.021510. All pre/post identities stable. Final logging needs
two independent balanced six-round campaigns, mean at most1.0 and every pair
at most1.02, after coherent qualification.

Current warmed PMU: logging/upstream cycles0.918210,instructions1.044970;
nbody/golden cycles1.040876,instructions1.033309. Logging18/nbody12 stat captures
are valid at100%event running. Both cycle-record comparisons report zero lost
samples. `sixth-perf-input-postflight.json` confirms401 logging inputs unchanged.
Use the retained native profiles for location evidence; no old-image timing or
handler census supplies current causal attribution. Launched perf works at
kernel level2; independent worker attach was denied and GDB is unavailable.

Reconcile actual Sol6.1 medium agent states after restart:

- A (`boundary_design_impl`): admitted producer increment in bytecodes,
  generator, instrumentation and provider tests: four-argument explicit owner
  edge marker, positive source-publication argument, physical-next threading,
  terminal JUMP/POP_ITER. Host evidence changes only these three providers,
  preserving prior ten observed and262 ordinary bodies. Actual source review
  and merged native gate pending. Yield is separate, not admitted yet.
- B (`boundary_cold_review`): actual debugger failure diagnosis and revised
  shared stopped-state proposal. Root executed84 capped cold/warm probe rows:
  list/set/dict ordinals2/3/4 stop at pc9 INSTRUMENTED_JUMP_BACKWARD and refuse;
  ordinal1/5 real LINE stops commit. All six unchanged fixtures have zero source
  records. Their line callbacks run under JUMP monitoring, not PRE LINE.
  A's certified JUMP outcome supplies a concrete POST consumer witness. Peer
  and root admission precede consumer edits; no broad event-name inference.
- H (`holistic_assessment`): independent schema/source/result reviews and design
  synchronization complete. Current performance review ranks audited canonical
  home duplication, float path and fixed call binding. A bounded canonical-home
  producer proposal is requested; no optimization source edits admitted yet.
- Root: integration, native gates, quiet performance, final full/broad checks,
  profiling and canonical/Loom state. Workers never commit or run native jobs.

Next: complete and review the coherent producer/consumer packet, focused native
release/debug checks, commit/basic performance, then coherent full suites,
current stats/API, unchanged29-workload screen with material-loss repeats, final
logging campaigns and cold review. Retained roots, caught exits, true yield and
remaining completion providers need explicit semantic contracts. Preserve sealed
controls/evidence and unrelated local work listed below.

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
