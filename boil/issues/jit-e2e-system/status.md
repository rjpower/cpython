# Cohesive method and tracing JIT

**M1 focused native behavior accepted; detailed IR/feedback design reviewed. Natural admission is next.**

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

- restart_audit: user-requested IR/feedback design, actual attribute strategies,
  typed SSA, opaque-call boundaries and concrete natural-admission/workload plan.
- qualification_review_resumed: independent design review after sealing M1 outcome.
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
