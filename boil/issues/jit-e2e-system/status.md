# Cohesive method and tracing JIT

**M3 first native batch failed: 35 passes, one admission regression.**
The fresh debug build passed, and all eight new observed-loop tests passed.
An unchanged old test errors after a zero-iteration call causes recorder
retirement to discard its installed baseline. The
[independent outcome](m3-debug-outcome-review-20261101-v1.md) and
[failed decision](m3-debug-decision-20261101-v1.json) preserve this result;
whole M3 behavior is not accepted. A reviewed correction is frozen and
source-accepted; its runtime remains unvalidated.

User steering now prioritizes [universal PyValue restoration under RC](../pyvalue-rc-restoration/status.md).
The correction froze/source-passed at95be1d28; its bound next native batch is
held and unactivated while the value contract is reconciled. Preserve the JIT
implementation and all prior evidence; no corrected runtime result is claimed.

M2 focused natural-method debug behavior remains accepted: 28 passes, zero skips.
Six registered-program diagnostics subsequently completed with no native-entry
witness among 12 queried functions. The missing common-constant and raise helpers
are present in the reviewed M3 source; no speedup is established.

User feedback36 on Loom revision31 replaced the narrow arithmetic-head tier with
an end-to-end method and tracing JIT. The JIT is the selected direction;
experiments choose mechanisms and optimization policies within that system.
The [reviewed architecture](../../design/jit-e2e-system.md),
[design decision](design-decision-20261101-v1.json) and
[implementation milestones](implementation-milestones-20261101-v1.md) define one
shared semantic ABI, optimizer/backend, state recovery and code owner.

The accepted RC runtime stays selected. GC work is closed. Earlier frame-entry
experiments remain rejected; unexecuted quickened-block source remains component
evidence. These records do not authorize competing permanent backends.

## Current implementation

M1 source `15987a03c236d5257b8a7b944bf858ee20e52fc3` is frozen in
`build/worktrees/jit-method-baseline`, with18 changed paths. The
[independent exact-source review](m1-source-review-20261101-v1.md) and
[source disposition](m1-source-disposition-20261101-v1.json) accept its shared
method descriptors/LIR, x86 branches and helper calls, common Action consumer,
exec-owned units, publication and active-byte lifetime handling.

The fresh debug build and all14 selected methods passed, with zero skips:
eight existing semantic-helper methods and six actual native methods. The
[independent outcome review](m1-debug-outcome-review-20261101-v1.md) and
[behavior decision](m1-behavior-decision-20261101-v1.json) accept focused behavior.
The native tests exercise varying loops/branches, Python and C calls, dunders,
exceptions, observer invalidation/recovery, affected-code-only fallback,
ephemeral constant destruction and invalidation during a live C/native return.

The captured27-word method has2419 executable bytes in a4096-byte RX mapping.
Independent review checks28 ENDBR destinations,55 calls bound to the candidate
ELF, direct edges/backedges, R12 preservation and stack alignment. The bound test
checks RX without write permission; its raw maps line was not retained.

Final cheap checks passed31 host generator methods, seven distinct C syntax
checks and a standalone emission/failure fixture. There are63 shared helpers and
290 explicit classifications. The source models NOT_TAKEN skip edges and exits
AFTER at the exact committed PC for any unexpected helper successor.

Supervisor94327 exited0; terminal cleanup confirmed native idle,22 available
locks,30 historical image/configuration files and776 prior-attempt files intact,
then removed the owned bytecode cache. Source and native grants are closed.

This is forced internal compilation, not natural hotness, comprehensive opcode
coverage, optimizing SSA, a tracing frontend, a speedup or main integration.
STORE_ATTR currently exits before the operation in M1; the interpreter's existing
attribute specialization remains available through exact fallback.

## Parallel next work

- restart_audit: terminal recorder settlement correction in isolation.
- qualification_review_resumed: parallel actual-source and outcome review.
- coordinator: source/runtime integration, evidence, issue board, WORKLOG and Loom.
- coroutine_design_review: prior research complete; later provider-capacity errors
  were reassigned without repeated retries or a model change.

