# Canonical dictionary read providers

- Stage: independently reviewed and committed as `99012f782ea49b7ec1f37eeafd7171cbedf02767`
- Owners: coordinator providers/contracts; state_audit fixtures; separate cross-reviews
- Scope: four read APIs, public contracts and five narrow FASTCALL fixtures

GetValue, GetValueString, ContainsValue and NextValue now have real definitions.
They reuse the existing canonical hash/known-hash lookup and cursor primitives;
no scalar boxes or alternate selection engine are added. Outputs are independent
owning snapshots. Pending errors return before callbacks; results remain owned
through query/dictionary retirement. Dict/frozendict errors retain their diagnostics.

The actual full Dict and fixture TUs pass with the new debug configuration.
The configured dictobject.o compiles with three preexisting unused-function
warnings. nm verifies four strong definitions and existing compiler/flowgraph
objects reference GetValue. All preexisting dictionary and fixture text is exact.
Two ordinary host controls pass; **13 candidate methods remain unexecuted**.
There is no merged executable, runtime qualification or performance result.

[Design](../../../../build/pyvalue-dict-value-readers-20261102-v1/design.md),
[design review](../../../../build/pyvalue-dict-value-readers-20261102-v1/independent-review.md),
[production review](../../../../build/pyvalue-dict-value-readers-20261102-v1/production-code-review.md),
[fixture review](../../../../build/pyvalue-dict-value-readers-20261102-v1/fixture-independent-review.md),
[composition review](../../../../build/pyvalue-dict-value-readers-20261102-v1/composition-independent-review.md),
[production checks](../../../../build/pyvalue-dict-value-readers-20261102-v1/production-checks.json),
[fixture checks](../../../../build/pyvalue-dict-value-readers-20261102-v1/fixture-results.json),
[commit receipt](../../../../build/pyvalue-dict-value-readers-20261102-v1/commit.json).

Next: execute the real fixtures after the candidate links. PopValue is still a
separate required mutation provider; this checkpoint closes only the read family.
