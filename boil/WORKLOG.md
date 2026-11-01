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
- 2026-10-31: Rejected the thirteen-handler constant-operand increment on actual
  elapsed results: coroutine medians regressed 4.47% and 11.64%, with both
  controls also slower. Independent review verified the fixed 80 samples,
  eight origins, quiet checks, unchanged products and terminal cleanup. Text
  growth remains diagnostic; no extra timing or qualification follows. GC
  cadence source froze early for independent source review and feedback prep.
  [Performance decision](issues/interpreter-constant-operands/performance-results.md),
  [outcome review](issues/interpreter-constant-operands/reviews/quiet-outcome-20261031-v1.md).
- 2026-10-31: Reproduced twelve bounded native captures showing extra common
  evaluator-state loads/stores in the rejected build despite unchanged handler
  C bodies. Opened a separate 30-minute host-only design investigation; this
  neither attributes the full slowdown nor reopens the rejected increment.
  [Research scope](issues/interpreter-dispatch-state/status.md).
- 2026-10-31: Accepted independent exact-source review of the seven-file GC
  cadence increment. Prepared a separate 45-minute native feedback scope for
  thirteen methods/twenty-two children and three isolated fault controls;
  concrete helper review precedes activation. No native cadence result yet.
  [Source review](issues/gc-nursery-adoption/reviews/cadence-source-20261031-v1.md),
  [Held feedback](issues/gc-nursery-adoption/cadence-feedback-plan.md).
- 2026-10-31: Accepted revised cadence helpers after independent review caught
  verifier-mode, timeout-output and optional-artifact defects. Activated one
  45-minute native batch with four fresh builds, positive selection and three
  isolated fault controls. Source and earlier evidence stay frozen; no native
  result is credited at launch.
  [Execution](issues/gc-nursery-adoption/cadence-feedback-results.md).
- 2026-10-31: Cadence feedback stopped at the first linked-frame audit failure,
  before focused tests. Preserved the failed build and verified idle cleanup.
  Reviewed a two-annotation precise-frame correction supported by actual ELF
  calls and source checks; fresh remaining-only feedback keeps the original
  batch deadline, with no audit weakening or silent clock reset.
  [Failure and correction](issues/gc-nursery-adoption/cadence-feedback-results.md).
- 2026-10-31: Started fresh corrected GC feedback under the unchanged deadline.
  Independent IR design review selected handler-local slot-base derivation
  against real accepted-baseline return/yield transport; granted a separate
  30-minute source prototype. The elapsed-first GC cost plan is accepted for
  helper preparation, with explicit fresh-process and observation limits.
  [GC execution](issues/gc-nursery-adoption/cadence-feedback-results.md),
  [IR review](issues/interpreter-dispatch-state/reviews/design-20261031-v1.md),
  [Cost plan](issues/gc-nursery-adoption/cadence-cost-plan.md).
- 2026-10-31: Corrected cadence build passed the linked audit, then focused
  execution stopped at one stress=1 failure among thirteen methods. Independent
  review proved the test trigger allocates twice; authorized a single-allocation
  replacement while retaining strict kind/count/liveness assertions. Preserved
  all failures, confirmed terminal idle cleanup, and kept the original feedback
  deadline. The slot-base prototype passed host generation/root checks; native
  behavior remains untested.
  [GC failure and correction](issues/gc-nursery-adoption/cadence-feedback-results.md),
  [Trigger review](issues/gc-nursery-adoption/reviews/cadence-trigger-correction-design-20261031-v1.md).
- 2026-10-31: Accepted the exact single-allocation test correction and activated
  seven remaining GC feedback rows, reusing unchanged compiled runtime products
  under the original deadline. Independently accepted the frozen handler-local
  slot-base source after reproducing generation, preserved-body/reload proofs
  and meaningful entry checks; native helpers remain held for review.
  [GC execution](issues/gc-nursery-adoption/cadence-feedback-results.md),
  [IR source review](issues/interpreter-dispatch-state/reviews/source-20261031-v1.md).
