# Large call-window test-model port peer

Proposal direction approved; actual diff and coordinator validation remain pending.

The current `test_stack_too_deep_is_refused` encodes 100 arguments followed by a
101st argument on the next line. Its comment and expected ValueError explicitly
protect the former bounded stack projection/scan limit. That limit is not a
Python semantic reason to reject the jump. The admitted real-PRE recipe can cancel
partial argument preparation and restart the call from retained named local a,
with empty target scratch needs and no active saved-local obligations.

Replace this refusal expectation with a positive ownership/result contract:
one redirect from line3 to line2; one actual invocation; 101 arguments, every one
identical to the retained callable; exact sentinel return identity; valid captured
frame code, local a and PC bounds afterward. Bound LINE count to prevent another
replay hang. Do not merely accept either refusal or success.

Rename the live class to JumpFrameModelTestCase and update live imports/usages
without a fallback alias. Its description should explain real frame homes, owner
requirements and safe cancellation rather than reconstructing an operand stack.
Preserve the existing source set and other behavior tests. Sealed historical
receipts and review failures retain their original names/content.

## Actual diff disposition

Actual test-only diff approved. The renamed class retains its existing source
cases. The replacement test caps LINEs, sets the one-shot flag before redirect,
asserts success rather than catching ValueError, and checks one callee invocation,
101 identical callable arguments, sentinel result, retained frame code/local a
and bounded real PC. It restores the previous trace in finally. Runtime Lib/Tools
contain no old class name or fallback alias. `git diff --check` passes. Native
validation remains coordinator-owned; no C source changed in this port.
