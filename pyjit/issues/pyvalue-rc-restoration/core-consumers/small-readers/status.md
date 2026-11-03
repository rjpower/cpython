# Tokenizer state and bytes tuple readers

- Stage: independently reviewed and committed as `639891a2e81`
- Author: coordinator; independent design/code/results reviewer: state_audit

The tokenizer validates decoder (bytes, int) state with canonical predicates and
an owned immutable tuple through its callback-free reads. Inline integers, bool
and heap integer subclasses keep their semantics. State cleanup preserves the
first error and reports decoder versus memory failure correctly.

Bytes prefix/suffix tuple matching holds each canonical candidate through buffer
calls. Invalid inline numbers raise the existing type error without boxing before
a raw-buffer comparison. First-match short circuit remains; cleanup errors cannot
become nominal success. The preexisting custom buffer/bytearray storage reentry
question is not certified by this finite change.

Two production bodies changed; all 39 other bodies remain exact. The existing
tokenizer fixture adds a direct real-provider helper, and its single existing
provider declaration has private exported visibility for the rebuilt extension.
All six old fixture bodies remain exact. Three full TUs and configured objects
pass without warnings on stable inputs; readelf confirms the decoder definition
is GLOBAL DEFAULT. Four host bytes controls pass without skips. Nine candidate
methods, including five decoder witnesses, remain unexecuted; no link/runtime pass.

[Design](../../../../../build/pyvalue-startup-small-readers-20261102-v1/design.md),
[design review](../../../../../build/pyvalue-startup-small-readers-20261102-v1/independent-design-review.md),
[code/results review](../../../../../build/pyvalue-startup-small-readers-20261102-v1/independent-code-review.md),
[native checks](../../../../../build/pyvalue-startup-small-readers-20261102-v1/native-v1.json),
[commit](../../../../../build/pyvalue-startup-small-readers-20261102-v1/commit.json).
