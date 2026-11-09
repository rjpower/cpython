# Value-aware IMPORT_FROM

The raw attribute-thunk PC `0+0x40d079` resolves to the first optional
attribute lookup in `Python/ceval.c:_PyEval_ImportFrom`. The packet adds
`_PyEval_ImportFromValue`, which returns the owned result of
`_PyValue_GetOptionalAttr` directly. An absent result enters a shared fallback
without repeating the requested lookup, preserving custom `__getattribute__`
callback count/order. `__name__` and `__spec__` are read as values; only a
checked Unicode module name or an object-valued spec is projected. An inline
spec is an exact int/float with no custom attributes, so the existing
`has_location` and `_initializing` optional queries are absent without boxing.

The `IMPORT_FROM` bytecode uses this value result, and guards the
`PyLazyImport_CheckExact` view with `PyValue_IsObject`. The existing object
`_PyEval_ImportFrom` adapter remains for `Objects/lazyimportobject.c`; lazy
replay is tracked as remaining core work, not a permanent compatibility
boundary. The bytecode change regenerated `Python/generated_ir_cases.c.h` and
`Python/ir_targets.h`.

Regression tests cover an inline-valued module attribute, an inline result
from an overridden `__import__`, and one lookup of a requested missing
attribute. Source review approved the status/ownership split and these edge
conditions; see the canonical design
[proposal](import-from-value-design.md).

Patch baseline: isolated source
`build/checkpoint-bulk-20261105` at `e983958bedeca23a192f3a566d2f27b4a0c9a7db`.
The exact patch was generated with `git diff` and reverse-checked against the
edited source using `git apply --reverse --check`.

`Python/compile.o` and `Python/codegen.o` compile in the isolated D build. The
`ceval.o` compile command reaches this function but fails at 14 unrelated
pre-existing calls elsewhere in `ceval.c` that the marked API checker declares
unavailable. The direct `bytecodes.o` target cannot compile the DSL source as
ordinary C; generated handler files were regenerated successfully. No runtime
test was run on the isolated tree; the coordinator owns merged import and
lazy-import test execution.
