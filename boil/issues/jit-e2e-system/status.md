# Cohesive method and tracing JIT

**M2 focused natural-method debug behavior accepted: 28 passes, zero skips.**

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

- restart_audit: sealed observed-loop SSA proposal and design responses.
- qualification_review_resumed: corrected M2 outcome and independent SSA design review.
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
and coroutine fixtures are now written and source-reviewed below; their runtime
behavior remains unverified.

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

The sealed observed-loop proposal specifies actual paired recording, shared
typed SSA, register allocation, native scalar operations and precise side exits.
Independent design review is pending. Source and standalone checks are closed;
natural workload diagnostics precede elapsed scoring. SSA implementation,
genuine trace execution, performance qualification and main integration remain
unfinished. Release/ASan/full-suite qualification, speedup and main integration
are not established by this focused debug acceptance.

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
