# Merged scalar and generator integration review

Reviewed source: **I f6ef4a81a87**, before the separate polymorphic SHAPE
component. This is a bounded source review, not full lifecycle qualification.
No must-fix was found in this composition.

The scalar normal endpoint now acts as the primary transfer only when the
shared source graph is complete. It preserves untouched scratch facts and
does not turn an incomplete incoming shape into a complete one. SAVE checks
initialization and required non-NULL inputs; publication requires vacancy or
the same current value identity. Alias coalescing, vacate stamps and publication
identity checks remain in the same four-home replay. Unknown graphs use the
existing conservative transfer. Windows, frame switches and returns cannot
acquire this normal endpoint.

The stronger callback receipt preserves scratch identity only for complete
source-checked plain native-value operations. Its default remains conservative;
dynamic local identities still expire. The runtime observer-context check binds
the observed frame and event. An ordinary descendant finalizer cannot redirect
an active UNARY parent through the ancestor CALL recipe. General observer and
frame-switch callbacks retain their broader invalidation. The remaining chained
runtime witness failure is a synthetic image without a valid line; the proposed
physical-location correction preserves the active-call denial assertion.

The exact-tuple CALL_EX probe ends its borrow before the existing refresh. Its
nonexact conversion and error route retain owning snapshots. Generator entry
uses one parsed recipe for recipient selection and emission: FOR uses A,
SEND uses its actual continuation recipient. Child/parent continuation and
exception ownership are published before callbacks. The product change removes
only the erroneous FOR local/discard guard. Literal completion entries share the
existing completion body and preserve PRE/POST observation, detachment and
retirement; they add no result-steal shortcut.

Root reports both builds at 117 modules/79 shared and 84 completion/observer
methods per role passing. The two-pair f6ef/golden54 screen gives generators
0.949424, Richards 0.968734, DeltaBlue 1.022672, coroutines 1.065409 and
comprehensions 1.027499. Logging ranges widely. The generator improvement does
not isolate an individual component, and the regressions prevent selection.
Golden54 remains selected. See [the checkpoint report](report.md) for evidence.

Five subsequent opportunities follow from this source and evidence:

1. Measure literal completion entry text size, incoming stack arguments and
   remaining kind branches in optimized code. Source-level specialization alone
   does not establish a hot-path saving.
2. Remove exact-coroutine GET_AWAITABLE owner reacquisition with the same accepted
   state predicate and callback-free TAKE, preserving the generic fallback and
   current free-threaded state policy. This needs its own source and runtime gate.
3. Evaluate the separate managed-SHAPE PIC using actual eligible hit/miss counts
   and generic lookup removal. Historical feedback and monitored site visits
   establish applicability, not speedup.
4. Extend shared value transport through checked container guards and an atomic
   anchored read/duplicate. Expire the current token on unknown lvalue mutation;
   remove the old body recognizer in the same component.
5. Complete readable checkpoint and retained/released EH coverage through the
   shared storage graph. Current scalar authority does not certify windows,
   arbitrary allocation or complete M4. Keep membership, readable authority and
   retirement progress distinct.

The current abstractions are coherent: one source-effect grammar, one storage
action graph and one generator-entry recipe. Further increments should extend
those interfaces rather than add opcode-name exceptions or parallel parsers.