- 2026-11-01: Accepted independent cadence first-feedback outcome: thirteen
  methods, twenty-two normal children and three detected scheduling faults,
  with terminal idle cleanup before the unchanged deadline. Preserved both
  earlier failures and the empty-callback observation limit. Cost helpers
  passed host review; release bindings remain held. The slot-base seventeen-row
  native preparation froze early for review; neither track has new timing.
  [GC outcome](issues/gc-nursery-adoption/reviews/cadence-trigger-outcome-20261101-v1.md),
  [Cost preparation](issues/gc-nursery-adoption/reviews/cadence-cost-helper-preparation-20261031-v1.md),
  [IR state](issues/interpreter-dispatch-state/status.md).
- 2026-11-01: Activated independently reviewed slot-base native feedback for
  seventeen serial debug/release, entry/root and emission rows, with a fixed
  45-minute deadline. GC release-helper review proceeds independently while
  its native builds wait. No timing or performance credit follows at launch.
  [IR execution](issues/interpreter-dispatch-state/native-feedback-results.md).
- 2026-11-01: Slot-base native batch completed all seventeen rows with terminal
  idle cleanup. Early native review finds the targeted exit transport removed
  but added frame/IP stack traffic; elapsed remains unmeasured. Activated the
  independently reviewed six-row GC release batch, reusing the historical image
  and preserving exact release guards. Held slot-base timing now has an accepted
  fixed-sample/noise plan; host preparation is separate from scoring.
  [IR evidence](issues/interpreter-dispatch-state/native-feedback-results.md),
  [GC release execution](issues/gc-nursery-adoption/release-feedback-results.md).
- 2026-11-01: GC release batch completed all six rows with terminal idle cleanup;
  independent outcome review remains open. Slot-base behavior/emission reviews
  accepted their exact scopes without a speed claim. At the user's request,
  reassessed the whole VM/IR/GC design against LuaJIT and its proposed collector:
  all-old nursery scanning, per-object inventory/reclamation, compiler-controlled
  interpreter registers and value/native boxing remain explicit opportunities.
  New benchmarks stay held during this design assessment; host preparation
  continues and no runtime alternative is being added.
- 2026-11-01: Independently accepted the six-row GC release outcome and froze
  actual three-arm cost bindings, still held for exact review and quiet
  activation. Completed the fixed slot-base comparison: coroutines regressed
  11.98% and 11.00%, and all controls regressed. Accepted the independent audit
  and rejected the increment without extra samples or a code-size veto.
  [IR result](issues/interpreter-dispatch-state/performance-results.md),
  [GC state](issues/gc-nursery-adoption/status.md).
- 2026-11-01: At the user's request, resumed three parallel read-only design
  tracks covering existing VMs, Boil state/root constraints and structural GC
  work. Drafted an explicit generated-tail state ABI, code-organization and
  boundary diagrams, and ordered compiler/backend, frame-transport, metadata
  inventory and conditional IR hypotheses. Independent review precedes
  implementation; a Loom artifact will mirror the reviewed repository design.
  [Candidate](design/vm-design-candidate.md).
- 2026-11-01: Accepted the exact independent VM candidate review after clarifying
  raw-memory inventory coverage and deadline feasibility. Published and read
  back Loom revision 1 with both Mermaid diagrams and source/review identities.
  Granted finite host-only compiler/backend plan preparation; independent GC
  structural and actual cost-binding reviews continue in parallel. No new
  runtime implementation, native activation or backend adoption is claimed.
  [Publication](issues/h1-tail-dispatch/publication-20261101.md),
  [Design review](issues/h1-tail-dispatch/reviews/candidate-design-20261101-v1.md).
- 2026-11-01: Activated the independently bound three-arm GC cost campaign
  under an actual quiet reservation. It stopped at the first historical
  Richards retention guard after twelve origins and forty-five normalized
  samples; the failed target returned zero. Preserved the incomplete run,
  confirmed terminal idle cleanup, and assigned separate failure audit and
  source diagnosis without widening bounds or appending timing samples.
  [Cost result](issues/gc-nursery-adoption/cadence-cost-results.md).
