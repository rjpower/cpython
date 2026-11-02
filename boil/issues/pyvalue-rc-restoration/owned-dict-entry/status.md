# Owned dictionary entry bridge

- Stage: source committed within `9b4ff426d1a`; integration/candidate execution pending
- Owner: coordinator; previous implementation/review assignments are closed
- Hypothesis: rebuilt consumers can iterate inline-capable dictionaries through
  fallible, owned snapshots instead of temporary borrowed boxes.

Two new providers and declarations return -1/error, 0/end, 1/entry. Canonical
owners precede boxing; outputs follow cleanup. No cursor write follows final
dictionary-pin release, and failure cannot undo nested cursor progress.
All preexisting Dict/header contracts remain exact in this additive increment.

[Author receipt](../../../../build/pyvalue-dict-owned-entry-source-20261102-v1/final-v1/receipt.json)
`0ef3ace1`; [independent review](../../../../build/pyvalue-rc-native-consumer-map-20261101-v1/dict-owned-entry-component-review-receipt.json)
`c43dd512`. Fixture syntax passes; 32 behavioral methods remain unexecuted.
Full Dict still fails six retained borrowed bodies. That boundary and all
affected core/bundled consumers remain [separate work](../core-consumers/status.md).

Next: port concrete consumers and test snapshot/OOM/
reentry behavior on the integrated candidate. Compatibility approval is not
a dependency.
