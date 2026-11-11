# Initial49 CALL admission: source diagnosis

The initial native focus failed ten leaf hit/owner-miss assertions. The retained diagnostic shows a certified target with all counters zero, a caller still using generic CALL after warmup, and specialized reads/comparison inside the callee. This is before leaf admission: a certificate alone cannot install CALL_PY_EXACT_ARGS.

## Proven fixture mismatch

`Objects/funcobject.c:_PyFunction_GetVersionForCurrentState` returns `func_version` unchanged. UNSET is 0, CLEARED is 1, and valid versions start at 2. `_PyFunction_SetVersion` is called only by MAKE_FUNCTION, which assigns the code's version. `PyFunction_New`/`types.FunctionType` leave it UNSET. Assigning `__code__` clears a previously valid function permanently; `test.support.reset_code` performs exactly that assignment. These are intentional existing admission policies, documented next to the version implementation.

`specialize_py_call` rejects invalid versions before installing an exact/general Python CALL handler. A function's own loads and comparisons can still specialize when entered generically. Thus the observed split—specialized callee body but generic caller and zero leaf counters—is expected for `fresh_record`'s FunctionType target and for positive identity/ignore/constant targets passed through reset_code. Longer warmup or lower thresholds cannot fix those functions.

## Smallest bounded correction

Correct the fixtures to construct positive targets through ordinary MAKE_FUNCTION. Fresh compilation/execution of a small function-definition module gives fresh target code and a valid version; a reused compiled definition module executed in two dictionaries creates two functions sharing exactly one code object while preserving distinct live globals. This tests general code/environment behavior without a new runtime control or forced handler/version. A factory definition also gives valid functions; reset only CALLER code when needed, not the positive target.

For changed immutable code/constant/branch metadata, embed the replaced target code as the code constant of a definition module and execute MAKE_FUNCTION, rather than constructing it with FunctionType or assigning it to a live target. Keep explicit mutation tests: after a function's code/default/provider mutation, correct fallback is expected under the existing permanent version invalidation policy. A subsequent positive-hit witness must use a freshly created eligible function. Preserve exact hit-delta assertions and CALL_PY_EXACT_ARGS coverage; do not turn them into skips or weaken the checked prefix.

No runtime version-policy change is needed for this experiment. Lazily assigning/reassigning versions would be a separate code/function-cache and invalidation design, especially for shared code, mutable defaults and free-threaded cache rules.

## Unresolved distinguishing witness

The sole-function-owner witness already creates its inner identity with ordinary MAKE_FUNCTION and does not reset it. The fixture mismatch above does not by itself explain its missing owner_miss. Root should separately retain its caller adaptive IR, CALL feedback (including cached function version), target certificate counters and a natural retained-function control. Admission/entry misses must be distinguished from an owner miss; no claim that all ten failures are resolved is justified yet. The assertion stays strict.

## Natural controls and order contamination

Root's `leaf49-natural-diagnostic.log` confirms ordinary identity and exec-created functions reach CALL_PY_EXACT_ARGS and record hits after two initial calls. `leaf49-float-sole-diagnostic.log` records 100 float hits and 100 ephemeral-function owner misses at 102 rounds. The corresponding two unittest methods pass alone. Warmup length is sufficient for these controls; the residual full-module failure is order-dependent.

The source supplies a distinguishing cause: after any instrumentation epoch advance, first entry into fresh code calls `force_instrument_lock_held`, then `update_instrumentation_data`, which unconditionally calls `allocate_instrumentation_data`. That allocates `_co_monitoring` even with all active events empty. The leaf entry contract deliberately rejects any non-NULL `_co_monitoring`. Local-monitoring/trace/profile witnesses therefore make later fresh functions permanently decline this first-scope leaf path; restoring event bits does not restore the interpreter's epoch or remove the allocated metadata. Ordered-float and sole-owner tests follow those observers alphabetically. Their full-module counters should show entry misses, unlike their isolated controls.

The smallest contract-preserving fixture correction is to run the observer-mutating witnesses in fresh subprocesses, retaining their event assertions and no-hit assertions while leaving later positive tests in a fresh unobserved parent. Do not reorder tests to hide the limitation, increase rounds, skip hit assertions or reset global internal epochs. A general runtime improvement could admit allocated-but-inactive monitoring metadata under a current epoch and zero active per-code/global events, but that expands the explicitly reviewed NULL-metadata entry contract and requires its own source/observer review. It is unnecessary to validate the current bounded experiment.

The author's `fresh_function(code, namespace)` helper is independently approved for the fixture correction: replace the sole nested target code constant in an outer factory, instantiate that factory with the desired namespace and invoke it. The unversioned outer function may run generically; its real MAKE_FUNCTION gives the supplied no-freevar target its code's version. This preserves shared target-code identity and distinct environments without a setter or forced version. All current positive target forms have no freevars; assert that precondition.

## Independent remaining root-provider gap

After the function-fixture correction, the first alias test still failed before any observer test. `leaf49-ignore-diagnostic.log` shows a naturally specialized exact CALL but a certificate count of 0 for `RESUME → LOAD_COMMON_CONSTANT 7 → RETURN_VALUE`. This is a real omitted constant-root provider, not warmup or binding. The approved four-file follow-up admits generic bounded LOAD_COMMON_CONSTANT roots for all 14 interpreter constants, with exact pure metadata and canonical current-table borrowing; the existing clearing guard protects teardown. All 14 positive-hit and invalid-index tests accompany it. Final corrected native results are pending, so the initial 49 failure record remains intact.

Sources inspected at `656326dc375`: `Objects/funcobject.c`, `Python/bytecodes.c:_MAKE_FUNCTION/_SPECIALIZE_CALL`, `Python/specialize.c:specialize_py_call`, `Lib/test/support/__init__.py:reset_code`, and all affected target constructions in `test_ir_leaf_calls.py`. Evidence read: `leaf49-initial-diagnostic.log`, `delta49-leaf-release-focus.log`. No runtime edits, native execution, builds or commits by this reviewer.
