# Final global stackref checker assessment

2026-11-01. **The corrected upstream control also failed to build. Stop this control path; no third correction or native attempt.** Recommend treating the added whole-runtime `Py_STACKREF_DEBUG` profile as currently unvalidated and non-gating for the ordinary RC product, subject to root and independent reviewer acceptance. This is a profile-applicability decision, not twelve test exclusions, a checker pass, baseline equivalence or proof that every failure is harmless. All twelve RC failed modules and43 assertion failures remain recorded and unresolved at their actual scope.

## Final control outcome

Root confirmed supervisor64610 terminal1. Configure returned0, make returned2, and the build gate returned1 after68.044seconds. The same four `_bootstrap_python` freezer recipes ended134. The interleaved raw stderr contains four complete token IDs308848/309192/310572/312360 and the diagnostic `closed with 1 borrowed refs` at generated_cases.c.h:468, opened at9786. The three comparison methods never ran; no test row or successful batch completion exists. The corrected source remains clean d2ca, exactly one line beyond321925. The pristine control failure remains unchanged.

Post-gate cleanup checks all four executable paths, including bootstrap/freezer, and records no live processes before its deadline. Independent read-only09:49:12 inspection found no matching native or compiler/build-tool process scoped to the fresh build directory. No project-local core/core.* file exists; the kernel apport pipeline remains outside this audit's storage inventory. Nothing was deleted, rebuilt or retried.

The new failure has a concrete ownership-model explanation in the unchanged upstream Unicode inplace-add path. Line468 steals `*target_local` into owning C variable `temp`; line470 closes the borrowed stack value `left`; only afterward can `PyUnicode_Append` move/free the string. A steal transfers an object reference into C without decrementing it. Debug `PyStackRef_AsPyObjectSteal` nevertheless calls `_Py_stackref_close`, whose table rejects closing an owner with an outstanding borrower before that C ownership transfer is represented. The log's Python stack includes posixpath.join during site initialization, consistent with this path. This demonstrates that the checker profile cannot currently bootstrap this source/configuration. It does not establish that every RC shutdown report has the same cause. No freezer override, normal-Python bootstrap substitution or further source fix is proposed.

## Fork bookkeeping and profile boundary

In ordinary debug/release/ASan configurations, stack values use the normal pointer/tag representation. Explicit `-DPy_STACKREF_DEBUG` instead selects per-interpreter token tables and injects lookup/create/close operations. Normal `Py_DEBUG` does not enable that mode by itself. Those extra operations require a current interpreter; async and remote readers cannot assume the ordinary pointer layout in this profile.

The fork's table extension is narrow. `Objects/frameobject.c:317–324` first stores `Py_NewRef(old_obj)` in `f_overwritten_fast_locals`, then marks that original local token as allowing borrowers to outlive its close. This is the only call site. Under the checker, a closed owner record is retained until its last borrower closes; `report_leak` ignores that already-closed owner but still reports nonclosed entries. Outside the checker the marker is a no-op. The actual object lifetime comes from the separate owning tuple, not from diagnostic metadata. No new blanket permission to close arbitrary borrowed owners or disable reports is present. The retained open-record failures therefore are not erased by that special closed-owner bookkeeping. This audit is not an exhaustive proof of every reference transition.

The asynchronous traceback path calls `_PyFrame_SafeGetCode` and then table-based `_Py_stackref_get_object`, which calls `PyInterpreterState_Get`; a watchdog/native thread may have no attached GIL thread state. The actual faulthandler/regrtest diagnostics show that failure. Separately, `_remote_debugging/frames.c:212,273` treats executable stackref bits as a code-object address after tag removal; it does not resolve this profile's table IDs. The actual profiling failures include zero samples. These are concrete representation/reader support gaps, not evidence that ordinary RC code pointers or ordinary profiling are broken.

## Retained RC failure families

The machine ledger retains every failed ID and traceback hash. The following is attribution, not an exemption list.

| Returned module | Assertion failures | Actual evidence and remaining limit |
|---|---:|---|
| multiprocessing_fork.test_misc |2|Fork/forkpty subprocesses abort with open-record reports. The standard methods fork while a daemon thread exists; abandoned-frame bookkeeping is a source-backed hypothesis, not a full lifetime proof.|
| multiprocessing_forkserver.test_misc |2|Same explicit ForkInThreads methods and open-record reports; no separate baseline reproduction.|
| multiprocessing_spawn.test_misc |2|Same methods still call fork/forkpty despite the package's start-method name. Same scope limit.|
| multiprocessing_spawn.test_manager |3|Manager exit−6 instead of0/−15. The retained assertion traces alone do not identify its abort cause. Unresolved.|
| test_faulthandler |7|Actual no-GIL/current-thread-state diagnostics and abort/output mismatches; async table lookup path identified.|
| test_embed |1|Last interpreter-clear GC deliberately creates a Task/coroutine/callback cycle; checker abort is explicit. Objects created after the last collection can still own tokens at reporting. No claim all such tokens are acceptable leaks.|
| test_interpreters |2|One explicit remaining-daemon open-record abort; another interrupt/shutdown exit−6 has incomplete causal attribution.|
| test_repl |2|One asyncio REPL shutdown contains open-record reports; the contextvars case records exit−6 without a complete causal trace.|
| test_pyrepl |1|Dumb-terminal exit−6. Its retained assertion does not establish an abort cause. Unresolved.|
| test_profiling |11|Zero/missing sample and expected-frame assertions; direct pointer reader does not implement debug token-table resolution. Not every assertion is individually explained.|
| test_threading |9|Multiple finalization/fork/daemon cases report open records or unexpected abort/output. Thread-state clear/raw datastack teardown can abandon tokens; preserve individual exceptions and unknowns.|
| test_regrtest |1|Expected worker-failure output is contaminated by the explicit no-GIL fatal-dump path. No general runner pass inferred.|

`PyThreadState_Clear` deliberately leaves the current frame when threads can be abandoned; raw datastack teardown does not perform normal frame closes. The last-GC reporting order and these contracts also exist in pristine source. That is useful source evidence, but both attempted upstream builds failed before the selected shutdown cases. **There is no actual upstream reproduction of these twelve failures.**

## Finite qualification disposition

Accept a documentation/qualification-plan amendment that marks the entire added global checker profile unsupported/unvalidated for this landing. Preserve the failed stopped full cohort (563 returned modules,12 failed), both failed upstream builds, partial positive checker results and their original deadlines. Do not change test code, add twelve skips, suppress table reports, mark the profile passed or advertise supported global checker behavior. Do not infer a normal-build defect solely from these failures; equally, do not infer their absence from this audit.

Keep ordinary debug and release full-suite requirements, the independently adjudicated bounded ASan heap/address/UAF scope, standard targeted refcount-leak/ownership tests and the pending unchanged CPU extension3+3 interval. The latter is still pending the packaging feedback; it is not already a pass. ASan does not replace reference-count leak checks, and no fake-stack/LSan claim is added. Final packaging native results and actual main integration remain required. Any concrete failure under those supported configurations stays blocking and needs its own causal resolution.

This makes the gate reflect the configurations actually supported and tested, without another control ladder. Further checker support is a separately scoped representation/lifecycle/debug-tool project; it is not a prerequisite to the selected RC/JIT path and is not authorized here. Root and an independent reviewer must accept this finite disposition before it changes qualification authority.

Evidence: `build/rc-stackref-control-final-assessment-20261101-v1/{receipt.json,retained-families.json}`. The audit read retained logs/source and `/proc` only; no source edit, compiler or target invocation occurred.
