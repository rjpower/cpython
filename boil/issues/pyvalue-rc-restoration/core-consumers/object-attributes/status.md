# Object-returning attribute bridges

- Stage: independent whole-file review requires finite corrections before commitment
- Authors: coordinator bridge/setter; wiki_audit final corrections; reviewer: state_audit
- Hypothesis: generic object-returning attribute access can use fallible owning
  boxes while preserving descriptor order, suppression and primary cleanup errors.

The reviewed two-function increment repairs _PyObject_GenericGetAttrWithDict and
_PyTypes_InitTypes with one local cleanup helper. Canonical class values stay rooted
through boxing. Dictionary retirement errors cannot fall through to descriptors
or disappear under attribute-error suppression. The borrowed cached object.__reduce__
identity comes from its immutable namespace after checked owning lookup; no new
owning cache lifecycle is introduced. All 139 other bodies remain exact to the
held baseline. The whole pending object.c delta needs composition review before
it can be committed.

At the initial two-function endpoint, full object.c TU and configured object
passed with zero diagnostics on
stable inputs. Twelve standalone host reference tests explicitly invoke
object-returning attribute APIs and pass without skips. The first host discovery
attempt failed before executing cases and remains in evidence. Twelve candidate
methods remain unexecuted; there is no linked restored interpreter.

[Design](../../../../../build/pyvalue-object-consumers-20261102-v1/design.md),
[independent design review](../../../../../build/pyvalue-object-consumers-20261102-v1/independent-design-review.md),
[scope](../../../../../build/pyvalue-object-consumers-20261102-v1/scope.json),
[actual native checks](../../../../../build/pyvalue-object-consumers-20261102-v1/native-v1.json),
[host controls](../../../../../build/pyvalue-object-consumers-20261102-v1/host-reference-v2.json).

Independent whole-file review accepted the getter/startup bridge but found
setter retirement, unused object-only GetMethod and repr-entry first-error seams.
The setter is corrected; wiki_audit removes the unused helper and repairs repr
cleanup. Earlier source/test hashes do not cover these mutable corrections.
Freeze/recheck/final independent composition review are required before commit.
[Finite findings](../../../../../build/pyvalue-object-consumers-20261102-v1/independent-code-review.md),
[setter amendment](../../../../../build/pyvalue-object-consumers-20261102-v1/setter-amendment.md).
