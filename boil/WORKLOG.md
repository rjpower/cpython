# Worklog

Append concise, high-level actions and outcomes. Link issue results for detail.

- 2026-10-31: GC cleanup and scanning are landed and measured; the matched
  29-workload result is 0.871038× upstream throughput.
  [Results](issues/gc-cleanup-scanning/performance.md).
- 2026-10-31: Documentation is organized around current contracts and issue-owned
  work. Bounded nursery and interpreter-cost experiments are the next tracks.
  [Work index](issues/README.md).
- 2026-10-31: Started parallel minimal young-collection and frame-transfer
  experiments. Existing-profile attribution selected the shared frame-copy
  boundary without another benchmark run. [Current work](issues/README.md).
- 2026-10-31: Rejected bulk frame transfer after bounded comparison found no
  repeatable benefit. Selected shared shape admission next; minimal young
  collection passed source review and moved to feedback.
  [Current work](issues/README.md).
- 2026-10-31: Minimal young collection passed its first behavior checks and
  independent outcome review; bounded compatibility checks continue. Shared
  polymorphic shape admission moved to build feedback. [Work index](issues/README.md).
- 2026-10-31: Accepted the minimal nursery hypothesis after bounded behavior,
  compatibility and negative controls; selected the opt-in candidate for
  qualification. Shape admission passed behavior/emission feedback and moved
  to independent review. [Work index](issues/README.md).
- 2026-10-31: Selected the exact nursery/shape combination for one reviewed,
  finite qualification batch. Started parallel call-setup attribution and
  completed benchmark documentation consolidation. [Work index](issues/README.md).
- 2026-10-31: Combined GCC builds and early nursery/shape controls passed;
  sanitizer and tail builds followed. Closed call-setup attribution without
  selecting an interface rewrite. [Work index](issues/README.md).
- 2026-10-31: Resumed after machine restart. Restored missing tracked primary
  files and worktree metadata without replacing survivors; retained four-build,
  backend-control and release-full evidence. Recorded the expired qualification
  attempt and bounded only its remaining checks; independent carry review and
  recovery of missing uncommitted notes continue.
  [Evidence and recovery](issues/nursery-shape-landing/results.md).
- 2026-10-31: Independently accepted the exact nursery/shape source union and
  started remaining-only qualification after restart input/cleanup checks.
  Accepted a reviewed next experiment for exact native coroutines, with
  separate admission/cleanup attribution and explicit rejection gates;
  implementation waits for the shared landing.
  [Current qualification](issues/nursery-shape-landing/status.md),
  [next design](issues/gc-nursery-coverage/status.md).
- 2026-10-31: Recovered every pre-existing user note lost with the primary
  checkout. One exact Git blob and four task-specific patch replays match all
  five preserved file hashes; their original modified/untracked state is
  retained outside project commits.
  [Recovery evidence](issues/nursery-shape-landing/reviews/restart-user-note-recovery.md).
- 2026-10-31: The exact nursery/shape union passed release, debug OFF and debug
  KILL full suites with zero unwind fallbacks. Remaining stress, sanitizer and
  closure gates continue within the separately bounded restart batch.
  [Qualification evidence](issues/nursery-shape-landing/results.md).
- 2026-10-31: Independent final review accepted the exact nursery/shape union
  after all required carried and refreshed qualification, sanitizer, closure
  and cleanup gates. The 334-artifact capsule and six validator replays passed
  within the restart batch deadline. Integration and post-merge smokes follow.
  [Acceptance](issues/nursery-shape-landing/reviews/final-qualification-20261031.md).
- 2026-10-31: Integrated the exact qualified nursery/shape source union and
  updated the runtime contracts. The nursery remains private and opt-in;
  actual merge binding and the reviewed release/debug smoke pair close this
  landing. No new performance result is claimed.
  [Landing status](issues/nursery-shape-landing/status.md).
- 2026-10-31: Closed nursery/shape landing at d98291018c9 after independent
  exact-merge and release/debug smoke review. Both smokes returned 1,937 tests,
  zero unwind fallbacks and verified cleanup. User direction now keeps the
  nursery architecture and removes its temporary whole-runtime switch;
  adoption/checker design and constant-operand IR research run in parallel.
  [Landing closure](issues/nursery-shape-landing/reviews/final-smoke-20261031.md),
  [adoption](issues/gc-nursery-adoption/status.md).
- 2026-10-31: Independent design review cleared mandatory nursery admission,
  real checked minors and removal of the global switch; automatic cadence is
  a separate measured increment. Native IR inspection selected a small-integer
  RHS operand form for a bounded host-only prototype. Both designs proceed
  to isolated source work, with SSA/LuaJIT comparison running alongside them.
  [GC review](issues/gc-nursery-adoption/reviews/nursery-adoption-design-20261031.md),
  [IR review](issues/interpreter-constant-operands/reviews/design-20261031.md).
