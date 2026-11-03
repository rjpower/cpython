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

The six-body matched probe improved from 27 errors to one unchanged batched
error. Checkpoint54 `a714cb97c14` closes that writer with the independently
reviewed sole-tuple-owner/consuming-resize recipe. The final full module and
actual object compile with zero errors or warnings; all 201 common dependencies
are identical and stable. Four new batched reference controls pass without skips.

- [Batched commit](../../../../../build/pyvalue-itertools-batched-composed-20261103-v1/commit.json)
- [Independent final review](../../../../../build/pyvalue-itertools-batched-composed-20261103-v1/independent-solcore-composed-review.md)

The former 22-error inventory omitted four array errors and one batched error
attributed through header expansions. The original failed probe is preserved;
the clean later probe does not relabel that failure.

Both copied test modules pass actual discovery on Python 3.14.4: ten controls,
no skips. These are reference controls. The restored candidate has not linked,
executed these controls or received fault qualification. The shared zip cleanup
path includes the independently required pure Recycle amendment; inherited
iterator order is retained without a new recursive transaction policy.

Merged v19 includes53 and removes 26 errors; v20 includes54 and removes the
last module error. The latest merged result is owned by the board. See the [merged build](../../merged-build/status.md) and
[porting queue](../../porting/status.md).
