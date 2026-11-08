# Value-aware IMPORT_FROM path

The remaining attribute thunk owner at `0+0x40d079` resolves to
`Python/ceval.c:_PyEval_ImportFrom`, specifically the first
`PyObject_GetOptionalAttr(v, name, &x)` call. A module dictionary can contain a
small inline value, so converting that successful attribute result to
`PyObject *` is a real core box.

## Proposed split

Add an owned-value helper, declared in `pycore_ceval.h`, taking the source as
`PyValueBorrow` and returning `PyValue`. Its first operation is
`_PyValue_GetOptionalAttr(v, name)`: status 1 transfers the owned attribute
value directly; status -1 propagates the lookup exception; status 0 enters the
existing circular-import fallback. Refactor the existing error/fallback body
into a helper that assumes the requested attribute was absent. That helper
must not repeat the first lookup, because custom `__getattribute__` callbacks
make a duplicate lookup observable. Its successful module-table fallback is
an object result and is wrapped with `PyValue_FromObjectSteal`; all current
error construction, `__name__`/`__spec__` lookups, and cleanup order remain
unchanged.

The source need not be an object: an overridden `__import__` can return an
inline integer or float. The bytecode must guard
`PyLazyImport_CheckExact(PyValue_AsKnownObjectBorrow(from))` with
`PyValue_IsObject(from)` before taking that object view. The absent-attribute
path must keep the source as `PyValueBorrow`; it cannot pass an inline value to
`PyModule_Check` or silently box it just to reuse the old helper. Query
`__name__` and `__spec__` through value-aware optional-attribute calls as well.
For the conventional Unicode `__name__` and module-spec object cases, take a
known-object view only after checking the value tag and type. For inline or
other unexpected values, preserve the old unknown-module diagnostic path
without projecting the source. The formatting implementation needs a
value-aware repr path for arbitrary values and must not rely on the current
`%vR` formatter, which can box inline arguments.

Route ordinary `IMPORT_FROM` through the owned-value helper and regenerate the
IR cases from `Python/bytecodes.c`. The lazy-import branch must also tolerate
an inline `from` value before its exact-type check. Lazy replay currently has
an object-returning caller in `Objects/lazyimportobject.c`; retain an object
adapter for that caller while tracking lazy placeholder/result migration as
remaining core debt. Do not describe this compatibility adapter as zero-box
closure.

## Ownership and behavior checks

The status result owns its `.value` and must be closed on every non-transfer
path. The source `from` value remains owned by the bytecode stack across
attribute callbacks. The absent-attribute fallback must run once and preserve
its current callback order and error text. Tests should cover ordinary modules
with attributes holding inline integers; an overridden `__import__` returning
an inline value; a custom source whose requested attribute lookup counts
calls; circular-import fallback; missing attributes with and without
`__spec__`; and lazy-import replay. Compile the bytecode definition and
`ceval.c`, regenerate `generated_ir_cases.c.h`, and run focused import and
lazy-import tests at merged integration.
