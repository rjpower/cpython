# DeltaBlue: next call-model work

Selected checkpoint: runtime 48 `5559f60093e`, canonical class values and owned
classmethod forwarding. Both are committed/pushed. Read the
[report](checkpoint-report.md), [tables](benchmark-tables.md) and
[status](../../STATUS.md) before resuming. This plan is the future queue, not a
list of completed agents. The single [Astra review](astra-architecture-review.md)
is complete; do not request another inner-loop review.

## Outcome and boundaries

Move unchanged DeltaBlue toward upstream parity while retaining logging parity
and broad useful progress. Accept incremental mostly-beneficial checkpoints
with explicit regressions. Preserve Python value/control protocols and explicit
ownership. Exact CPython display/finalization timing, startup RSS, zero-box counts,
CPI/layout perfection and hard extension compatibility are not gates.

Ordinary exact calls already overlap argument homes and use initialized storage;
there is no full argument copy or full temporary sweep to remove. Keep one owned
call window, frame ABI, return recipient/continuation and observation model.
See the [grouped design](design-overview.md), [ordinary call audit](ordinary-call-audit.md),
[IR](../../design/register-ir.md) and [interpreter](../../design/interpreter.md).

The complete [leaf experiment](integration49.md) is rejected: 25,504 real elisions
but DeltaBlue +15.2% and logging +21.3% against frozen 48. Its gate-off control
retains shared probes and gives +2.0%/+9.0%; ordinary logging cost remains open.
Do not promote either source or expand its syntax. The
[C2 proposal](C2-proposal.md) has [peer approval](C2-proposal-review.md) as a future
direction; no implementation or speedup is claimed.

## Delegation and sequence

| Owner | Bounded deliverable | Evidence before integration |
| --- | --- | --- |
| Sol design/debug | Isolate the ordinary shared-probe residual, using installed-cache invariants documented in the [audit](C-performance-audit49.md) | Preserve current mutable guards and canonical inputs; one reviewed neutral/profitable refactor against runtime 48 before layering leaf execution |
| Sol implementation | Select eligible leaf calls during specialization; leave ordinary exact calls without a leaf gate; store validated feedback units and consume checked-prefix facts | Independent review of shared exact-prefix uops, current handler/provider guards, format identity/deopt/instrumentation maps and untouched MISS |
| Sol completion design and independent ownership reviewer | Define the [common committed-state boundary](C2-completion-review.md) for publication, callback retirement and redirects; consider scalar unary retirement only as a bounded fallback | Explicit observer/phase authority, detached owners before callbacks, actual continuation after redirects, overlap-safe publication, no borrowed callee state after release; no fabricated child or second evaluator |
| Luna/Terra medium | Mechanical generated call-site/fixture updates and design synchronization after reviewed interfaces settle | Exact file ownership and minimal applicable checks; no speculative parallel edits to shared runtime files |
| Coordinator | Merge, regenerate, build, review, measure and land each useful increment | Focused native/API gates, independent actual-code post-review and basic matched performance; full diagnostics only at coherent useful batch boundaries |

1. Propose the smallest concrete source/contract change and obtain peer review.
   Separate experiments with exact source identities; keep golden/upstream fixed.
2. Implement only the reviewed scope. Use the existing DSL composites and narrow
   true effect facts; do not add a macroframework, per-handler exemption or second
   ABI. Format/opcode changes update all producers, consumers and pinned fixtures.
3. Run focused semantic/codegen tests and strict value-API checks. Review the
   actual generated/native composition. Debug fallback is not optimized coverage.
4. Integrate, count real admissions/misses without instrumentation, and compare
   DeltaBlue plus logging/nbody directly against frozen 48. If the ordinary path
   regresses, resolve it before expanding coverage. Stop if complexity outgrows
   measured benefit; keeping runtime 48 is a valid result.
5. At a useful batch boundary run release/debug full diagnostics, the unchanged
   29-kernel upstream screen and six-pair target confirmations. Publish failures
   and broad losses separately from the geometric mean. Update status/designs,
   report, handoff and Loom mirrors, then land the concrete checkpoint.

The nonfinal ledger is an implementation choice, not Python semantics. Removing
it needs the common completed-state proof above; existing PC equality and native
callback ancestry cannot establish it. This is a reviewed design lane, not an
approved implementation or speedup.

Direct scalar-comparison returns and bounded predicate composition may improve
coverage later. They are not remedies for an expensive execution contract.
Property/native adapters, keyword micros, coroutine entry, frame-layout migration,
object-layout replacement and instruction-scheduling work remain deferred.
Astra is reserved for the next genuinely broad grouped design review.

## Reproduction and source state

Source/build/control paths and current job state are in [HANDOFF](../../HANDOFF.md).
Evidence: `BASE/pyjit-link-wave1/deltablue-call-model-20261110`.
The selected source is runtime 48 on `pyjit/deltablue-checkpoint-20261110`; rejected C49
`24d97fa8c3b` and gate-off `af60e5eb43e` are committed/pushed experimental refs.
Full runtime 48 release/debug diagnostics and the stats smoke run are complete; known
debugger/tooling limits remain. The quiet full 29-kernel screen and six-pair target confirmation are complete; the report owns
results and next source admission.
Historical golden39 and upstream stay immutable. Prior
[checkpoint46](../object-call-20261109/checkpoint46-report.md) remains historical
rather than a permanent landing veto or an active CPI work queue.
