# Native scope and one real call route

The user requested an opaque extension API, a slower bridge for rebuilt source,
and gradual adoption, without hard compatibility. The coordinator accepts the
[reviewed extension design](../../design/extension-api.md) as the direction and
authorizes its first narrow source slice on the shared `95be1d28`-based universal
value worktree. This is a new implementation, not execution of the superseded
dictionary API-removal script.

Independent design receipt:
`build/pyvalue-rc-native-consumer-map-20261101-v1/opaque-api-design-review-receipt.json`,
SHA256 `adf7dc2c7a8a46ecef562077c959865e8f86ebbcc3dd4f8b7c7217769d7ed27d`.
It binds design SHA256 `73038166530bb209112a4011547bcbe237bf496141c496439f7015934afdff03`
and detailed proposal `a09b5a1371664093423a5196e01516e2224cf1d882d68e8f1aaa50f6493d3dbb`.
Those immutable reviewed bytes remain unchanged.

## Ownership and source scope

coroutine_design_review implements:

- New `Include/internal/pycore_native_scope.h` and `Python/native_scope.c`.
- Only the new `_PyCFunction_CallScoped` function in `Objects/methodobject.c`.
- New `Modules/_testinternalcapi/native_scope.c` and
  `Lib/test/test_pyvalue_native_scope.py`.

restart_audit retains all other methodobject functions, the real bytecode routes
and generated products, and the narrow Makefile/test-registration additions.
The first actual adoption is `_CALL_BUILTIN_O`; `_CALL_NON_PY_GENERAL` remains on
its current bridge until its different input-release sequence is closed in a
later consumer slice. Cold/warm tests must distinguish those routes. The
foundation owner confirmed this division. value_api_consumer_map reviews source.

This slice supports fixed NOARGS/O workspace and result transfer. Capture
independent roots before boxing; stage an owned result; publish the caller's
destination and actual live state before releasing any callback-capable owner.
Handle displaced owners, abort, pending exceptions and cleanup failures exactly
once. Revalidate and reload after cleanup even on success. Reuse existing
ownership and post-cleanup machinery rather than creating a TLS owner engine.

The runtime fixture must cross an actual Python/native gateway and caller
publication path. A synthetic C invocation is a separate mechanism check. Tests
cover scalar/object results, boxing OOM, callback mutation, abort, primary errors
and cleanup failure through cold and warmed dispatch. Test definitions are not
runtime evidence until a coherent candidate executes them.

HPy vendoring/backend, variable FASTCALL/keyword parsing, other slot families and
broad API removals are outside this first cut. They remain later consumers of
the shared transaction. Full HPy conformance does not gate the internal
register-interpreter checkpoint.

## Finite implementation and checks

Record the actual first source edit and a 60-minute ceiling with 20-minute
checkpoints. Supply the stable header/engine by 23:25 UTC if possible. The
foundation owner may integrate the selected caller only within the existing
23:49:40 UTC ceiling. A late engine or material contract issue requires an
explicit finite successor after freezing the current source; neither ceiling
is extended implicitly.

Source edits, host checks and up to three syntax-only translation units at
30 seconds each are authorized. No object output, linking, candidate execution
or native-code execution is authorized by this source grant. Coherent source,
focused review and a concrete runtime batch precede that work. Existing failed
source/checkpoints and the qualified interim RC images remain preserved.
