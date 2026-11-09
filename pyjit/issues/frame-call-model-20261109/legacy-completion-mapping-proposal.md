# Legacy completion adapter and payload construction

Proposal only; no runtime changes in this document. Modern completion capability
and legacy trace projection have separate contracts.

## Payload construction first

ObserveStep PRE currently calls PyErr_SetObject(StopIteration, boxed). This can
expand a returned tuple or reuse a returned StopIteration instance, so the event
payload no longer represents the actual completed C-owned value. Existing
_PyGen_SetStopIterationValue(value) explicitly constructs one argument and
preserves the exact tuple/exception value. Replace only that construction block,
add its direct internal header, then keep the existing raised-exception fetch,
dispatch, close and PRE-error route. The helper borrows and duplicates its input;
the held result owner and the commit protocol are unchanged. B source-peer
approved the helper direction; root has admitted this repair after native25.

The coordinator payload probe confirms incorrect modern payloads on candidate
AND upstream, with upstream legacy FOR preserving them on its other route.
Candidate SEND still returns the correct actual value. This is an observed
payload bug, not proof that all completion behavior differs from upstream.

## Proposed adapter projection after payload verification

Split the existing shared legacy RAISE/STOP registration into two private
callbacks. RAISE keeps sys_trace_exception_func unchanged. STOP's callback:

1. Validate the supplied code and word offset, then obtain the immutable word's
   handler through _PyCode_InstrHandler and normalize its family. Never infer
   the originating operation from the mutable frame PC after another observer.
2. Only for canonical FOR_ITER and an exact StopIteration instance whose
   delivered internal value is PyValue_NULL or None, return the ordinary None
   callback result without invoking legacy PyTrace_EXCEPTION.
3. Every other STOP delegates to the existing exception tuple/trace adapter.
   SEND, non-None FOR results and unusual/subclass payloads remain observable.
   Actual iterator-raised StopIteration uses RAISE and is never filtered.

This maps implicit valueless FOR completion to the legacy API, while modern
STOP remains enabled for None and non-None under existing independent masks.
It neither disables modern tools in combined mode nor modifies ObserveStep,
CompleteStep, frame ownership, ordinary dispatch or instrumentation tool IDs.
No duplicate events, synthetic instructions, alternate offsets or result-state
fields are introduced. The None predicate reads internal value bits, not an
arbitrary Python attribute getter. Unknown shapes delegate rather than omit.

The existing official sys.monitoring contract permits STOP_ITERATION and
RAISE(StopIteration) interchangeably, but this does not erase modern STOP(None)
capability or justify an extra legacy exception for implicit None exhaustion.
Upstream v2 modern-only reports both values; legacy/combined omit None. This
proposal intentionally keeps modern events independent when both APIs run.

## Identity, mutation and reentry

The adapter classifies the exact event payload delivered to it. It does not
claim to recover an immutable original result from mutable exception.value.
No new snapshot field exists. Current broadcaster order is highest tool first:
legacy reserved7 precedes public0..5, so an earlier public callback cannot
mutate the payload before legacy in this implementation. Tests must retain this
actual order rather than reorder callbacks to manufacture that case. A legacy
callback can mutate the shared exception before modern5 and modern0; later tools
see those mutations in existing order. If ordering changes later, delivered-
payload semantics still define this adapter without claiming original identity.

The omitted route calls no Python, allocates no frame and changes no cursor,
continuation or generation. The forwarding route retains existing callback error
and setter/reentry handling. A successful same-PC redirect remains governed by
the semantic observer's existing generation-aware transaction after broadcast.
No cached frame PC is restored by the adapter.

## Gates

First run probe-completion-payload.py before/after helper repair: FOR/SEND,
None,7,tuple2,returned StopIteration(None), cold/warm100, all three observer modes,
exception.value identity and actual SEND result identity. Keep modern STOP for
None, exactly once. Combined mutation must record existing legacy→modern5→modern0
order and delivered shared-object effects. Then test real custom iterator RAISE
StopIteration(None) remains visible, SEND None remains visible, non-None callback
errors preserve PRE owner/EH, and original tracing cases retain their event lists.

Only after payload receipts and independent peer may the adapter be implemented.
Tree-specific monitoring tests that asserted the old generic RAISE category must
adopt one STOP with exact payload under the documented equivalence; modern None
expectations must retain capability. No blanket event filtering is admitted.
