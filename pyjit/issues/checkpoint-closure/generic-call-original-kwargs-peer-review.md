# Generic callable original-keyword review

Reviewed `r27-generic-call-original-kwargs-v2.diff` and the composed call-site hunk in `r27-combined-call-original-kwargs.diff` against the current runtime. The v2 placement of `_PyType_IsGenericCallPair` after `slot_tp_call_value` fixes the v1 declaration-order compile error.

The branch selects only the exact generic object/value `tp_call` pair when the instance has no active value or object vectorcall. It scans borrowed dictionary keys without invoking Python, then calls the existing object slot with the original tuple and dictionary under the existing recursion guard. That preserves the observed descriptor lookup before keyword-key rejection. An inline callable value now reports not-callable before keyword unpacking. The resulting object-to-value bridge is still core debt; it is not an external boundary.

The composed branch order is sound: the ordinary object-only slot runs first, the selected generic pair second, the selected type-call continuation third, and normal value vectorcall last. Type objects use the metatype call pair, so the generic branch does not intercept the AST/type-specific branch. No further source blocker found. Linked integration must still run the direct C `PyObject_Call` descriptor-order and inline-noncallable differential probes; the v2 source has not been linked in this review.
