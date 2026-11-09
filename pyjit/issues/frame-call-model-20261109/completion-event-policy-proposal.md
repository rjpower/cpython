# Semantic completion event policy

Proposal only during native24 freeze. Existing event assertions are unchanged;
no runtime or test patch is admitted by this file.

## Evidence and API latitude

Candidate23 v2 probe separates legacy-only, modern-only and combined tools,
cold0/warm100. FOR generator returnNone always generates artificial STOP when
modern enabled and a legacy exception when tracing is enabled. Return7 generates
exactly one completion notification for each enabled observer. The previous
matched combined upstream probe emits no completion for None; for7 it emits a
legacy exception without the modern STOP callback. Modern-only upstream v2 is
still required to characterize that route independently.

The official [STOP_ITERATION contract](https://docs.python.org/3/library/sys.monitoring.html#the-stop-iteration-event)
(and local Doc/library/sys.monitoring.rst) explicitly permits STOP_ITERATION or
RAISE with StopIteration as equivalent generated completion notifications. Thus
moving non-None completion from the old generic RAISE implementation to unified
STOP is an allowed API choice. It does not justify duplicate callbacks or an
extra legacy exception for a valueless FOR completion. Callback payload,
exception identity/value, propagation and disable/restart behavior still matter.

ForLoopMonitoringTests currently documents a tree-specific generic RAISE route
and asserts its exact category for return5. Its None expectation is no event.
These two assertions have different meanings: the category is an implementation
choice explicitly allowed by the monitoring API; valueless exhaustion is also
visible through existing legacy tracing expectations and must have a deliberate
shared policy. Do not weaken either by accepting arbitrary counts or filtering
all StopIteration.

## Proposed shared policy

| Outcome | Completion observation |
| --- | --- |
| FOR generator/coroutine completes without a meaningful return payload (None) | No artificial completion exception/event; branch/source completion still runs. |
| FOR generator/coroutine returns a non-None payload | Exactly one artificial STOP_ITERATION with that value through the shared PRE observer. |
| SEND completes, including None | Existing artificial STOP_ITERATION policy, because this is a value-producing delegation result. |
| Real iterator operation raises StopIteration, including None | Existing real RAISE path before it is consumed; never suppressed by the artificial-completion rule. |
| Ordinary non-generator iterator exhausts without an exception | No fabricated generator completion event. |

Apply the None decision at the semantic FOR PRE boundary, identically for generic
iternext and inlined Python-generator return. Do not inspect test names, code
lines, old handlers or a legacy tool ID. Do not discard the distinction only in
the legacy bridge: that would leave tools observing different semantic outcomes
for the same operation. Payload disposal remains owned by CompleteStep and its
result/window commit; this rule only selects whether an artificial observer is
called. POST branch/source events remain intact.

This is a selected consistent runtime policy, not a claim that Python requires
one historical decref/event sequence. The modern-only upstream receipt must be
reviewed before admission; if its optimized route reports None STOP, record that
as an implementation difference and decide explicitly whether the documented
API permits the proposed valueless omission. Do not claim the matched screen
already proves all modern configurations.

## Minimal implementation if admitted

In ObserveStep's phase0 decision, return the ordinary successful PRE result for
kindFOR and canonical None before constructing an artificial exception. Keep
SEND/THROW, real iterator-error handling, non-None payloads and all owner-state
logic unchanged. No new frame field, handler, per-dispatch metadata or generic
callback filter. This patch is not the larger performance question about
CompleteStep's unobserved commit/publish cost.

Update the tree-specific value-slot non-None expectation to exactly one STOP
with the same type/value, and rewrite its current-contract comment accordingly;
retain its no-event None assertion. Keep original sys_settrace expectations and
monitoring implicit-return exact-once test. Update the current value-API design
row only after admission; sealed historical evidence stays unchanged.

## Gates before acceptance

- Matched modern-only/legacy-only/combined cold/warm receipts, including None,
  non-None and explicit iterator-raised StopIteration(None).
- Generic versus actually selected inlined generator entry, with stable code
  per fixture; SEND/yield-from None and payload results independently covered.
- Exact type/value and exactly-once counts; two tools, local+global enable,
  DISABLE/restart and tracing reconfiguration preserve independent masks.
- Non-None completion callback raises: propagate it with PRE ownership and EH
  intact; no duplicate RAISE plus STOP. None rule does not call that callback.
- Original five tracing cases, value-slot tests, monitoring, generator/coroutine,
  semantic completion/source-error and observer-reentry suites remain gates.

Root/B peer selects the policy and tests before source edits. Immediate debugger
entry and missing source LINE repairs remain higher priority.

## Corrected upstream v2 evidence and admission status

The coordinator had already produced
`ownership23-upstream-completion-events-v2.log`; the earlier pending statement
used the wrong filename. Its modern-only route emits exactly one STOP for
both None and7, cold and warm. Legacy-only and combined modes omit None; for7
they report one legacy exception and no modern STOP. Consequently the proposed
uniform None suppression would remove a modern-only capability that upstream
actually provides. It is not admitted and must not be presented as a behavior
preservation patch.

The next design decision must distinguish modern STOP capability from the
legacy trace bridge's None-completion policy, using the shared semantic PRE
outcome and existing independent tool masks. Such a distinction need not copy
upstream's combined-tool suppression: modern tools can retain their event while
the legacy adapter omits only its artificial valueless exception. No event code
or expectation was changed in this review. Root/B must review that narrower
bridge policy before implementation.
