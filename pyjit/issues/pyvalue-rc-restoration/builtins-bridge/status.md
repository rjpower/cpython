# Scalar builtins ownership bridge

- Stage: independent review accepted; included in dictionary commit `9b4ff426d1a`
- Owner: state_audit; coordinator reviewed design; independent final review required
- Scope: _PyDict_LoadBuiltinsFromGlobals and a focused new test file

Independent dictionary composition review found an unconditional object borrow
after canonical lookup. Inline scalar `__builtins__` values cannot use that
operation. Convert through a fallible owned object bridge, acquire the result's
builtins ownership before closing temporary owners, and preserve primary errors.
Retain module-to-dictionary normalization and defer unsupported lookup errors
until the language operation that needs a mapping.

[Design](../../../../build/pyvalue-builtins-bridge-20261102-v1/design.md).
Only the selected function changed. Five host reference cases passed; full Dict
syntax retained exactly the six known legacy-function failures, with none in
this function. The reviewer verified the masked delta and corrected hash.
Candidate execution remains pending.
[Results](../../../../build/pyvalue-builtins-bridge-20261102-v1/results.json).
