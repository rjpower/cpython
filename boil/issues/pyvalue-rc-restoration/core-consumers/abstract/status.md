# Instance and subclass consumer port

- Stage: design independently accepted; implementation active
- Author/implementer: coordinator design; state_audit implementation
- Code/results reviewer and integrator: coordinator

Four remaining tuple reads and the special-method callable bridge in abstract.c
still assume object-pointer storage. Use checked owning tuple views through
recursive predicates, retain a selected single-base child through both old-owner
closes, and materialize canonical special methods only at the C call boundary.
Preserve ordered short circuit, recursion accounting and primary/cleanup errors.

Exact source grant: Objects/abstract.c's value-header rename, abstract_get_bases,
abstract_issubclass, call_special_method, object_recursive_isinstance and
object_issubclass; one local first-error close helper; new
Lib/test/test_pyvalue_abstract.py. Keep all other functions exact, including
check_class/object_isinstance, whose adjacent cleanup/fault paths are separate
holds. No provider/header/index/shared-build grant. Root coordinates native
checks and commits only after independent code/results review.

[Design](../../../../../build/pyvalue-abstract-consumers-20261102-v1/design.md),
[independent review](../../../../../build/pyvalue-abstract-consumers-20261102-v1/independent-design-review.md).
Next: freeze implementation, check full TU/object under actual configured
headers, review results and commit the useful dependency. Host controls do not
execute the restored runtime; candidate witnesses await a linked interpreter.