- 2026-10-31: Dispatched isolated nursery adoption and constant-operand source
  work with finite checkpoints. Source research confirmed regional single
  definitions, local folds and interblock liveness, without a general SSA
  optimizer. LuaJIT's baseline literal-operand staging supports the current
  arithmetic prototype and a conditional subscript follow-up; its trace
  optimizations do not establish a missing interpreter pipeline.
  [Research](issues/interpreter-lowering-research/results.md),
  [current work](issues/README.md).
- 2026-10-31: Froze nursery adoption candidate b34931a881b after host syntax,
  root and traversal checks, with twenty focused tests prepared. Independent
  source review found and resolved retry-backoff and private-tuple diagnostic
  lifetime cases before freeze. Exact-source review and held debug feedback
  follow; no native pass or landing is claimed.
  [Adoption status](issues/gc-nursery-adoption/status.md),
  [feedback scope](issues/gc-nursery-adoption/feedback-plan.md).
- 2026-10-31: Independent source review accepted nursery b34931a881b for
  bounded native feedback. Froze IR operand candidate 7f0b1819f84 with shared
  handlers/metadata, eleven target tests and passing host controls; its exact
  source review continues. Both launch preparations remain held for binding
  and review, with no native or performance result claimed.
  [GC source review](issues/gc-nursery-adoption/reviews/source-20261031-v1.md),
  [IR status](issues/interpreter-constant-operands/status.md).
- 2026-10-31: Independent exact-source review accepted IR 7f0b1819f84 for
  bounded native feedback after host replay and generated/root consistency
  checks. Preselected the existing recursive-coroutine workload for a held
  performance decision; source inventory identifies its literal subtraction
  sites without claiming compiler or native evidence. Both tracks prepare
  reviewed launch bindings before execution.
  [IR review](issues/interpreter-constant-operands/reviews/source-20261031-v1.md),
  [held measurement design](issues/interpreter-constant-operands/performance-plan.md).
- 2026-10-31: The first nursery feedback launch stopped before configuration:
  readiness and build recorded different assembler path aliases for identical
  binaries. Preserved the failure and verified idle cleanup; bounded a helper
  correction under the original batch deadline. No native build/test credit.
  Independent review accepted the revised held IR measurement proposal after
  requiring original low/high counter capture.
  [GC feedback](issues/gc-nursery-adoption/feedback-results.md),
  [IR measurement review](issues/interpreter-constant-operands/reviews/performance-plan-20261031.md).
- 2026-10-31: Independent review accepted the nursery helper correction; the
  remaining-only launch passed tool identity and began configuration under
  the original deadline. Accepted the frozen IR feedback helpers after host
  replay and queued their activation behind the GC module window. Native
  behavior and performance remain pending.
  [GC execution record](issues/gc-nursery-adoption/feedback-results.md),
  [IR helper review](issues/interpreter-constant-operands/reviews/feedback-helpers-20261031-v1.md).
- 2026-10-31: Nursery debug build passed; its first twenty-method selection
  returned eighteen passes and two private-tuple diagnostic-record failures.
  Stopped the batch, verified cleanup and retained both failures for provenance
  diagnosis. Activated the reviewed IR native batch while GC analysis continues
  without another GC invocation.
  [GC failures](issues/gc-nursery-adoption/feedback-results.md),
  [IR execution](issues/interpreter-constant-operands/feedback-results.md).
- 2026-10-31: IR debug build passed; eleven focused cases returned ten passes
  and one negative-oracle failure. Preserved the stopped batch and idle cleanup.
  Source review identifies existing exit-block inlining as a concrete cause
  to test with the exact original code objects before changing the pass or
  oracle. Both streams prepare bounded observations with unchanged deadlines.
  [IR failure evidence](issues/interpreter-constant-operands/feedback-results.md).
- 2026-10-31: Reviewed exact-body/code observations narrowed both oracle
  failures. GC targets retired while unrelated records remained; a separate
  last-entry metadata control preserves that boundary's coverage. IR words
  confirmed existing block inlining; accepted its test-only correction and
  exact debug-product bridge. Corrected eleven-case selection passed; broader
  remaining feedback runs under the original deadline.
  [GC observations](issues/gc-nursery-adoption/feedback-results.md),
  [IR continuation](issues/interpreter-constant-operands/feedback-results.md).
