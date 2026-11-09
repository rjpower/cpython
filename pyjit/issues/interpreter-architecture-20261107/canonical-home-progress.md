# Canonical-home experiment progress

2026-11-07. Preparation against runtime `225480a4250`; no runtime changes,
builds or measurements. The prior STOP remains in force until R4 lands and
the coordinator explicitly releases R5.

## Source map and premise

- `Include/internal/pycore_object.h:916` documents the borrowed store input;
  `Objects/dictobject.c:7257` is its entry. Add the canonical-home contract
  and fatal debug assertion before the first duplicate or callback.
- The only direct in-tree callers are `Objects/object.c:2135` and the object
  adapter in `Objects/dictobject.c:7293`; the adapter canonicalizes with
  `PyValue_FromObjectBorrow`. `PyObject_GenericSetAttr` and its dict variant
  canonicalize through `_Py_VALUE_ARG` in `Objects/object.c:2203–2214`.
  Value-slot thunks use that same constructor.
- `Objects/dictobject.c:7231` duplicates the input to an inline home;
  `Python/bytecodes.c:2952,2969` moves canonical frame-home values into shape
  slots. Initialization writes NULL, and copy/move preserves bits. Shape
  invalidation/materialization and alternate adapters still require review
  against the final integrated source before admission.
- `Python/bytecodes.c:_LOAD_ATTR_SHAPE` duplicates the guarded slot. Declare
  the trusted producer there and assert its premise; the declaration is a
  manual source proof, not inferred C-pointer provenance.
- In R3, `IRStack` emits `IR_STORE_TEMP_DST` in
  `Tools/cases_generator/ir_generator.py:2092`. Re-map after R4 lands. Only
  the declared result's first temporary flush may bypass publication; final
  folded-local storage, error cleanup, refresh and companion outputs retain
  existing behavior. Generator source ownership requires coordinator release.

## Focused checks to implement after release

- Extend `Lib/test/test_ir.py` with shape-specialized reads for inline int and
  float, large boxed int, exact-number subclasses, static immortal and mortal
  values. Warm each reader and assert `LOAD_ATTR_SHAPE` is selected; exercise
  both temporary and folded-local destinations. Use fresh code as necessary to
  avoid specialization history contaminating cases.
- Existing `test_specialized_attribute_result_survives_receiver_retirement`
  checks mortal result survival, receiver callback/GC and displaced-local
  finalizer order for shape and slot paths. Extend its shaped folded-local
  case: the receiver callback observes the original local, then installs a
  sentinel through `frame.f_locals`; the final store must retire that sentinel
  once and return the result. The original local retires during the callback's
  write, so adapt its finalizer observation for this variant.
- On refresh failure, the error branch must not overwrite or roll back a
  callback's local write. The existing generated-case fixture can verify that
  the failure jump precedes the final local store. No supported test hook was
  found to force `value_operation_refresh`'s frame/word identity failure;
  exceptions raised by `__del__` are unraisable, not that failure path.
- Extend `Lib/test/test_generated_cases.py` (`TestIRCases`) generator fixtures to require direct
  first temporary store only for the marked shape producer, ordinary final
  local publication and ordinary stores for unmarked producers. Negative
  fixtures must reject wrong result, duplicate or conditional declaration,
  mismatched retirement, and intervening mutation, address-taking or escape;
  reuse R4's output/phase checks and do not build a provenance parser.
- Run the focused tests with coordinator-owned release and debug executables
  using `-m test test_generated_cases test_ir test_ir_contract test_opcache`; coordinator handles
  regeneration, actual builds, broader checks and machine-code/performance
  evidence. Exact executable paths are to be confirmed at the integration gate.

The coordinator will release the isolated contract worktree after R4 focused
qualification and commit. R5 integration into the runtime remains gated on
the R4 cold review. Reuse the existing receiver-finalizer coverage.
