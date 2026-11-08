# Additive activation-lifetime clarification

This note leaves reviewed proposal7c598fd92dd6ebdf2515a5bd3f6601b0bf0238d430463d905175707e3858d629 unchanged.

For Python EnterFrame, returning the Action unwinds the native machine activation. Copy the logical caller return-word and clear borrowed DecodedOp/old-CodeUnit pointers before dropping that activation's pin. No native return address is saved in the Python frame. Returning from the callee resolves a fresh valid caller CodeUnit at the logical post-call word; an invalidated caller falls back to the existing interpreter continuation.

For a C helper or extension callback that returns directly into an existing native activation, keep its CodeUnit pinned across the callback and any GIL release. Invalidation detaches future entry links and requests an after-effect exit; it must not unmap the active native return address.

The author had written these sentences into a successor46a11 draft while the peer was sealing7c. That draft and its original receipt remain preserved as proposal-lifetime-draft-v2.md and proposal-receipt-superseded-v1.json. The reviewed7c bytes were restored at proposal.md; this additive note is the final clarification. No source or execution authority follows.
