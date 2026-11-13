# Producer-published created-frame home authority

2026-11-13. Bounded next packet after the empty-created copy certificate.
Retain the current NULL control for every uncovered lifecycle. No source edit
before root/IR/GC freeze the literal marker and actual visitor scope.

Publish `_PyIR_PHASE_CREATED` as lifecycle bit 12 in the existing 16-bit
phase_state, alongside CAPTURED bit 11. Only the certified fast created-copy
producer stores it, after header/local transfer and destination scratch fill.
The caller then publishes generator ownership and FRAME_CREATED before
tracking/return. This costs one literal 16-bit store, no frame/header/handler
argument growth. The uncovered control helper leaves phase zero.

`_PyFrame_GetHomeView` retains CAPTURED dispatch first. Otherwise it accepts
CREATED only as the exact standalone marker, with owner GENERATOR, actual
atomic gi_frame_state FRAME_CREATED, the independent code pin and its ready
execution image's ALL_CREATED_EMPTY certificate. It returns MUST_READABLE,
dynamic bindings [0,L), and empty scratch. It never reacquires authority from
phase zero, flags or PC alone. Public frame-to-generator publishes CAPTURED
from its actual source capture, so a code flag cannot erase arbitrary public
frame scratch. Combined/unknown lifecycle bits return NONE.

Existing invalidations precede mutation: `_PyFrame_SetReturnValue` and
PublishReturnSlot clear phase before the sent value; generator send claims
FRAME_EXECUTING before that entry. TryClear/InitClearRoots and ClearTemporaries
clear phase before closes. CopyHeader clears both lifecycle states before
transfer. Accepted f_lineno identity and general redirect commits clear phase
before continuation/home changes; a rejected request retains the old valid
state. Thus an accepted redirect that leaves gi_state CREATED cannot later
restore the marker. Named local writes preserve it because all current local
homes remain dynamically selected. Frame trace metadata has no scratch role.

The actual first consumer is `_PyGC_VisitFrameStack`, in GIL and FT collector
files owned by GC. Scope selection to the explicit CREATED marker, resolve
the common view, then use `_PyFrame_NextHomeRange` and the existing per-value
visitor policy. This changes reads/edge selection, not count subtraction rules.
GIL tracing/cyclic visits share that selected range. FT only uses the selected
view when the world is stopped; generic unsynchronized tp_traverse retains its
current control path. The common view's caller must hold a stable frame and
pin code through its range reads. Deferred-reference adjustment remains
separate; an owner bitmap is not introduced.

Test with a real ordinary created generator: query MUST/empty shape, mutate
named locals through f_locals, run collection, then resume and check those
values. After resume the marker must be gone; suspension stays uncovered
unless its own actual phase recipe is admitted. Public frame-to-generator
with the same certified code must still report captured CONTROL. Add a cold
protocol probe that clears the marker while leaving FRAME_CREATED and checks
the selector returns NONE; this demonstrates no phase-zero reacquisition
without depending on debugger admission of a particular jump. An actual
accepted/rejected debugger fixture can be added when its recipe is known.
Collection/reference tests verify the real visitor consumer; a precise test
helper may inject poisoned inactive scratch only into private nonexecuted
probe storage to prove that visitor never reads it.

Frames owns frame.c producer/selector plus tests and test helpers. IR owns the
single literal marker definition/rejection contract. GC owns both existing
collector visitor functions. Work follows the current CALL source freeze;
no concurrent whole-file edits. Source peer precedes root generation, both-role
checks and native marker/visitor cost inspection. General active/suspended
selected visits and all-runtime NULL-control removal remain later gates.

The implementation is concrete: the frames component changes frame.c,
test_frame.py and _testinternalcapi.c (97+/3-); IR owns the standalone marker
definition and GC owns the two existing collector functions. The selected
copy probe now allocates private untracked embedded-generator storage,
poisons inactive destination scratch, and calls the actual GIL frame visitor
with a nonallocating count callback. Only the real payload/code owners are
released. FT skips this poison visit instead of forging world-stop state.
The live fixture exercises local mutation, collection and resume; a separate
callback-free probe clears/restores only the marker and verifies no phase-zero
reacquisition. Existing public frame-to-generator coverage preserves CONTROL
capture with the same certified code. GC source peer is GO; IR final peer and
root combined generation/native checks remain pending. Host AST/whitespace
checks pass; no native execution occurred in this lane.
