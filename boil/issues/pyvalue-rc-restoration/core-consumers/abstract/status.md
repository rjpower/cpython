# Instance and subclass consumer port

- Stage: independently reviewed and committed as `3296af9ba588274a2584b6e4c05e10ad37801ac9`
- Author: coordinator design; state_audit implementation
- Code/results reviewer and integrator: coordinator

Four remaining tuple reads and the scalar special-method callable bridge now use
checked owning views. The iterative single-base walk keeps a selected child
through retirement of both old owners. Predicate order, recursion accounting,
truth conversion and primary/first cleanup errors are preserved.

Exactly five existing functions changed plus one local close helper; 106 other
bodies and all other text remain exact except the admitted include alignment.
check_class/object_isinstance retain separate cleanup/fault qualification holds.

Actual full abstract.c TU and abstract.o pass with zero warnings on stable
configured inputs. Twenty-three host reference tests pass without skips:
twelve new methods and eleven unchanged controls. Twelve candidate methods
remain unexecuted; host boxed values do not prove canonical lifetime behavior.
The merged interpreter remains unlinked.

[Design](../../../../../build/pyvalue-abstract-consumers-20261102-v1/design.md),
[design review](../../../../../build/pyvalue-abstract-consumers-20261102-v1/independent-design-review.md),
[code/results review](../../../../../build/pyvalue-abstract-consumers-20261102-v1/coordinator-code-review.md),
[receipt](../../../../../build/pyvalue-abstract-consumers-20261102-v1/receipt.md),
[actual native checks](../../../../../build/pyvalue-abstract-consumers-20261102-v1/native-v1.json),
[commit](../../../../../build/pyvalue-abstract-consumers-20261102-v1/commit.json).
Next: execute candidate predicates/lifetimes and applicable fault checks when
the merged runtime links. This source acceptance is not runtime admission.
