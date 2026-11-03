# Canonical code generation metadata

- Stage: independent review accepted; committed as `ebc9740c4d5f311cdd54d2c113566fe914422a5d`
- Owner: coordinator implemented; wiki_audit reviewed actual code/results
- Scope: Python/codegen.c and Lib/test/test_pyvalue_codegen.py

Canonical metadata copies and scalar reads replace removed boxed accesses. Both
dictionary loops retain owning keys; per-name opcode work cannot leak outer
iterator owners through early-return macros. Object conversion is bounded to
the native AST-pointer and compiler-constant interfaces.

Actual full-TU syntax passed with 305 stable header inputs; six host Python3.14
semantic controls passed. The new debug build also produced Python/codegen.o.
Candidate cases remain unexecuted. Existing documented unrelated macro-return
mangled-name leaks are not claimed fixed.

[Design](../../../../../build/pyvalue-codegen-consumers-20261102-v1/design.md),
[results](../../../../../build/pyvalue-codegen-consumers-20261102-v1/results.json),
[review](../../../../../build/pyvalue-codegen-consumers-20261102-v1/independent-review.md).
Next: execute focused tests in the coherent merged candidate.
