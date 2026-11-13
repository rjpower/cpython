# Separate signature classification from execution readiness

2026-11-12. Read-only diagnosis of integrated `0851fd51801` after the new
test_dict.test_item_iterator_oom subprocess SIGSEGV. Root's offline stack maps
CALL_PY_EXACT_ARGS to `_PyFrame_Initialize` / `_PyFrame_InitIRInstrPtr`, which
requires a prepared execution image. No baseline/native reproduction was run
in this lane; root owns those comparisons.

The source has `SIMPLE_FUNCTION=0` and `SPEC_FAIL_OTHER=0`. `function_kind`
clears a failed `_PyCode_GetExec` error and returns SPEC_FAIL_OTHER, but its
consumers interpret zero as ready SIMPLE_FUNCTION. This can nominate an exact,
property/getattribute, getitem or init cache with NULL exec. The builder itself
publishes only successful complete execs, and the checked generic constructor
handles GetExec failure before frame reservation; they are not the unchecked
proof violation. Extra cold-plan allocations expose the ambiguous result but
do not justify a hot readiness guard.

Prefer two small interfaces over a four-state preparation-result struct:

* A pure local signature enum classifies SIMPLE_SIGNATURE, GENERAL_SIGNATURE
  and UNOPTIMIZED_SIGNATURE. It performs no allocation and never returns a
  specialization-statistics integer. Preserve existing shape checks/ordering.
* `prepare_function_for_inline(code)` returns bool only. It calls GetExec,
  returns true for a complete image, and clears an optional-specialization
  failure before false. Each nominator reports OTHER where appropriate and
  refuses publication on false. Generic execution may subsequently retry at
  its existing checked ownership boundary.

This makes readiness a separate positive proof. A four-state typed result can
also work, but duplicates shape state and encourages diagnostic-only callers
to run preparation. A negative success sentinel alone still mixes unrelated
domains and is not selected.

| Consumer | Concrete change |
| --- | --- |
| `function_check_args`, property/getattribute callers | Pure SIMPLE and exact argc first; explicit ready bool before function-version/cache publication. Shape diagnostics remain complex/notoptimized, prepare failure OTHER. |
| `store_subscr_fail_kind` (stats only) | Pure signature classification only. No GetExec allocation merely to choose PY_SIMPLE versus PY_OTHER. |
| `get_init_for_simple_managed_python_class` | Require SIMPLE plus true readiness before returning the cached init owner. Shape miss retains INIT_NOT_SIMPLE; preparation miss refuses nomination. |
| `specialize_py_call` | Choose exact versus GENERAL from pure shape, retain nonoptimized rejection, require readiness for both before func_version/binding/opcode publication. |
| `specialize_py_call_kw` | Retain accepted general signatures; require true readiness before func_version, keyword binding or opcode publication. |
| `specialize_subscr_getitem` | Require SIMPLE, argc2, then true readiness before type getitem-cache nomination. |

Audit every old SIMPLE_FUNCTION/function_kind reference, including forward
declarations and Py_STATS arms; remove the ambiguous sentinel. This is cold
specialization work only. No extra ordinary CALL/property/getitem guard and no
ownership/call-window ABI change is needed. Other checked generic routes and
existing readyexec guards remain their actual contracts.

Keep the existing dict OOM subprocess unchanged and run it first. If its root
reproduction/retry evidence is insufficient, a focused deterministic probe
should use fresh admitted code with NULL exec, fail actual preparation
allocation, observe no ready-dependent nomination/error leak, remove the fault,
and retry the same code successfully. Cover allocator and plan-builder failures
without assuming one fixed allocation ordinal. Prefer a bounded cold helper or
single-fault sweep that observes the actual preparation/nominator path; do not
warm the failing code or prebuild its image. A separate actual-call retry then
checks behavior. The root may discover a further contextmanager __exit__ OOM
after this crash is fixed; do not conceal it by changing the original fixture.

IR and GC peer the separation direction. Both require failed preparation to
refuse all ready-dependent nomination, including GENERAL/KW, and the stats-only
path to remain allocation-free. Root selected the smaller two-interface
formulation; both final design/source peers are GO. Root owns native reproduction,
selected54 comparison, generated integration and both-role tests.

The frozen one-file patch in pins is Python/specialize.c (53 insertions,
23 deletions) over `b722330ef3f`. It removes all old sentinel/helper uses,
preserves shape priority, and leaves the original OOM fixture unchanged. Usage
census and diff checks pass. Root owns actual allocator-fault/retry validation;
no candidate execution or new fault helper was added here.
