# Terminal observer commit peer review

Design review during corrected native28 validation; no runtime edits or native
worker jobs. Reviewed `terminal-observer-commit-proposal.md` against the current
generator final epilogue and instrumented OR_POP/FOR paths.

The proposed final-output barrier addresses the actual owner bug. Current
`generate_handler` publishes final storage and window-to-A movement only after
the semantic body. OR_POP and FOR observe before that publication; callback error
can bypass an owned item, while redirect can be followed by an old store. A
terminal generator marker, one commit per reachable path, and no later generic
epilogue are the correct structural changes. Defined NULL must remain distinct
from undefined output. Ordinary output text should remain unchanged outside the
observed providers.

The concrete owner accounting is suitably bounded: one destination owner for
either displaced local or NoDst result, explicit statically counted transferred
input owners, and direct generated closes. Capture the displaced owner before
installing the new local in a callback-free block. Publish all homes and the
continuation before observation or deferred close. On error, preserve the raised
callback exception around cleanup with Get/SetRaisedException. The latest
redirect must determine actual PC/EH after cleanup; no cached destination or old
slot store can follow it.

One API obligation remains unresolved in the reviewed proposal. Capturing the
activation and its source-error `(frame, record)` pair does not identify a recipe
installation. A close-time callback can potentially install a new recipe with
the same tuple. Equality would then incorrectly authorize deleting the newer
recipe, contradicting the promised replacement preservation. Require a cold
installation identity or a positive exclusion proof for the admitted callbacks;
redirect generation alone identifies intent, not recipe installation. This is
not a request for another unwind engine or interpreter-frame hot field.

Encoded POST EH validation must accompany provider admission. The existing
completion-only constructor proof does not certify every ordinary branch or FOR
body after item/local publication. Scoped POST debugger permission remains a
later consumer of the same committed provider facts, rather than a prerequisite
for reproducing or repairing the owner leak. Retain the original event lists and
test collectability after explicit collection, without imposing prompt destruction
as Python semantics.

## Revised invalidation interface

The revised design resolves the tuple collision without an installation counter.
A cold `_PyEval_InvalidateSourceErrors(tstate, frame)` may walk the existing
active evaluation-activation chain and clear only entries whose pending
source-error frame is the exact redirected frame. Those entries all predate the
actual committed redirect. The chain is linked from live evaluation C scopes;
the helper neither allocates nor calls Python. Unrelated frames remain untouched.

Call this in ordinary and identity redirect commitment, before the first
detached-owner close. A pending-child setter publishes only a future request;
child failure can cancel it. That request must leave old recipes intact until
`_PyFrame_CompleteRedirect` actually publishes the destination after child
retirement, before its temporary cleanup callbacks. Failed setters and cancelled
requests do not invalidate recipes. No post-close clear remains, so a newer
same-tuple installation survives.

Disposition: revised API approved as a bounded design. Root must admit the shared
setter/frame/ceval hooks before edits. Actual generator path/output/error proof,
encoded POST constructor validation and scoped observer records still need
source review and native qualification; this is not whole-model admission.

## Final generator closure source review before native29

Reviewed the actual terminal marker emitter and checker after the bounded fixes.
The checker matches dotted owner names as token sequences for both live and
spent owners, rejects opaque uses and any exit while a deferred owner still
needs the terminal drain, and retains per-path defined-output checks. The emitter
uses typed error status as authoritative, asserts the callback exception exists,
preserves it across deferred closes, then reloads the latest frame PC. Close-only
value-record use remains non-boxing. These narrow fixes close the concrete source
findings; reported host mutation checks and unchanged ordinary-handler hashes
support their intended boundary. No further concrete blocker was found in this
source pass. Merged generated/native owner and event tests remain required.
