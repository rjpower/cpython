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
