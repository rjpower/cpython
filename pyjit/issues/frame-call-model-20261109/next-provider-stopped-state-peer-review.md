# Next provider and stopped-state packet peer review

Design disposition: approve the refined bounded packet for root admission after
native29 integration/measurement. No implementation has been reviewed or
performed for this next packet. Reviewed the current
next-terminal-provider-implementation-plan.md and
stopped-state-first-increment-brief.md against actual instrumentation callers,
POP_ITER/jump/yield DSL and shared decoder.

The original event-derived edge rule had a concrete defect: POP_ITER reports
FOR BRANCH_RIGHT but owns a normal FALLTHROUGH source transition. The amended
single four-argument terminal marker resolves it. Reported source, immediate
physical next, public event and owner edge are independent coordinates. Both
metadata and runtime use the same explicit literal owner edge; supported
physical destination shape proves that edge. Existing providers/host fixtures
must all migrate, with no three-argument fallback. A positive RIGHT/FALLTHROUGH
POP_ITER case is required. Do not replace the old suffix inference with another
generic public-event/edge equivalence rule. Negative tests reject unknown
events, dynamic edges and edge/destination mismatches, plus provider-specific
reported-event mismatches only where that capability is actually proved.

No-output jump facts are structurally supported by the existing decoder:
zero writes do not imply a destination or a produced owner. POP_ITER's explicit
input retirement must cut consumed provenance while holding the iterator in C
through observation and preserving callback exceptions across its deferred
close. The source/EH site is actual POP_ITER, never its publicly reported FOR
source. Periodic checks remain before backward-jump observation. Ordinary
handler text must remain unchanged.

The four actual source-edge caller classes are correctly separated. Committed
Step POST, certified terminal observation, and a completed predecessor's landing
wrapper can publish source authority. Legacy inline jump hooks may have later
stores/closes and must mask authority while retaining source callback delivery.
Thread one explicit publication contract through source-edge delivery. The
scoped SOURCE_POST contract, not public event spelling or code offset, is the
setter's sole authority. Keep spec.next as the immediate real continuation;
validate a separately normalized source landing only using the existing bounded
relaxation adapter. Helpers remain real dispatched words with their existing
INSTRUCTION events.

B's first stopped-view consumer remains inherited-context, save-empty target,
no required target scratch and no token-creating transfer. A copied row does
not make register_context_normal pure: PUSH_EXC_INFO/LOAD_FAST_AND_CLEAR update
parent tables. First refuse those transitions, or later extract a separately
reviewed read-only transfer. Do not mutate Analysis parent state while deriving
an exact stopped edge. The same Plan/token/binding/commit machinery is reused;
new physical output facts cannot restore an old token. Global landing joins are
not substitutes for exact edge state.

The proposed YIELD_READY adapter is coherent as a separately gated provider in
the packet: original outgoing value remains frame-owned, callback payload is
independently held, and only unchanged success reaches heap/suspension links.
Typed redirect/error paths terminate before old return-slot/frame-state/chain/
recursion stores. Restore scopes before payload retirement, save callback error
across that retirement, and recheck generation and actual PC afterward. No
actual suspended-frame or caught-exit authority is implied. Host proof must
check this suffix barrier rather than reuse the scalar terminal marker claim.

File boundaries are agreed: A provider DSL/generator/scopes/yield adapter and
provider tests; B frameobject stopped-view/Plan consumer and context tests; H
shared physical owner declarations/decoder/constructor witnesses. Root owns
regeneration, merged native gates, source integration and performance. No new
hot-frame field, dispatch-time metadata walk, alias operation or shadow context
engine is required. Whole END_ASYNC_FOR and direct END_SEND/FOR remain explicit
later providers, with no silent legacy authority fallback.