- 2026-10-31: Independent review accepted GC oracle/fixture correction856;
  fresh debug feedback started while preserving the old image and deadline.
  IR full-module and generator checks passed, then stopped at the anticipated
  literal-operand disassembly golden. Prepared a bounded unchanged-source
  inventory before updating expected output; no performance claim.
  [GC fresh run](issues/gc-nursery-adoption/feedback-results.md),
  [IR remaining checks](issues/interpreter-constant-operands/feedback-results.md).
- 2026-10-31: Independent review accepted all seven fresh nursery feedback
  rows, including full GC OFF/KILL, child roots and cleanup. Temporary causal
  controls precede qualification. IR disassembly inventory observed six
  assertions, then stopped on a host phase-name assumption; preserved both
  failures and bounded read-only classification before any golden edit.
  [GC outcome](issues/gc-nursery-adoption/reviews/retirement-feedback-outcome-20261031-v1.md),
  [IR evidence](issues/interpreter-constant-operands/feedback-results.md).
- 2026-10-31: Reviewed a stronger finite-major test after identifying the old
  witness's limit; froze the test-only GC correction for positive/negative
  feedback. IR fixture capture passed and explained the three changed strings
  and one-instruction jump-table shift; froze golden-only source for exact
  review and remaining feedback. No timing or qualification acceptance.
  [GC oracle review](issues/gc-nursery-adoption/reviews/finite-major-oracle-20261031-v1.md),
  [IR capture](issues/interpreter-constant-operands/feedback-results.md).
- 2026-10-31: Accepted exact IR goldens and remaining helper bridge; corrected
  disassembly, code-object and tracing modules passed. Monitoring stopped at
  one loop-exit offset assertion. Preserved the failure and idle cleanup;
  exact-code diagnosis precedes correction, with release/emission still open.
  [Remaining feedback](issues/interpreter-constant-operands/feedback-results.md).
- 2026-10-31: Exact code confirmed the monitoring offset shift; its one-value
  correction passed all 108 monitoring cases. All planned debug scopes passed.
  O3 then hit its fixed-window timeout; preserved unqualified partial products
  and assigned an explicit separate twenty-minute release/emission completion
  scope for preparation/review. No speedup or qualification claim.
  [IR execution and decision](issues/interpreter-constant-operands/feedback-results.md).
- 2026-10-31: Independent review accepted the preserved 426-entry interrupted
  tree and exact release-completion helpers. Started the separate twenty-minute
  fresh O3/cases/emission batch; GC causal controls remain reviewed and held
  behind that native window. Original timeout retained, no new speed claim.
  [Completion scope](issues/interpreter-constant-operands/feedback-results.md),
  [GC controls](issues/gc-nursery-adoption/causal-plan.md).
- 2026-10-31: Fresh O3 and eleven release cases passed; emission stopped on an
  unsupported non-PIE assumption. Matched evaluator text grew 18.4248%. The
  coordinator's initial size-based rejection was withdrawn after user steering
  before any timing: actual elapsed performance will decide; size/counts remain
  diagnostics. GC causal controls started; baseline witnesses passed.
  [Revised performance decision](issues/interpreter-constant-operands/performance-revision-20261031.md),
  [execution evidence](issues/interpreter-constant-operands/feedback-results.md).
- 2026-10-31: Independent review reopened the IR performance decision under
  the elapsed-based revision, preserving the original size observation and
  historical rejection. GC causal feedback completed with four positive cases
  and all three nominated fault detections; terminal cleanup passed. Independent
  causal outcome review precedes cadence work.
  [Performance revision](issues/interpreter-constant-operands/performance-revision-20261031.md),
  [GC execution](issues/gc-nursery-adoption/feedback-results.md).
- 2026-10-31: Accepted independent GC causal-control and IR native-work reviews.
  The focused counter journal change passed nine mock tests and independent
  source review. A single restarted-host readiness probe denied hardware
  counters; preserve the failure and revise the held plan before scores, with
  elapsed performance still decisive. GC cadence design review proceeds in
  parallel; neither runtime candidate has performance or landing acceptance.
  [GC review](issues/gc-nursery-adoption/reviews/causal-controls-outcome-20261031-v1.md),
  [IR review](issues/interpreter-constant-operands/reviews/emission-work-removal-outcome-20261031-v1.md),
  [capture review](issues/interpreter-constant-operands/reviews/raw-capture-source-20261031-v1.md).
- 2026-10-31: Accepted the concrete halfway GC cadence design and granted a
  separate 45-minute source increment, preserving the overall deadline and
  mandatory nursery architecture. The policy must earn its place through
  actual elapsed performance; instruction/text size are diagnostics. Source
  and native review remain separate gates.
  [Design and review](issues/gc-nursery-adoption/reviews/cadence-design-20261031-v1.md).
