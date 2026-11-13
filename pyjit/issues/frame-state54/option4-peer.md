# Option 4 independent peer review

Approve the proposed bounded implementation under the explicit preparation,
association, tracking and retirement contracts. Source/proposal review only;
no runtime execution or native validation by this reviewer.

The revised interface is coherent: normal Initialize remains consuming and
nonfallible, supplied mapping entries prepare an empty untracked PyFrameObject
before reservation, and the commit moves mapping/object/function owners only
after failure can no longer require an unpublished-frame rollback. Preparation
failure must leave args/locals/func to their existing failure block; reservation
failure additionally destroys only the empty prepared object. Initializing its
f_frame=NULL is essential for that safe destruction.

Reuse the already allocated PyFrame_New object; do not invoke lazy observation
or recursively allocate from Initialize. Existing-object publication must still
materialize pending kwargs at the observer boundary, not during preparation.
A mapping-bearing attached object must be tracked after complete association:
visiting its new mapping only in frame_traverse is insufficient if it remains
untracked as selected lazy records sometimes do. Keep the mapping visit before
the external-frame early return, and remove its old interpreter-frame edge.
This yields one counted/visited owner location rather than duplicate edges.

Unique frame-object retirement correctly needs an explicit C mapping owner:
detach and NULL before object destruction, retire homes, then mapping, then
function, retaining executable geometry through callbacks. Escaped takeover
instead leaves mapping on the surviving object and moves its complete homes
and specials. Owner-dependent containing-object conversion must be defined once
and asserted; never manufacture a self-owning frame_obj for FRAME_OBJECT.

Copy must update association at the existing commit and clear the moved source
frame_obj. Review generator creation and PyFrame_New-to-generator conversion
individually: the latter needs an explicit new generator owner of f because
its embedded source frame deliberately has frame_obj=NULL. Callback/error
paths must not observe a tracked object with f_frame=NULL or two owners both
claiming the same interpreter-frame mapping edge. Keep existing free-threaded
acquire/CAS/critical-section rules.

No unresolved proposal blocker. Implementation review must inspect actual
producer failures, object traversal/clear and all globals==locals identity
consumers. Added eager variable-tail allocation on class/module/exec paths and
clear bookkeeping are material performance costs; header shrink alone cannot
justify admission. Root's focused semantic and broader timing gates remain
required.