The user-requested [IR walkthrough](../../design/jit-ir-feedback/design.md) and
[dictionary comparison](../../design/jit-ir-feedback/dictionary-storage-comparison.md)
passed [independent review](ir-feedback-design-review-20261101-v1.md).
The [decision](ir-feedback-design-decision-20261101-v1.json) adopts their typed
SSA, guard/effect/ownership and recovery contracts for implementation planning.
It distinguishes current interpreter feedback from proposed optimizer facts
and hardware registers. Dictionary-location specialization is a concrete
same-IR hypothesis; a storage rewrite or permanent alternate mode is not selected.
Natural admission and a fixed call/loop/object/suspension workload slice precede
observed-loop SSA, then layered guards, CSE/DCE, representation selection,
register allocation and guarded inlining. Broad timing and expensive qualification
belong at coherent integration milestones, not at each opcode.

The [concrete M2 design](m2-design-20261101-v1.md) passed
[independent review](m2-design-review-20261101-v1.md). Its
[source grant](m2-source-grant-20261101-v1.json) starts18:35:56 UTC,
checkpoint19:05:56, hard cap20:05:56. It adds15 existing shared generic helper forms,
three appended execution-only warming/entry handlers, site-local CALL heat,
per-exec periodic-backedge heat and bounded scalar caller continuations.
Installed methods patch their actual RESUME entry; uncompiled methods retain
ordinary entry. Before reentry, admission must own the original evaluated inputs.
Pending records retire on return, exception, generator creation and frame reuse.
Old IDs0–301 and pristine/marshal behavior remain stable. Initial policy bounds
compilation, memory and retry cost; it is not a performance claim.

Source inspection corrected one proposal inventory error: BUILD_CONST_KEY_MAP
does not exist in this tree. Constant-key dictionary construction uses BUILD_MAP,
which is covered; no opcode will be invented to satisfy the mistaken list.
Four closure adapters bring the callable-helper count from 63 to 82.

The [partial checkpoint](m2-checkpoint-decision-20261101-v1.json) passed before
19:05:56. The author sealed at 19:00:25; independent review at 19:02:16 verified
all 21 source hashes, 33 host methods, nine distinct actual-source syntax checks,
all 290 old metadata format rows and the appended private IDs. Generated normal
return, exception and generator-creation consumers are present. Historical host
and syntax failures remain preserved. This permits continued source work under
the original cap, without final source or runtime acceptance.

The [policy clarification](m2-policy-clarification-20261101-v1.json) fixes bootstrap
resource backoff before state exists, and records the explicit leaf-renomination
limit after observation ends. The final natural observer, resource, pending-capacity
and coroutine fixtures were subsequently validated in the corrected focused
batch described below.

M2 source is now frozen and clean at `3bcb891956a9d2796bbe5e92c4f0c9e2f2516899`,
with 22 changed paths in `build/worktrees/jit-natural-method`. The
[exact-source review](m2-source-review-20261101-v1.md) and
[source disposition](m2-source-disposition-20261101-v1.json) accept readiness
for a separate focused native validation. The source attempt is closed early.
All 28 semantic/native/natural methods are written; 33 host methods, nine final
syntax checks and actual encoder emission/failure checks pass. The failed
style-pass syntax attempt remains preserved alongside corrected final evidence.

The first fresh build passed, but its focused suite stopped after 13 passes at
an obsolete unsupported-list fixture. M2 now supports list construction, so the
fixture's premise was false. All 14 natural-admission methods were NOTRUN.
The [independent outcome](m2-debug-outcome-review-20261101-v1.md) and
[failed decision](m2-debug-decision-20261101-v1.json) preserve the original
source, image, raw failure and missing suite-result snapshot. Supervisor75385
is closed; cleanup verified native idle and removed its owned cache.