- 2026-11-01: Accepted independent structural-GC design review and granted a
  thirty-minute host-only page-scoped object-map prototype. VM0 plan correction
  preserved its first revision and specified actual PIE policy plus retention
  of the existing stats child's raw witness; native execution remains held.
  [GC design review](issues/gc-nursery-old-work/reviews/object-map-inventory-design-20261101-v1.md),
  [VM0 plan](issues/h1-tail-dispatch/compiler-backend-plan.md).
- 2026-11-01: Independent audit confirmed 58 actual cost processes and 46 raw
  timings before the historical-control retention failure. Source explains its
  intended 56-object growth per batch; historical heap inspection also collects
  outside the clock. Accepted a typed-graph oracle design retaining unchanged
  kernels and unexplained-growth limits; granted twenty-minute held helper
  preparation. VM0 plan v2 passed separate source review and entered held
  helper preparation; qualification draft now excludes rejected slot-base work.
  [Diagnosis](issues/gc-nursery-adoption/retention-observer-diagnosis-20261101-v1.md),
  [Oracle review](issues/gc-nursery-adoption/reviews/retention-oracle-design-20261101-v1.md).
- 2026-11-01: Prepared a source-only qualification intake while independent
  implementation tracks proceed. Confirmed twenty adopted-baseline and
  twenty-seven cadence nursery methods and identified legacy broad-helper
  class consumers. No source is selected and no native qualification runs;
  the final roster and input bridges remain tied to the eventual cost decision.
  [Qualification draft](issues/gc-nursery-adoption/qualification-plan.md).
- 2026-11-01: Reopened the IR/VM/collector architecture recommendation after
  the user's challenge. Redirected all three agents to critical source and
  primary-design reviews, with a 02:00 UTC report ceiling. No native job was
  launched: accepted retention helpers remain unactivated; VM0 and structural
  feedback preparation are suspended at saved checkpoints. Initial source
  findings identify stack-constrained slot allocation, always-current frame
  homes, largest-kind feedback strides and all-old minor scans as choices to
  test rather than settled design requirements. The integration boundary is
  unchanged. [Review scope](issues/fundamental-runtime-design/status.md).
- 2026-11-01: Three critical research reports finished early. Independently
  accepted an integrated design target: optimize logical values before frame
  placement, generate bounded effect-complete regions, and co-design stable
  core allocation with tracing/reclamation. Preserved historical decisions,
  explicitly reopened the particular nursery/storage commitment, and published
  Loom revision 1 with exact readback and source/review hashes. This accepts
  design selection only, not an implementation or performance result. Assigned
  finite IR-region, stable-storage-cohort and missing inventory-prefix witness
  designs; native execution remains held.
  [Candidate](design/integrated-runtime-candidate.md),
  [review](issues/fundamental-runtime-design/review-20261101-v1.md),
  [publication](issues/fundamental-runtime-design/publication-20261101.md).
- 2026-11-01: Froze the IR-region, integrated stable-storage and inventory-prefix
  witness designs early; no census or runtime implementation is granted.
  At the user's request, launched Claude Fable 5.1 to critique the GC-heavy
  ranking and propose a better IR/compiler/VM/GC work breakdown. Stopped the
  initial plan-mode/high-effort attempt on correction, preserving terminal143
  and raw output; replacement runs directly at medium effort with local file
  pointers in normal print mode. Its live tool handle is 78126; response and
  stderr remain in the recorded evidence directory. New native work stays held.
  [Peer request](issues/fundamental-runtime-design/claude-fable-5.1-request-20261101.md).
- 2026-11-01: Direct medium-effort Claude Fable 5.1 review completed with
  terminal zero. Preserved its response verbatim and assigned separate IR/VM,
  GC and native fact audits rather than adopting unsupported causal claims.
  Implemented complete parsed stats retention, handler/pair/native-PC
  differences and an original-dump journal in the host benchmark driver.
  Independent review accepted the exact change; all sixteen related host
  counter-record tests passed, with no target, compiler or permission probe.
  Fresh unchanged-runtime debug/stats baseline preparation is held for review;
  no current-source stats image was reusable. No runtime speedup is claimed.
  [Tooling review](issues/fundamental-runtime-design/counter-retention-review-20261101.md).
