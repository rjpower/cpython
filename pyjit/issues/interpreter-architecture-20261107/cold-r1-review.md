# Cold R1 review: call transaction effect record

**Decision: accept** runtime commit `22f49d676db` against `7780e32e0e2`. I found no correctness or ownership regression in this ceval-only cleanup.

The deleted `_PyCallTransaction.effects` field had no readers in the parent. Its initializer and five later writes could not affect call dispatch, error recovery, or publication. The live effect state remains the handler's `call_state`: generated call cases set `CALL_EFFECTS` for specialization, method expansion, tuple preparation and monitoring, and test it before redispatch or fallback. The patch leaves those guards, `CALL_PREPARED`/`CALL_MONITORED`, and the transaction's preparation and refresh paths intact. The shorter wrapper comment describes the remaining role accurately.

The recorded release and debug builds returned zero and each checked 117 modules. Both focused runs passed the same five test files (`test_call`, `test_ir`, `test_pyvalue_native_call_transaction`, `test_sys_settrace`, `test_monitoring`): 916 tests run and one skipped per profile. `r1-identities.json` ties both rebuilt images to `22f49d676db`. Evidence: `build/pyjit-link-wave1/interpreter-architecture-20261107/r1-checks.json`, the sibling build and test logs, and `r1-identities.json` in the runtime checkout. I did not run new checks. These focused checks do not establish full-suite qualification or a speed change; the performance screen is still active.

Optional local polish: `call_record_expand_method()` can return the negative-status comparison directly now that it no longer uses `status` to set the removed field. This is too small to hold R1. Keep `CALL_EFFECTS` and its handler comment: they still describe live guards.