The [test-only correction](m2-fixture-source-disposition-20261101-v1.json),
source `3174bc0ab726cb7b7b39f25d93f4ec9194c88d3a`, uses the actually unsupported
CALL_FUNCTION_EX form and checks that its callback executes once. Runtime core,
the other 27 methods and all 28 IDs are unchanged. Independent source and
controller reviews passed. A separate fresh build/suite batch started at
19:53:10 UTC with a 20:23:10 deadline and first-failure stop. All 28 methods,
including all 14 natural-admission methods, passed with zero skips. Root
supervisor53412 exited0 at19:55:47. Cleanup confirmed native idle,24 available
locks,2,353 prior files and30 parent fingerprints unchanged, then removed the
owned cache. The [independent outcome](m2-corrected-debug-outcome-review-20261101-v1.md)
and [behavior decision](m2-behavior-decision-20261101-v1.json) accept this focused
debug milestone. The emitted27-word/2419-byte method again binds its55 helper
calls to the exact candidate ELF and passes native entry/continuation assertions.
This batch is closed; source, native and timing permissions remain closed.

The [observed-loop proposal](m3-design-20261101-v1.md) and
[independent review](m3-design-review-20261101-v1.md) now pass design review.
The [source grant](m3-source-grant-20261101-v1.json) opens one isolated attempt
from accepted3174 in `build/worktrees/jit-observed-loop`: start20:01:13 UTC,
checkpoint20:31:13, hard cutoff21:31:13. It requires actual paired recording,
shared typed SSA/effects/FrameState, scalar LIR, register allocation/spills and
immutable baseline-plus-loop publication with precise recovery. Three commits
are dependency steps; partial graphs or encoding are not M3 completion.
Host/generator and bounded syntax/encoder fixture checks precede fresh native
validation. No candidate build/runtime, timing or integration is authorized by
this source grant. M2 workload diagnostics used their separate reviewed campaign.

The [partial checkpoint](m3-checkpoint-decision-20261101-v1.json) passed on time:
author20:27:28, independent review20:31:07, before20:31:13. Review verifies12
archived source copies,34host checks and a fixture executing the actual shared
SSA/verifier/allocator against synthetic events. Seven actual-source syntax
commands passed, but their receipt does not establish exact compiled byte
identity; that limitation remains explicit. This permits continued source work
under the same21:31:13 cutoff, with no final source or native acceptance.
Register PHI moves, folding/CSE and rooted backward DCE are now in source.
Versioned host fixtures exercise the actual verifier/allocator, malformed maps,
dead-definition removal and six physical spills. Backend emission/failure checks
pass without executing emitted code; eight syntax checks now pin source bytes
and compiler streams. Eight runtime methods are drafted; independent review is
correcting path coverage before final source freeze and review.

The held validation controller plans 36 methods and passes five mocked host
controls. Its [unbound receipt](../../../build/jit-method-m3-helper-preparation-20261101-v1/unbound-host-receipt.json)
records two rows, a future 1,800-second inclusive window and first-failure stop.
It preserves 10,392 closed files and keeps a fresh lock-only profile. Draft
controller review found no blocker; exact frozen-source binding and final
readiness review remain mandatory. No activation exists.

The [recording-budget clarification](m3-recording-budget-clarification-20261101-v1.json)
closes indefinite temporary-hook overhead when the nominated loop is unvisited:
4,096 temporary-Unit events, distinct from the unchanged512-event active buffer,
then settle at a validated AFTER boundary. Normal baseline Units acquire no
recorder budget. Replacement/free must revalidate before borrowed exec access;
terminal actions touch pinned metadata and defer cold retirement to fresh entry.
The selected-loop policy and source deadline remain unchanged. Native evidence
must establish successful typed execution/backedge, rather than an attempted
trace-entry marker alone.

Repeated source-word occurrences now reject this first trace format because a
Python return PC would otherwise select an ambiguous import adapter. Baseline
execution remains available. Independent review also found that inherited
compiler scratch is excluded from the process byte counter despite the reviewed
compiler-inclusive 8 MiB bound. The author is correcting cold allocation
accounting within the original source cutoff; the quota contract is unchanged.