- 2026-11-01: Reconciled the external review against three source/evidence
  audits. Accepted a balanced compiler/IR, backend, native-interface and GC
  work plan; rejected unsupported boxing attribution, type-only coroutine
  eligibility, unsafe root omission and mandatory tail adoption despite a loss.
  Independent plan review accepted the exact revision. Deferred old cadence
  timing/qualification without extending its deadline or crediting incomplete
  samples. Shared stats preparation and concrete operand-locality/regex designs
  now have separate owners and finite scopes.
  [Revised plan](design/runtime-work-breakdown.md),
  [verbatim peer review](issues/fundamental-runtime-design/claude-fable-5.1-review-20261101.md).
- 2026-11-01: Published the unedited peer review and independently accepted
  revised work breakdown as separate Loom revision-1 artifacts. Exact readback
  matched both bodies and retained their Mermaid diagrams and provenance.
  The plan owns compiler facts, backend controls, native conversions and GC
  eligibility separately; shared baseline helpers remain unactivated.
  [Publication](issues/fundamental-runtime-design/peer-plan-publication-20261101.md).
- 2026-11-01: Corrected the execution emphasis on the user's clarification:
  interpreter architecture and integrated allocator/collector work are primary;
  regex extraction is a frozen, paused proposal. Assigned short executable maps
  for reusable value/effect-aware region lowering and stable-span ownership,
  reusing the existing research. Shared stats helpers froze early and entered
  independent review; no native launch or runtime speedup is claimed. Corrected
  stale issue-board ownership and distinguished the deferred nursery deadline
  from the separately scoped baseline window.
  [Current state](issues/fundamental-runtime-design/status.md).
- 2026-11-01: Independent source/helper readiness reviews passed for the one
  unchanged-runtime GCC debug/stats baseline. Root verified exact inputs and
  actual lock availability, then activated the reviewed ten-row batch at
  02:41:08 UTC, inclusive deadline 03:26:08 UTC, live tool session 20126.
  Compilation is observed; no build/census outcome or performance credit yet.
  The reusable interpreter-region proposal froze and entered independent
  review alongside integrated-storage design review. Regex remains paused.
  [Baseline source review](issues/fundamental-runtime-design/shared-baseline-source-review-20261101.md),
  [helper review](issues/fundamental-runtime-design/shared-stats-helper-review-20261101-v1.md).
- 2026-11-01: Shared baseline completed once at 02:44:35 UTC, terminal zero,
  all ten rows verified with idle cleanup. Independent outcome review accepted
  twenty focus methods, six stats methods, six-by-100 stale controls and six
  journals containing eighteen raw dumps. Current coroutine comparison/branch
  activity supports the region family; saturated boxing tables and unconnected
  standard GC counters limit other attribution. No elapsed gain is claimed.
  [Outcome](issues/fundamental-runtime-design/shared-baseline-outcome-review-20261101-v1.md),
  [findings](issues/fundamental-runtime-design/shared-baseline-counter-outcome-20261101.md).
- 2026-11-01: Independent designs accepted for reusable interpreter regions
  and integrated coroutine/frame spans. Started the isolated storage source
  attempt, 02:51–04:21 UTC, with mandatory 03:21 ownership checkpoint; it retains
  ordinary deallocators and lifecycle records, measuring storage facts inside
  the prototype rather than requiring a separate cheap-death census. Prepared
  a separately scoped minimal current-IR capture before region installation.
  Published the corrected execution focus and two code-organization Mermaid
  diagrams as Loom work-breakdown revision 2, with exact readback.
  [Storage review](issues/fundamental-runtime-design/stable-storage-mechanism-review-20261101.md),
  [IR review](issues/fundamental-runtime-design/interpreter-region-review-20261101-v1.md),
  [publication](issues/fundamental-runtime-design/peer-plan-publication-20261101.md).
