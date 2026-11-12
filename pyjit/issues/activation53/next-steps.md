# Next decision after activation53

Golden51 remains selected. All three round53 implementations are rejected;
their focused correctness passes and removed native work do not outweigh the
confirmed regressions. Do not compose them, repeat round52 forwarding, or reopen
them as a code-layout tuning queue. The [report](report.md),
[Astra review](astra-review.md) and independent [cold review](cold-review.md)
record the evidence and limits.

## Preferred investigation: reduce eager activation state

Start from the selected owning ABI. Inventory each header field's writes,
readers, lifetime authority and exposure requirements. Distinguish independently
owned Python state from values derived from the pinned executable and data
needed only when a frame is exposed. Name a concrete set of operations that can
be removed before implementing. A smaller initializer with the same stores is
not a candidate.

The measured header is 112 bytes versus upstream's 80. That is an opportunity,
not a predicted speedup: derived reloads can cost more than stored pointers.
Keep function/code owners, real current-frame and exception state, and semantic
continuations correct at every poll, callback and escape. Update all consumers
together. Use existing entry authority; add no universal eligibility guard or
second lazy-frame representation merely to make reconstruction convenient.

Delegate two independent Sol medium audits: one inventories header ownership,
construction and observer consumers; one weights actual writes/reads and reviews
native code across DeltaBlue, Richards, recursive coroutines and generators.
The coordinator combines them into one falsifiable proposal and obtains peer
review. A broad Astra review is appropriate for the resulting group of designs,
not each implementation revision. Terra/Luna can update consumer tables and
documentation after interfaces are settled. Use separate subtrees for actual
alternatives; the coordinator owns generation and merged validation.

Admission requires the exact removed stores/loads or reduced frame geometry in
native output, repeatable ordinary-call and coroutine gains, and acceptable
generator/logging/all29 breadth. First run focused owner, frame escape, GC,
observer/reentry and exception tests. Screen two balanced pairs against actual
golden51, confirm with six, then run broad diagnostics only for a worthwhile
integration. Compare the final candidate directly with upstream. Reject a
derived field if its reloads or reconstruction checks erase the benefit.

## Alternative: carry existing proofs through SEND/YIELD

The generator workload executes about 1.569 million resume/yield pairs per
100000 Tree constructions. Count actual SEND/FOR admissions, CREATED/SUSPENDED
entries, recipient classes and callback-capable retirement. Then identify a
specific native reload, classification or intermediate dispatch that existing
producer/admission facts make redundant. Preserve all real frame, exception and
recursion transitions. A callback or poll invalidates any fact it can change.

A Sol medium implementation lane should proceed only after an independent
review confirms that the proposed work is both executed and redundant. No new
chain cache, completion descriptor or per-resume eligibility test is justified
by the current evidence. Another renamed helper or terminal is not a proposal.
The gate is less dynamic transition work and a repeatable unchanged-generator
gain without material coroutine or ordinary-call regressions.

## Larger alternative: compile bounded regions on this ABI

The register IR names memory homes; it does not by itself keep values in hardware
registers or remove dispatch and repeated classifications. The unfinished shared
method/tracing compiler can address those costs across workloads. A first region
must use actual existing feedback and preserve the current owning ABI and real
frames. Add guards at compiled entry or actual invalidation points, with exact
owner/control recovery at exceptions, callbacks, polls and exits. Generator
allocation elision requires a separate escape/materialization design.

This is a larger commitment, not a claim that interpreter parity requires a JIT.
Before expansion, demonstrate an end-to-end region with real entries, fewer
dispatch/home operations, invalidation and exception recovery, and net elapsed
improvement including guards/exits. A skeleton or boxing counter is insufficient.

## Measurement discipline

Keep one GCC configuration, immutable controls, CPU15, hash0, JIT disabled for
interpreter comparisons and unchanged workload bodies/loops. Separate phase
censuses, GDB hits and PMU diagnostics from quiet elapsed runs. Retain both pair
ranges and stable source/native/extension identities. Instruction reductions,
symbol sizes and flat sample percentages do not establish a throughput cause.
Do not mask a repeated target regression with a noisy logging result or a
single suite geometric mean. Publish admitted and rejected results alike.
