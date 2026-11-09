# Independent outcome review: two retirement observations

2026-10-31, completed at 21:36 UTC. **ACCEPT the bound diagnostic evidence
and its limited provenance interpretation.** Both children remain expected
failures. This gives no passing-test, runtime-source or qualification credit;
the original focused batch remains failed and stopped.

Source is unchanged and clean at
`b34931a881b33b47a2b2f4b1b2a644c50fed963b`. Reviewed directory:
`build/worktrees/nursery-adoption/build/nursery-adoption-diagnostic-v1`.
The plan SHA-256 is
`d6a5f3d0c50aae9a9fb01015fede32229b62186621a56df0394aed38b8e9c20c`;
launch SHA-256 is
`57b05ccb816a46a4772e056867a87259f233319890e8f48d068383cebc324ab2`.
The actual activation binds the coordinator's preparation review
`e53b67fe2f964052c7653047fce7523050851353b925b43869fb7305d34a0d60`.
Result SHA-256 is
`ade9e44e45f664174641ef8b98b0d4b20deee7215999a3d54d9aac80659ecada`.

The independent host receipt is
[`build/nursery-adoption-review/observation-outcome-20261031-v1.json`](../../../../build/nursery-adoption-review/observation-outcome-20261031-v1.json),
SHA-256 `8e4917acc2646c4aeb3677508c5f8a29585be5e74b57cdd4acb947c6e77a2f5c`.
It rehashes the plan/helpers, original failure log, build/product/configuration
bindings, activation/review, completion/result/cleanup, both original/probe
bodies, raw logs/child states/roots, exit/observation records, and every outer
`two-case-observation.*` launch/gate/verification/current-cleanup receipt.

I independently reconstructed each original failed argv from the preserved
first-feedback log. The original observer prefix and child body are byte
identical; the only added prefix installs the exception hook. Raw marker
records reproduce the saved observations. Each observed child returned 1;
the outer observation and verifier returned 0. Actual child PIDs, parent
bindings, retain mode with no extras, N0, seed zero, observer origin/hash,
target executable and sealed `_testinternalcapi` product all match. The two
children record 37 collections in total, zero fallbacks and zero unpinned
reports. They are failed auxiliaries, not successful tests.

For the free case, the first snapshot contains the target record
`(2199062537424, age=0, origin=11)` plus an unrelated origin-3 record. The
second snapshot contains only that unchanged unrelated record. Sequence is
11 in both snapshots: the target diagnostic disappeared without another
collection. The hint correctly remains 1 because a record remains. The
failure is the original assertion that the *entire initial ledger* consists
only of the target. The initial B witness is present in this observation;
the earlier possibility of an absent witness is not its explanation.
Last-record removal and hint 1-to-0 are **not covered** by these snapshots.

For resize, the target is
`(2199062536976, age=0, origin=12)`. Its original record survives the same-size
and failed realloc snapshots. After moving, that target and its origin are
absent, while both unrelated records remain unchanged. The original
post-move address-absence assertion therefore passes in this observation.
The child instead fails the final assertion that the complete ledger is
empty after a major. The hook did not capture the final `state` dictionary,
so it does not identify every entry at that final failure.

This changed resize failure location is a material limit. The added imports
and hook metadata can perturb allocation layout before the unchanged body
starts. These observations do not reproduce or explain conclusively why the
original unprefixed resize run failed its earlier address-only assertion.
They establish target retirement in this observed execution. They do not
prove original-address reuse or rule out every defect in the original run.
Any correction should compare the selected allocation's provenance and retain
unrelated-ledger controls; it must preserve or replace the missing last-record
hint control explicitly.

The outer run finished in 8.708 seconds at 21:31:38 UTC; verification finished
at 21:31:40. The local deadline was 21:41:29.625640, within the unchanged
22:07:46.664152 original batch ceiling. Recorded cleanup found no live
targets, and the independent audit acquired the build lock and found it idle
again. No additional target, compiler, build or reproduction ran for this
review. All native evidence here comes from the coordinator's single
authorized two-case observation.