The [six-program diagnostic](m2-workload-diagnostics-20261101-v1.md) is now complete:
six valid fresh processes, zero manual compile requests, no reference arm or
scores, no queried entry witnesses. Eight targets were refused at
LOAD_COMMON_CONSTANT79, one at RAISE_VARARGS102; three did not nominate
compilation. [Independent review](m2-workload-outcome-review-20261101-v1.md) and
the [decision](m2-workload-decision-20261101-v1.json) accept this negative
inventory. Root cleanup confirms native idle,24 available locks and all current
products/prior evidence intact. The
[reviewed amendment](m3-coverage-amendment-20261101-v1.json) adds these two
generated semantic helpers and their ownership/error/reentry checks within M3's
same paths and deadline. It changes neither hotness/cache budgets nor the fixed
workloads. Rare unsupported paths blocking whole-method admission are retained
as a separate future policy hypothesis, not silently added scope.

Source froze early at 21:04 UTC in three dependency commits, ending at
`90cdb7bbda9d1547f1d1ccdcef2388a5b82744c5`. Capsule99666396 and independent
a8815b39/458d93f1 review bind 16 source paths, 171 closed artifacts and seven
authority references. The old 28 test ASTs and six old generated products are
unchanged. The compiler-inclusive quota correction is implemented and reviewed;
the original limit and source deadline did not change.

The source decision closes source permissions early. Final bound controller
checks pass five mocked controls and exact 36-method AST equality. Independent
launch review7452632e/ea77aa40 accepts the two-row batch after verifying 17,713
inputs, 10,392 preserved files, 30 parent fingerprints and 12 tools. Root activated
at 21:10:03 UTC with a 21:40:03 inclusive deadline. Actual supervisor41481
exited1 at 21:12 UTC after build success and a failed focused row. All 36 methods
ran: 35 passes, one error, zero skips. The eight observed-loop methods all pass;
their scalar witness retains 4,013 bytes, 16 source words, 14 nodes, one spill
and seven scratch slots with actual scalar/backedge success flags. This is
partial mechanism evidence; it does not override the failed cohort. Root cleanup
verifies native idle, 25 available locks, 10,392 prior files and 30 parent
fingerprints unchanged, then removes the owned cache. That batch is closed.

The old native loop/call fixture explicitly compiles its method, then calls it
with zero iterations. Terminal handling marks its recorder exhausted; the next
entry detaches it, and later short calls cannot meet fresh hotness thresholds.
The [correction design](m3-terminal-settlement-design-20261101-v1.md) and
[observer clarification](m3-terminal-settlement-observer-clarification-20261101-v1.md)
passed independent review. Reuse the existing cold replacement transaction at a
validated live-frame entry, settle into ordinary baseline, then refetch and
revalidate once at the saved logical PC. Synchronize changed monitoring state
without replaying an operation or copying witness flags.

The separate [source grant](m3-terminal-settlement-source-grant-20261101-v1.json)
opens only runtime admission and the existing test module in
`build/worktrees/jit-terminal-settlement`, based on frozen90cdb7bb. Start21:19:16,
cutoff21:34:16 UTC, 900 seconds with no extension. All 36 IDs and old 28 test ASTs
remain. Host checks and changed-runtime syntax precede freeze/review. No new
candidate execution is authorized. The original source/image/logs stay frozen.
Performance qualification and primary integration remain unfinished.

## Retained evidence

M0 corrected source `f0a49b6d` passed all eight helper methods with zero skips;
its [behavior decision](m0-behavior-decision-20261101-v1.json) is the prerequisite.
The [initial generated-C failure](m0-debug-outcome-review-20261101-v1.md) and
[focused correction](m0-directive-source-disposition-20261101-v1.json) remain
preserved. M1's [timely source checkpoint](m1-checkpoint-decision-20261101-v1.json)
retains the initial syntax-v1 command-label/include failures and corrected evidence.
The source cap closed early, rather than extending its18:47 UTC deadline.

The [Loom architecture artifact](http://127.0.0.1:7878/s/6rydgj13/artifacts/runtime-work-breakdown)
contains the system and lifecycle diagrams; immutable revisions retain earlier
status. Canonical issue decisions above own current execution acceptance.
