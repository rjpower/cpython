# Positional constructor owner scopes

- Stage: independently reviewed and committed as `be15f9856b5`
- Author: state_audit; reviewer/integrator: coordinator
- Hypothesis: the existing tuple owner scope can replace raw reads in fixed-arity
  positional constructors without adding another capture or ownership model.

The provider checks argument count before materialization. The Clinic producer
selects direct converters without resources, keeps owner declarations through slot
wrapper reconstruction, and closes owners after conversion/implementation cleanup.
Twenty production wrappers in eleven generated products changed; all eleven
handwritten inputs and 219 other generated wrappers remain exact. The increment
removes 84 raw tuple reads. Two real registered new/init fixtures exercise ordinary
and expanded call routes when the candidate becomes executable.

Double generation is idempotent. All 270 host producer tests and eight reference
controls pass without skips. Actual codeobject/classobject full TUs and configured
objects pass without warnings. Other available TUs fail in older unported paths;
four optional-module TUs lack bz2, SQLite or OpenSSL headers. All 117 nonfatal diagnostics map to unchanged before-function bodies; this is
source attribution, not a passing full-TU build. Six candidate methods remain
unexecuted; no restored runtime links.

[Design](../../../../../build/pyvalue-clinic-positional-design-20261102-v1/design.md),
[design acceptance](../../../../../build/pyvalue-clinic-positional-design-20261102-v1/coordinator-review.md),
[frozen scope](../../../../../build/pyvalue-clinic-positional-design-20261102-v1/scope-checks.json),
[actual native checks](../../../../../build/pyvalue-clinic-positional-design-20261102-v1/native-v1.json).
[Code/results review](../../../../../build/pyvalue-clinic-positional-design-20261102-v1/coordinator-code-review.md),
[diagnostic attribution](../../../../../build/pyvalue-clinic-positional-design-20261102-v1/native-attribution.json),
[commit receipt](../../../../../build/pyvalue-clinic-positional-design-20261102-v1/commit.json).
