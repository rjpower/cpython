# Suppressed code must not enter monitoring refresh/redispatch

Proposal only, against frozen54b283/7c6478. No native edit is applied.

The existing CO_NO_MONITORING_EVENTS early return intentionally leaves static
shared `_Py_InitCleanup` untouched. An accepted `code.replace(co_flags=...)`
can give the same property to ordinary code with RESUME. After a global
monitoring-version advance, its version0 never becomes current. The old
_MAYBE_INSTRUMENT repeatedly calls the no-op refresh and redispatches RESUME.
The interrupted final affected run records the actual Python stack in the new
flag regression, 5m38 total, with only test_monitoring failed. Worker3056428's
executable was verified; SIGUSR1 used regrtest's existing chained faulthandler
and normal cleanup. ROOTS was not signaled. Neither version acknowledgment nor
an execution-test weakening is proposed.

The exact proposed native patch has two guards:

1. _MAYBE_INSTRUMENT checks the explicit code property alongside tracing before
   considering versions. Both RESUME and INSTRUMENTED_RESUME share this uop;
   normal/tailcall generation uses the same source. No new runtime option,
   numeric classification, version write, instruction rewrite or allocation.
2. instrument_lock_held returns before version comparison/sanity for the same
   property. This closes direct `_Py_Instrument`, global executing-frame
   refresh and generator.throw entry; suppressed code's local configured
   request can differ intentionally from empty installed active state.

The local setter calls force directly. Its reviewed separate CO_NO path keeps
requested local mask/tool-version bookkeeping and no installed state. Generic
force(tool_id=-1) remains a no-op. Static shared cleanup code receives no write.

Call-site closure from full tracked C search:

- bytecodes.c172–179 and its two generated case expansions call _Py_Instrument
  only after the new eligibility check; generic code retains existing refresh,
  failure propagation and redispatch behavior.
- ceval.c1282 calls _Py_Instrument for generator.throw because that path avoids
  RESUME; the instrument_lock guard protects it without ceval.c edits.
  monitor_throw delegates to do_monitor_exc, whose ceval.h263 existing property
  guard suppresses exception callbacks.
- _Py_Instrument1880 delegates to instrument_lock_held; its other caller is
  instrument_all_executing_code_objects1907, used by global version changes.
- _PyMonitoring_SetLocalEvents1994 alone passes a selected local request to
  force. Existing legacy_tracing and public sys.monitoring adapters delegate
  to that checked endpoint.
- debug_check_sanity is used only from actual instrumentation callback paths.
  Constructor admissibility rejects runtime monitoring wrappers as pristine;
  replacement creates fresh ordinary words. Suppressed code cannot acquire
  wrappers through these guarded install paths. No sanity check is weakened,
  and no assumption active==global|local is imposed on suppressed code.

First falsifiable controls and gate scope:

- Keep the actual failing54b trace/native hashes. A bounded isolated old run
  explicitly advances the global version before entering valid replaced code;
  watchdog dumps/exits at2s, core0, normal actual verifier/stress/optimization
  witnesses. New guard must complete the same script and deliver no callbacks.
- Strengthen the in-module existing flag regression with explicit version
  advance so isolated ordering cannot accidentally hide the old loop. Require
  configured/cleared request, empty active masks/arrays, unchanged words from
  setter, correct result and no callbacks. Paired unflagged function must still
  deliver its local instruction events under the same advanced version.
- Valid flagged generator yield/throw exercises both RESUME and ceval direct
  refresh, with correct yield values and no callbacks; global refresh while
  flagged code is on stack must not allocate/install monitoring or write its
  version. Existing class/init smoke covers the real static cleanup object.
- Regenerate cases; IDs/base/kind/word layout stay unchanged. Run affected
  monitoring/IR/code/generator/legacy modules release and Og, strict actual-kill
  with worker.main, smoke and regeneration closure. No full qualification or
  standalone-prerequisite gate claim from a combined storage candidate.

Commit scope: prerequisite can be preserved as a focused source patch/commit
for the two eligibility guards and generated expansions, with proof explicitly
bound to the combined dirty storage candidate. Coordinator must decide whether
standalone prerequisite qualification is needed before integration. Do not
silently call combined gates standalone tests. Estimated implementation and
first old-fail/new-pass 15–25min after review; remaining storage qualification
continues afterward, one H3 native transaction at a time.
