# Option 4 independent handwritten-source review

Status: approved corrected handwritten source after generator publication and
staging visibility fixes; no unresolved source blocker.
No compiler, runtime, test or
performance execution by this reviewer. Root owns those gates and regeneration.

The patch keeps Initialize consuming/nonfallible, prepares an empty untracked
frame object before reservation for a supplied mapping, and destroys only that
empty object on reservation failure. f_frame=NULL makes failed preparation
safe to destroy. Supplied mapping is installed before complete association and
tracking, with no ordinary NULL-mapping allocation. PyFrame_New reuses its
allocated object and uses the containing-object mapping owner for its embedded
header. Complete owner conversion is centralized rather than guessed by readers.

Mapping is visited once from PyFrameObject traversal, including its external-
frame case; interpreter-frame traversal visits frame_obj and no duplicate map.
Mapping-bearing attached objects are tracked after association. Unique record
retirement detaches a C mapping owner before dropping the record, clears homes,
then mapping, then function, with executable pin retained through callbacks.
Escape/takeover leaves mapping on the surviving object. Existing-object observer
publication materializes pending kwargs; preparation does not. Raw f_locals C
field references were exhaustively removed. New custom mapping, cycle, trace,
finalizer, failure and C-API conversion tests exercise meaningful behavior.

Two actual publication findings were reported to the author/root:

1. Frame-object-to-generator conversion originally released f's critical section
   before generator names/exception/weakref metadata initialization. The revised
   code holds the lock through complete metadata and association/tracking;
   coroutine origin allocation happens before ownership move, and its failure
   destroys only the empty untracked generator. Wrapper constructors no longer
   mutate an already-published incomplete object. This correction is sound.
2. RETURN_GENERATOR must have a complete first-send recipient before publishing
   its associated frame. Moving source return_slot assignment before Copy fixes
   that header gap. However, setting gi_state=FRAME_CREATED before Copy is unsafe:
   make_gen already tracks the object with FRAME_CLEARED, so another thread can
   discover it directly and attempt send during a partial move. Keep it
   nonrunnable until header/homes/recipient are complete; set CREATED inside
   the association lock immediately before exposure/unlock. Final source does
   this: CopyData completes homes and destination owner, publishes associated
   f_frame, then release-stores CREATED before unlocking. RETURN_GENERATOR
   records the incoming recipient on its source before this complete move.
   Reinspection confirms the correction and approves source handoff.

These are free-threaded source-contract requirements, not a claim that any
free-threaded build or test was run. Actual GIL release/debug qualification and
native layout/allocation/clear accounting remain required after source approval.

## Staging visibility follow-up

Tracking a mapping-bearing object immediately after Initialize exposes a
semantically incomplete frame through gc.get_objects/get_referrers before
binding and the first RESUME. GIL tracked-list enumeration has no incomplete
filter; FT heap enumeration stops the world but likewise lacks that filter.
frame.f_locals still asserts completeness. Header initialization alone is
insufficient. The coordinator found this after the source approval above;
that approval was suspended until the visibility fix below was reviewed.

A bounded candidate is to keep the thread-attached eager record untracked
until actual observer publication, and track it on complete generator/frame-
object transition. An unobserved thread record has bounded frame lifetime and
cannot form a back-reference through supported APIs. Cyclic GC treats the
mapping's count from an untracked record as an external root. Selected custom
trace code reaches untracked complete records through precise frame visits
(gc_trace.c:1181; mark_visit/mark:712–773); its conservative staging scan also
scans reserved headers from datastack_limit (1025–1045) and queues the raw block
of an untracked record, retaining its initialized mapping pointer. This supplies
source evidence for hiding the record without losing the mapping root.

Tracking must still occur at complete heap transition even if no observer asks
for the frame: mapping→generator→record→mapping can otherwise become an
uncollectable cycle. Actual publication must materialize pending kwargs before
returning the frame object, and records must become tracked before external
references can create cycles. No universal per-RESUME guard or GC API filter is
proposed.

Final reinspection confirms the refinement is implemented: the generic
initializer no longer tracks its private eager record. Existing-object
publication locks the record, materializes kwargs through its current locked
`res->f_frame` association, then tracks on success before returning it. This
avoids using an original frame pointer that could rebind while lock acquisition
waits. Complete generator Copy tracks its associated object after header/homes/
owner/association commit and before release-publishing CREATED. Takeover retains
its existing final complete-object tracking. No per-RESUME check was added.

Two added tests address the new contract directly:
`test.test_frame.TestFrameLocalsMapping.test_private_mapping_gc_visibility`
checks exec/class private records stay absent from get_objects/get_referrers
while their original mapping remains valid through collection; tracing/profile
is explicitly skipped because it intentionally publishes frames.
`test.test_frame.TestFrameLocalsMapping.test_unobserved_generator_mapping_cycle`
checks the otherwise-unobserved mapping→generator cycle is collectible.
The full TestFrameLocalsMapping class and the two new C-API frame mapping tests
remain the focused root selectors. Source approval is restored; actual runtime
execution, generation and native validation remain root-owned and pending.

## Regeneration and C-API follow-up

Root generation exposed an effect-inference regression: RETURN_GENERATOR lost
its direct CREATED store when publication moved into `_PyFrame_CopyData`, so
`writes_suspended_frame_state` no longer recognized it and classified it as
PUSHES_FRAME instead of SUSPENDS. The focused handwritten fix recognizes the
actual `_PyFrame_Copy` third owner argument. A literal GENERATOR owner means
CREATED publication; other literal owners do not; ambiguous/conditional owner
expressions fail generation instead of silently acquiring the wrong effect.
Guard mentions alone do not establish suspension. The locals author
independently approved this effect fix. New generator tests cover those
positive/negative/rejection cases; `ContractTests.test_effects` also explicitly
rejects PUSHES_FRAME for RETURN_GENERATOR. Root owns regeneration and execution.

Root's actual C-API probes retained the baseline debug failure at the impossible
frame_obj==f assertion, then found that repaired owner association exposed a
second initial-recipient bug: PyFrame_New starts after construction with generic
return_slot0, so the first None send can populate an otherwise empty scratch
home. The locals author set the source return_slot=IR_NO_DST under the existing
f critical section before Copy in the cold frame-object-to-generator converter.
Independent reinspection approves this correction: codeobject.c:1217 explicitly
rejects a RETURN_GENERATOR destination other than IR_NO_DST during constructed
IR admission. Thus discarded first input is an established contract, not an
assumption about the test's slot layout. Normal construction is unchanged.
The existing `FrameTest.test_frame_new_generator_mapping` exercises this path;
root retains failure evidence and owns the corrected debug/release runs.
