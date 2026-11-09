# Nonneutral committed POST source peer

Disposition: **source approved for root's native gate**, on the stable
`Objects/frameobject.c` and five `Lib/test/test_code.py` methods. This is not a
build or behavioral qualification. A's separate six-read borrow correction in
`Python/legacy_tracing.c` is also source approved.

The stopped-state factory accepts a normal POST only through the existing
observer scope, exact site/edge/physical successor, generated outcome and
shared observed-home decoder. The new request-owned row copies site context,
saved-local columns and origins; `register_context_normal()` runs on that copy.
The only transfers that mutate shared parent tables, PUSH_EXC_INFO and actual
LOCAL_MOVE token creation, are excluded. Consumed old origins remain cut even
when the home contains a replacement result. The Plan admits cancellation only
for inherited context, empty save columns and a target with no incoming
temporary-owner need. It retains PRE, neutral JUMP and stricter adapters.

An early draft passed an unbound OTHER phase to the source-only target loop
after moving row binding out of `register_resume_plan()`. That would have made
its saved/origin pointers NULL. The final code binds a separate conservative
PRE/OTHER site row; nonneutral POST does not use source-only fallback. Warning
preparation discards old borrowed rows, recaptures the scope and binds a fresh
analysis before committing. Close-time callbacks retain the existing
transaction order.

The focused tests assert the instrumented conditional route, FOR backedge
requirement and inlined LOAD_FAST_AND_CLEAR witness. They cover positive
committed redirection, owner and saved-binding refusal, close-time nested
redirection, and warning-time recapture. `git diff --check` passed in this
source review. The first native check reached the positive and callback cases,
but the two negatives registered BRANCH_LEFT while their actual true
conditionals emit BRANCH_RIGHT. Root's encoded probe showed src7→9 and
src12→14. The test-only correction registers RIGHT for those two cases and
asserts a taken successor; it retains their refusal and frame-state checks.
Root owns the corrected native rerun and integration; this review does not
claim those results.
