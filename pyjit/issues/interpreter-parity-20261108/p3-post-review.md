# P3 cold post-review

2026-11-08. I reviewed the P3-only patch materialized on M1
`d93b83800a5` and committed as `10627c339fa`, its regenerated handler and metadata, independently of the
draft review. No must-fix source, generated-output or focused-evidence
finding. Merged qualification and isolated timing remain outstanding.

The generated `CALL_PY_GENERAL` handler has the PEP 523, function-version and
recursion guards before its private call. Its shared `_CHECK_FUNCTION_VERSION`
code is unchanged. Specialization first requires the standard function
vectorcall and assigns only a valid version (at least two). The generated
private edge asserts that cached-version premise in debug builds. Public
`PyFunction_SetVectorcall` clears a live version before changing the provider,
so a later call fails the shared equality guard and deoptimizes. A method
wrapper uses `CALL_BOUND_METHOD_GENERAL`, whose generated handler still calls
the generic transaction. Instrumented calls likewise keep their generic
route. The new uop adds an ID but does not change the register word layout or
the `CALL_PY_GENERAL` metadata entry.

The private runner builds the existing transaction record with the already
prepared vector window, invokes the existing overlap path and then its
consuming fallback. On positive decline it invokes the existing native path
in that record. The overlap path may allocate kwargs, so it retains all
post-allocation checks of callable identity, code, provider, PEP 523,
defaults, stack space and argument offset. The consuming path also checks
provider after any allocation. The generated fallback label still calls the
generic `call_transaction_run_vector`; the generator's override is local to
the guarded invoke, and the handler's resulting child/result/error dispatch
is otherwise shared with the old call completion. This avoids replaying call
preparation while keeping callbacks and failures under the prior protocols.

The analyzer now names general register-IR marker effects separately from
the two terminal call-invoke markers. The validator pins this private entry
to `CALL_PY_GENERAL`, its exact prefix, scalar/window roles and nonescaping
guards. Generator tests reject missing/reordered or duplicated guards and
placement in bound/instrumented handlers, and check that the generated
fallback remains generic. Runtime cases exercise function code/default
mutation, vectorcall invalidation, methods, generators, arity failure and
monitoring. The added `_testcapi` setter uses the public vectorcall setter
and exists only to exercise invalidation.

`make regen-cases` and both release/debug builds returned zero; each build
checked 117 modules. Both focus runs passed 583 tests with eight skips each
across `test_generated_cases`, `test_ir`, `test_call`, `test_monitoring` and
`test_frame`. `git diff --check` is clean. The generated diff is just the
intended private invoke plus its debug assertion in the guarded handler; the
terminal fallback still uses the generic runner. Fixed-GCC disassembly of
`_TAIL_CALL_CALL_PY_GENERAL` calls the private runner directly after the
shared guards. That runner calls `call_transaction_overlap_python`, then
`call_transaction_push_python_fallback` on decline, then
`call_transaction_native` on a further decline; it has no generic
`call_transaction_invoke` call. This proves removal of initial generic route
rediscovery in the accepted specialized path, not an elapsed-time gain.
Isolated timing and merged qualification remain for the coordinator.

## Merged smoke addendum, 12:43 UTC

The first stats-build merged smoke exposed a missed affected test usage:
`test_ir_contract` failed seven assertions for `CALL_PY_GENERAL`. Its source
scanner recognizes only `IR_INVOKE_CALL` as terminal, then scans P3's new
`IR_INVOKE_GUARDED_PYTHON_CALL` through the trailing legacy frame uops and
incorrectly reports no result. This is one test-contract root cause, not a
demonstrated runtime failure and not caused by the stats-only diagnostic
overlay. The scanner and its explanatory comment must recognize both terminal
call markers; rerun the test after the temporary overlay is retired. The
earlier no-must-fix verdict applies to runtime source and focused evidence,
not this newly exposed merged-check finding.

## Contract resolution, 12:45 UTC

Commit `1c8774797f3` updates the independent source scanner to recognize both
terminal call-invoke markers and adds the guarded uop to the effect/frame
property test. The two-file diff is limited to tests and passes `git diff
--check`. After the stats overlay was retired to byte-identical source,
release and debug `test_ir_contract` each passed all 24 cases; both builds
also passed the six targeted call-marker property cases. No must-fix finding
remains from this post-review. Full merged qualification and timing remain
separate admission gates.
