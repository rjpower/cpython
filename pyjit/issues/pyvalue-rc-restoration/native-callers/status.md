# Selected generic native VM callers

- Stage: source committed within `c21d0a374ee`; integration/candidate execution pending
- Owner: coordinator; previous implementation/review assignments are closed
- Hypothesis: the ordinary/KW C-function routes can share the scoped engine
  while preserving actual caller publication, cleanup order and observer state.

The reviewed delta captures roots, prepares owned object views, stages/publishes
the canonical result, ends the scope, retires original owners in semantic order
and refreshes around one event poll. +1 fallback and observer redispatch do not
replay a body. Real-route fixtures assert both outer/inner generic KW handlers.

[Author receipt](../../../../build/worktrees/pyvalue-rc-universal/build/pyvalue-native-caller-source/final-v2/receipt.json)
`e56eeac2`; [independent review](../../../../build/pyvalue-rc-native-consumer-map-20261101-v1/native-caller-component-review-receipt.json)
`9db64497`. Seven generators, 46 host generator checks and three actual syntax
TUs pass. The 27 behavioral methods remain unexecuted; this is not whole-CALL,
parser/Clinic, instrumented, EX or opaque-backend closure.

Next: execute focused routes in the coherent candidate. The shared composition
checkpoint includes the exact reviewed increment. Broader families need their own minimum useful scoped increments.
