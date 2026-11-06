# IMPORT_FROM value-aware ModuleSpec follow-up

Apply `w5-import-from-value.patch` first, then
`w5-import-from-modulespec-fix-v2.patch`, then
`w5-import-from-cleanup-order.patch`. The additive patch changes the internal
ModuleSpec origin helpers to accept `PyValueBorrow`, while preserving the
existing object helper signatures through small adapters. IMPORT_FROM now
distinguishes a missing `__spec__` from a present inline value and queries the
present value without boxing it.

`_PyModuleSpec_GetFileOriginValue` keeps the original operation order:
optional `has_location` lookup, truth conversion, then optional `origin`
lookup. It returns an owned Unicode value only when the origin is a Unicode
object. `_PyModuleSpec_IsInitializingValue` performs the optional
`_initializing` lookup and truth conversion. The IMPORT_FROM error path keeps
the source, spec, and returned origin alive through subsequent callbacks and
message formatting.

`test_from_import_inline_spec_value` covers an overridden importer returning
an object whose `__spec__` is the inline integer `42`. The upstream control
reports an unknown-location `ImportError` for that case. Sol's source review
found no must-fix issue. D syntax compilation was attempted for
`Objects/moduleobject.c` and `Python/ceval.c`; both stop at unrelated,
pre-existing marked-API violations elsewhere in those translation units, with
no diagnostic in the new helper or IMPORT_FROM hunks. The merged runtime still
needs focused import tests and integration compilation.

The cleanup-order follow-up preserves the upstream finalizer order on an error:
origin, spec, then module name. Its test uses an ephemeral Unicode subclass
for `__name__` and an ephemeral spec whose `has_location` truth conversion
raises `TypeError`; upstream observes `spec` finalization before `name`.
