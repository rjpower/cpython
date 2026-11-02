# Canonical dictionary read-provider closure

- Stage: design accepted; implementation and focused checks
- Owner: coordinator providers/contracts; state_audit append-only fixtures
- Scope: four declared read providers, contracts and narrow FASTCALL fixtures

GetValue has active interpreter/compiler/object callers but no definition.
GetValueString, ContainsValue and NextValue are also declared-only. Implement
this read family through existing canonical providers, with owning output
snapshots through hash/callback/cleanup and no scalar boxing. PopValue mutation
closure remains a separate required task; no complete public value API claim.

[Design](../../../../build/pyvalue-dict-value-readers-20261102-v1/design.md),
[accepted independent review](../../../../build/pyvalue-dict-value-readers-20261102-v1/independent-review.md).
Next: implementation, actual TUs/object compilation,
symbol-definition checks, focused controls, independent review and commit.
The missing definitions are link dependencies; syntax alone cannot close them.
