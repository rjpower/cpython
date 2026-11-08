# Calls workstream progress

2026-11-07: Inspected current runtime `7780e32e0e2`, call/frame assessment,
record state, generated `CALL_EFFECTS` uses, existing stats, retained profile
and immortal tuple/value definitions. Wrote staged proposal. The record field
is write-only, while the handler flag is live. Existing call counters do not
establish overlap shape or decline frequencies. No runtime edits, builds or
tests; waiting for peer review and coordinator baseline freeze.

Stage 1 source is ready for coordinator integration: removed only the unused
record field, initializer, five writes and stale record-effects comment in
`Python/ceval.c`. The generated handler's `CALL_EFFECTS` state and guards are
unchanged. `git diff --check` passed and source search finds no record-field
references. No builds or runtime tests were run by this worker. Suggested first
checks: release/debug compile followed by `test_call`, `test_ir`,
`test_pyvalue_native_call_transaction`, `test_sys_settrace` and
`test_monitoring`, narrowing to the repository's discovered module names.
Stages 2 and 3 remain separate and unimplemented.

Stage 2 is ready: moved the entire consuming-provider body into
`call_transaction_push_python_fallback`, marked `Py_NO_INLINE`, and retained a
wrapper that returns overlap `-1`/`0` directly and calls the fallback for its
only positive result, 1. A byte-for-byte comparison against `22f49d676db7`
confirms the fallback body is unchanged. In `Py_GIL_DISABLED`, the overlap
helper returns 1 and the same fallback runs; return status and storage ownership
stay unchanged. `git diff --check` passed. No builds, generated products,
benchmarks or runtime tests were run by this worker. Coordinator owns release
and debug compile, focused call tests, GCC inspection and timing screen. Stage 3
remains unimplemented.

Stage 3 source is ready against `25ce16548011`: the empty-varargs branch uses
existing `_PyValue_STATIC_IMMORTAL` to initialize its local owned `PyValue`.
The singleton is statically initialized with exact tuple type and immortal
object headers in both GIL configurations, so it cannot take numeric inline
conversion and has the same tagged identity and no owned count as the previous
borrow/duplicate. The overlap path remains disabled on free-threaded builds.
Extended the existing call test to check empty-tuple identity across repeated
calls and a generator-owned caller fallback; its existing assertions cover
positional defaults, fresh kwargs and nonempty arguments, and nearby tests
cover retained/generator frames and mortal argument finalization.
`git diff --check` passed; no builds, runtime tests, generated products or
measurements were run by this worker. Coordinator owns compile, tests and GCC
comparison. The initializer change remains a measurement hypothesis.
