# Marshal container consumers

- Stage: independently reviewed and committed as 69a1b3ea5330a6381a6caf374112b5b1b5a43450
- Author: coordinator; independent review: wiki_audit
- Scope: Python/marshal.c and one focused test file

Recursive tuple/list/dictionary/sorted-set reads retain owning snapshots through
callbacks. Checked reference-list insertion propagates failure to all callers,
including slice decoding. TYPE_REF transfers one owner; code/slice cleanup
discards successful results if cleanup raises. The existing tags/versions/schema
and canonical containers remain; no boxed storage mode or new selection engine.

Full marshal TU and actual object compile pass with no warnings; all 305 header/
configuration inputs and source stayed stable. Nine ordinary host controls pass
with zero skips. The initial reserved-reference control mismatch is retained;
the corrected slice vector tests a placeholder reserved on both versions.
Thirteen candidate methods, including mutation/frozendict cases, remain unexecuted.

[Design](../../../../../build/pyvalue-marshal-consumers-20261102-v1/design.md),
[design review](../../../../../build/pyvalue-marshal-consumers-20261102-v1/independent-design-review.md),
[code review](../../../../../build/pyvalue-marshal-consumers-20261102-v1/independent-code-review.md),
[focused results](../../../../../build/pyvalue-marshal-consumers-20261102-v1/focused-results.json),
[final native checks](../../../../../build/pyvalue-marshal-consumers-20261102-v1/native-final.json),
[commit](../../../../../build/pyvalue-marshal-consumers-20261102-v1/commit.json).

Next: execute candidate marshal/import/reference tests after the merged runtime
links. Allocation-fault qualification and remaining consumer ports are separate.
