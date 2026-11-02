# Startup type constructor owners

- Stage: single-function source applied and independently reviewed; pending held typeobject composition
- Author: state_audit; coordinator applies/reviews; wiki_audit independently rechecks

The actual type_new argument boundary uses canonical fixed-field checks and the
existing owning tuple scope. Exact count/type diagnostic order is preserved before
materialization. Valid name/bases/namespace inputs are heap objects within inline
scope capacity. Status-specific derived-bases ownership prevents borrowed/doubled
cleanup. Selected inputs, keyword dictionary and result survive all callbacks and
first-error retirement; no metaclass/MRO effects replay.

Only the named function changed at its application point; all surrounding text
was exact to core1. Twelve host controls pass without skips; three dedicated
candidate methods remain unexecuted. The actual full typeobject TU still failed
with 49 errors/39 warnings, all 88 in unchanged bodies and source lines, with none
in type_new. This is a pending function delta, not a source commit or runtime pass.
Later graph work changes the whole-file hash without changing this named body.

[Design](../../../../../build/pyvalue-startup-type-new-design-20261102-v1/design.md),
[coordinator review/grant](../../../../../build/pyvalue-startup-type-new-design-20261102-v1/coordinator-review.md),
[application](../../../../../build/pyvalue-startup-type-new-design-20261102-v1/applied.json),
[independent code/results review](../../../../../build/pyvalue-startup-type-new-design-20261102-v1/independent-code-review.md).
