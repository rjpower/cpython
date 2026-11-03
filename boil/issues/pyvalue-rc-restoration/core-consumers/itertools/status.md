# Itertools canonical result readers

Checkpoint53 `5b993d71b1e` ports product, combinations, combinations with
replacement, permutations, pairwise and zip-longest next functions. It removes
21 old getter expressions plus value-array conversion assumptions. Canonical
copies promote borrowed values; occupied writes detach actual owners, publish
replacement slots and retire old owners under the reviewed callback contracts.

Sol designed and independently reviewed the rules. Luna applied exact templates
and composed the six bodies and two unchanged control modules. Root executed the
native and copied-test gates; Sol independently reviewed actual code/results.
Root committed exactly the source and two tests, preserving held type changes.

- [Composition and commit](../../../../../build/pyvalue-itertools-next-six-composed-20261103-v1/commit.json)
- [Independent actual review](../../../../../build/pyvalue-itertools-next-six-composed-20261103-v1/independent-solport-composed-review.md)
- [Coordinator decision](../../../../../build/pyvalue-itertools-next-six-composed-20261103-v1/coordinator-review.md)
- [Native comparison](../../../../../build/pyvalue-itertools-next-six-native-20261103-v1/dependency-comparison.json)

The actual matched native probe improves from 27 errors/zero warnings to one
error/zero warnings. All 201 common dependencies are identical and stable. The
remaining error is the unchanged batched_next pointer-array assumption; no
full-TU or object pass is claimed. Its separate recipe is in design. The former
22-error inventory omitted four array errors and one batched error attributed
through header expansions; the raw failed probe remains preserved.

Both copied test modules pass actual discovery on Python 3.14.4: ten controls,
no skips. These are reference controls. The restored candidate has not linked,
executed these controls or received fault qualification. The shared zip cleanup
path includes the independently required pure Recycle amendment; inherited
iterator order is retained without a new recursive transaction policy.

Merged v19 includes this checkpoint and removes exactly 26 errors, leaving 254
and no executable. See the [merged build](../../merged-build/status.md) and
[porting queue](../../porting/status.md).
