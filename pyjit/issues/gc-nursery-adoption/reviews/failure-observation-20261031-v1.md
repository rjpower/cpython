# Coordinator review of the failure-state observation

2026-10-31 21:32 UTC. **Accept one bounded observation of the two exact failed
children**, using the sealed b34931a881b debug products. This authorizes
diagnostic preparation for root activation, not a corrected test, runtime
change, behavior pass or retry of the feedback batch.

The isolated `build/nursery-adoption-diagnostic-v1/` plan SHA-256 is
`d6a5f3d0c50aae9a9fb01015fede32229b62186621a56df0394aed38b8e9c20c`;
its held launch is
`57b05ccb816a46a4772e056867a87259f233319890e8f48d068383cebc324ab2`.
I read the hook, extractor, runner, literal commands and host control, then
independently reconstructed both original invocations from the preserved
failure log. Original child bodies, all assertions, historical bindings and
the two-line observer prefixes match byte-for-byte. The only additional
prefix installs the exception hook from the pinned helper directory.

The hook runs after an exception and reads existing traceback-frame globals:
snapshots, addresses, original/unrelated records and call count. It does not
call the collector, request new state or wrap GC/stress APIs. It chains the
original hook. Added prefix imports/metadata can perturb allocation layout
before the original initial full collection; the observation states this
limit and cannot rewrite the meaning of the first failure.

The runner requires exactly two exit-1 assertion-failure auxiliaries with
captured state. It compares initial full-record provenance with records later
at the same address and explicitly identifies a missing initial B witness.
Neither an expected failure nor a missing witness earns retirement/test credit.
An unexpected exit or missing observation stops the probe. Outcome review must
inspect the actual records and positive collection counters before drawing a
causal conclusion.

Source, build/configuration/products, tool identities and prior helper bytes
are guarded by the accepted v2 loader. Actual children assert/record retain,
N0, seed zero, isolation flags, executable/extension and observer origins/hash,
PID/parent and original-body digest. The unchanged accepted supervisor's row
body supplies process-group timeout, machine slot, build lock, terminal-before-
cleanup recording, exact idle observation and bounded verification. Changing
its in-process output directory does not edit the accepted helper.

Root activation must confirm an exclusive native window and bind this review,
plan and launch. Exposure is at most ten minutes, clamped to the original
**22:07:46.664152 UTC** deadline, with existing cleanup/verification reserves.
Final review must bind the outer gate/launch/verify/idle receipts, raw child
logs/states/roots and probe result/cleanup, not just the compact completion.

Independent host replay reproduced the synthetic exception capture and original
hook chaining, including distinct original and new origins at one address.
Only its exclusive receipt destination changed in memory. No target ran.
Receipt `build/nursery-adoption-review/diagnostic-helper-review-20261031-v1.json`
SHA-256 is
`4af0250abe20411bd78a8d7beb29feaae60bb5a9fdbcebd1338ec5080a039eb5`.
The source and sealed diagnostic helpers remained unchanged. No blocker remains
for root activation within these bounds.
