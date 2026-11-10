# DeltaBlue: ordinary call setup, execution and teardown

Active round, 2026-11-10. Runtime46 `b8e7e1bee66` is the development baseline;
the prior report retains its tradeoffs. Historical golden39/upstream controls
remain immutable. The user permits incremental mostly-beneficial landings and
selected DeltaBlue as the next single focus. Exact CPython finalizer/debugger
transcripts, startup RSS and instruction-scheduling perfection are not gates.

## Outcome and design

Reduce complete ordinary call cost and drive unchanged DeltaBlue toward upstream
parity. Baseline46 screen is1.63233/upstream. Keep one canonical value model,
owned call window, in-place binding, result/continuation and observation contract.
See the [grouped design](design-overview.md), [ordinary call audit](ordinary-call-audit.md)
and [lookup audit](method-lookup-audit.md). Source implements argument overlap and
initialized storage already; do not budget savings from nonexistent copies or
full temporary sweeps.

One bounded semantic census counts51,422 calls per workload iteration. Conditional
field selectors account for49.4%; classmethods5.3%; constant/single-getter recipe
candidates only1.56%. These are frequencies, not time shares. The larger structural
lane is complete guarded leaf-region call elimination including comparison/select,
not many low-coverage tiny recipes or a new frame ABI.

## Review and workstreams

The single [broad Astra review](astra-architecture-review.md) is complete. It
approves canonical class values, then classmethod forwarding, and selects complete
conditional leaf-region elision as the structural follow-on. Sol owns component
proposal/code review and debugging; do not call Astra repeatedly.

| Owner | Work | Boundary |
| --- | --- | --- |
| Astra medium | One grouped architecture review | Rank structural options, choose first increments and missing proofs; no runtime edits |
| Sol lookup | Concrete scalar/classmethod cache proposals, then implementation | Own lookup runtime files only after review and agreed interfaces |
| Sol calls | General leaf-region proposal and independent lookup peer review | Whole final-IR/effect certificate, mutable guards and common completion; no secondary Python interpreter |
| Coordinator | Architecture integration, file ownership, builds, measurements and landings | Commit reviewed increments, update designs/status and report actual tradeoffs |
| Luna/Terra medium when needed | Mechanical ports/documentation sync | Bounded file ownership, reviewed shared rules |

The chosen order is canonical direct class-scalar reads, then exact Python
classmethod forwarding as separate landings. The [lookup proposal](lookup-implementation-proposal.md)
and [focused test plan](test-plan.md) define those boundaries.
Sol prepares the larger conditional leaf-region lane in parallel; implementation
requires its own complete certificate/observer/completion peer review.

## Admission and execution

1. Finish grouped review; record the selected call representation and contracts.
2. Sol peer-reviews the first concrete lookup proposal; coordinator sets exact
   nonoverlapping file ownership. Implement, regenerate, compile release/debug,
   run focused mutation/ownership/descriptor tests and post-review the actual diff.
3. Integrate and run basic DeltaBlue plus logging/varied controls. Keep mostly-
   beneficial increments with documented tradeoffs; a small unrelated regression
   does not automatically veto progress. Correctness and valid ownership remain
   requirements.
4. Repeat for classmethod capture using the existing owned function/class pair.
   Preserve lookup-before-argument-evaluation semantics and live descriptor payloads.
5. At the coherent batch boundary run full diagnostics and the unchanged29-kernel
   screen, publish DeltaBlue comparisons and the next architectural decision.
   Full suites do not belong to each opcode/fixture update.

Use one GCC configuration, fixed affinity/hash/JIT-off inputs and verified source
identity. Reuse established native builds/runners. Basic timing is enough to
select a useful increment; use perf only to answer a concrete design question.
The old CPI/annotation ablation, keyword micro work and coroutine entry are deferred
from this round. No benchmark/source-name specialization or altered workload.

## Status and reporting

Last update:2026-11-10 22:35UTC. The grouped review, preparatory audits and semantic
census are complete. Sol approved the first concrete lookup proposal, including
its generated publication contract and unchanged serialized format. Baseline46
timing has completed; Sol is implementing class-value reads. Another Sol reviewer
is resolving leaf entry/observation and ordinary completion interfaces. Next
report follows first integration; no timer or unattended delivery is promised.

Evidence: `BASE/pyjit-link-wave1/deltablue-call-model-20261110`, BASE as in
[HANDOFF](../../HANDOFF.md). Prior [checkpoint46](../object-call-20261109/checkpoint46-report.md)
remains historical evidence, not the current work queue or a permanent landing veto.
