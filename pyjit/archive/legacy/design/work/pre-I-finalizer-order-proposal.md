# Filter finalizer-order roots by actual call eligibility

Source proposal only, 2026-10-29. Read the isolated
[eligibility finding](/home/power/code/cpython/.worktrees/boil-h3-stats/boil/design/work/pre-I-finalizer-order-eligibility.md)
and primary f2a5b3ca33d1. Collector/generator files match the finding:
gc_live.c0833204b, genobject.c9a948aac, pycore_genobject.h4c56a0e4 and
pycore_frame.hd11fa9f9. No patch, target, build, probe or additional profile
was run. This is avoidable per-dead graph work, independent of generations.

## Small change and proof

`gc_trace.c:live_walk_block()` classifies every dead object with tp_finalize
as FINALIZE. `gc_live.c:finalizer_order()` currently starts a DFS at each
such object and appends every tp_finalize-bearing postorder entry. Only
`handle_finalizers()` subsequently rejects non-dead, FINALIZED and no-op
generator objects. Thus an all-cleared generator batch still allocates
visited/stack/order storage and traverses its retained code/name/exception
graph, despite queueing no finalizer calls for those generators.

Factor the existing actual-call check into **one local gc_live.c predicate**:
tp_finalize present, DEAD set, FINALIZED clear and
`!_PyGen_FinalizerIsNoop(op)`. Use it for DFS root admission, postorder
emission and the existing enqueue guard. Existing FINALIZE classification
and res->kinds remain unchanged. No public API, header/type flag, generator
layout or special-case list is needed. Proposed patch scope is gc_live.c
and one pure-Python regression in test_gc.py.

Reuse `_PyGen_FinalizerIsNoop` exactly: the object's finalizer function equals
PyGen_Type.tp_finalize and FRAME_STATE_FINISHED is true. Here FINISHED means
exactly FRAME_CLEARED, not every suspended/exhausted-looking numeric state.
gen_finalize already returns immediately in that state. Skipping its queue
allocation/resurrection already exists and is not a new benefit to claim.

**Do not filter order_visit or general traversal.** An ineligible generator,
already-finalized object or ordinary container can connect two eligible
finalizers. It still participates in the dead-graph DFS and visited-mark
cleanup. Only root selection and order-list membership are narrowed.

Proof: let E be the objects eligible for a finalizer call after legacy-cycle
resurrection. Every member of E remains a DFS root. Every dead path from
one member to another is therefore present in the DFS-reachable graph,
including all ineligible transit vertices. Reverse postorder puts a referrer
before its referent across acyclic paths; filtering emitted vertices to E
preserves that relation. Within a cycle the current ordering is arbitrary.
Removing inactive roots can change unrelated/cyclic order, which is not an
observable ordering guarantee. It cannot remove an eligible vertex or its
reachable transit graph. Predicate state is stable during ordering: no Python
code runs there; temporary MARKED changes do not affect the predicate.

The saving is bounded: ineligible independent DFS roots and order entries
disappear. With no eligible roots, the loop still inspects dead metadata but
allocates none of the three DFS vectors. Transit traversal required by an
eligible root remains. General mark/heap enumeration, dead classification,
allocator frees and gen_traverse work outside this DFS remain; the existing
profiles do not isolate the candidate's savings.

## Required unchanged behavior

Weakref callback clearing still precedes this stage. Legacy tp_del handling,
cycles and DEBUG_SAVEALL retain their current decisions; the legacy pass can
clear DEAD before eligibility is checked. Queue every eligible call before
resurrecting its object/dead referent closure, exactly as now. Do not narrow
resurrect(), gen_traverse(), destroy-hook work or owner routing.

Keep once-only FINALIZED/DEL_CALLED behavior, non-GC re-finalization rules,
callback/exception context, queue/current-call roots and permitted Python
execution points. Referrer-first DEALLOC_READS_REFERENTS ordering, instances
before types and the separate final block-free pass are different operations
and remain intact. Existing allocation failure returns/keep-all paths remain
required; the local predicate allocates nothing and must not create an early
return that bypasses visited cleanup. Existing queue/OOM tests do not by
themselves prove every libc DFS-vector allocation failure is covered.
The known queue-progress/lifecycle debt is unchanged; no Queue2 work belongs
in this patch.

## Regression and falsifying control

Add one graph regression beside TracingCollectorTests.test_finalizer_order:
two ordinary Python F instances with logging __del__ methods, connected by
`outer -> closed generator -> code -> co_consts tuple -> inner`.
Construct the generator using types.FunctionType from a template generator's
code.replace(co_consts=original_consts + (inner,)); close it, then keep it
only in outer. Appending a constant preserves all bytecode constant indices.
Assert gi_frame is None and gi_code.co_consts[-1] is inner while constructing
the graph. A helper returns no F/function/code/generator references; outside
it keep only scalar event records/weakrefs. Collect using the existing bounded
test_gc collection convention, then require exactly outer before inner and
intact access to the inner's name from outer's finalizer. This uses public
Python objects and verifies the actual cleared-generator transit path; no
raw C object factory, address mutation or memory probe is needed.

Propose one separate copied invariant-breaking source control: omit the
postorder reversal, leaving eligibility and traversal unchanged. On this
acyclic graph, ordinary postorder necessarily emits inner before outer,
independent of DFS root/heap order. Expect the named event-order assertion,
not a crash, import failure or missing test. This validates the ordering
oracle; it is not empirical coverage of every possible transit omission.
For the specific tempting bad change, source proof also gives a counterexample:
if order_visit drops the closed generator and outer is visited first, the
independent inner root reverses ahead of outer. A heap-order-dependent
negative that happens to pass must receive no credit; do not replace this
with blind repetition or global gen_traverse mutation.

Relevant existing semantic coverage is:

- test_gc: finalizer_order; resurrection_only_happens_once_per_object;
  resurrection_is_transitive; is_finalized; legacy_finalizer/newclass;
  weakref_callback_runs_after_collection; dealloc_reads_referents and its
  flagged/cycle variants; capsule_destructor_runs_python_code;
  finalize_collects_and_raises; InterpreterQueueTests allocation_failure and
  queued_payload_survives_collection.
- test_generators.FinalizationTest: frame_resurrect, generator_resurrect,
  refcycle and exhausted_generator_frame_cycle.
- Coroutine unawaited-warning/close tests and asyncgen GC-close/hook tests
  cover still-active finalizers, which the predicate must continue admitting.

Select the exact affected cases once on release and actual debug kill for
first feedback, with declared skips/modes. Keep parent receipts as comparison,
not proof of the candidate. Broader changed-collector/integration gates remain
due under PROCESS and coordinator review; this proposal waives none and
authorizes no test launch.

## Matched release decision

After correctness/source review, compare frozen parent/candidate releases
with identical compiler/config/eval flags, standard library and source except
this patch. Reuse the existing time driver, registry loops, warmup, paired
fresh-process order, quiet reservation and all sample retention. Begin with
the three profiled slow rows plus Richards control; five samples are an
inexpensive initial estimate, not a reliability threshold. Extend only a
named ambiguity that changes the keep/reject decision. Do not time debug,
stress or Py_STATS products against release or use unmatched ce/upstream
times to attribute this change. No new benchmark-specific fast path is needed.

Keep the patch only if its small implementation and semantic proof hold;
report measured benefit or uncertainty honestly. Large collection fractions
do not establish a large finalizer-order fraction. Reject extra flags, new
counter frameworks or unrelated cleanup to chase a predicted improvement.
