# Checkpoint closure peer review

Review of the R27 acceptance rule and G2 evidence, 2026-11-05. This review
covers the canonical value API design, the preserved W5 interface survey, the
current `check_r27.py` and its tests, and the G2 prototypes and timing. It does
not certify an implementation increment. Runtime code was read only.

## Decision

R27 is an appropriate checkpoint gate, but the current checker can report a
pass with incomplete evidence. Repair its fail-open cases before admitting the
checkpoint. Defer both G2 prototypes until after the merged PyValue baseline is
qualified and measured. Preserve the A4, B3 and control trees as experiments;
neither is an integrated interpreter feature. The existing IR remains a valid
register interpreter with a run-time slot-class test. Update the design's G2
status and the closure plan to say this plainly.

## R27 acceptance criteria

The accepted rule in `design/value-api.md` R27 and
`design/proposals/value-api-v2a0.md` §3 is: in-tree core code calls value forms
and makes no box; only compatibility thunks and adapters may box values for
outside object-form callers. The gate has three parts:

1. Static: every object function with a value form is marked; no core unit
   calls a marked object form; no deferred core unit remains; all applicable
   `Modules/` units are value-native; compatibility bodies use THUNK boxing.
   Direct calls through marked object-slot families also need the agreed
   `slot-read-call` rule; the two stored-pointer cases in `w5-interface.md`
   require hand inspection.
2. Kernels: each of the 29 declared kernels in `bench/run.py` has zero box
   counters in every recorded process, including startup. This is a pure
   Python corpus, so even a boundary box is a failure. Every expected name
   must occur exactly once and carry nonempty process and box-counter data.
3. Full suite in the stats build: zero CORE, core-pending or unresolved boxes;
   zero boundary boxes owned by a core unit; all other boundary boxes, crashes
   and uncounted child launches explicitly listed. The test run itself must
   succeed, and its process accounting must be complete.

The design states a **Linux gate**. Platform-only modules without a Linux
build are outside this run and should be listed as unverified, then checked on
their platform. The compatibility test units are an explicit narrow exclusion,
not a general exemption for modules that are hard to port.

## Blocking checker holes and minimal repairs

1. **Static completeness is absent.** `check_r27.py:static_violations` only
   checks the `Py_BUILD_CORE_*` module condition; an optional ownership file
   produces the literal note “rows not checked.” `check_value_api.py` reports
   `Py_VALUE_DEFER_MARKS` as a nonfailing finding. The W5 survey found 36
   unmarked object-form twins, 19 conditional number extractors, about 1,200
   calls after marking, and 17 direct slot calls. Make a reviewed inventory
   of functions with value twins a required static input, verify each marked
   declaration, and fail on a compiled deferred core unit. Run the existing
   value-API lint with its full compile database. Add negative tests for an
   unmarked inventory row, a deferred unit and an unmarked `Py_BUILD_CORE_*`
   module. Keep generated `Python-ast.c` paired with `Parser/asdl_c.py`.
2. **A kernel result can be vacuous.** `kernel_violations` accepts an empty
   list, an empty process map and processes without any recognized box
   counter. It does not check the 29-name roster. Require the declared roster,
   reject missing or duplicate names and require at least one process with
   recognized box counters per kernel. Reject a process lacking them. The
   tool's existing zero and nonzero fixtures should be extended with these
   three negative shapes. Do not use a filtered kernel run as admission.
3. **Suite execution can fail while the gate passes.** `cmd_suite` saves
   `regrtest_returncode` but `suite_gate` ignores it. The preserved
   `w5-suite0.json` records return code 2. A zero-box or carefully excepted
   run with that result must fail. Reject zero dumps too; otherwise an empty
   directory passes `collect --gate`. For `suite --gate`, require return code
   zero and a nonempty dump set. For `collect --gate`, require dumps and treat
   test success as separately evidenced. Test these cases with synthetic
   summaries or command stubs, without rerunning the full suite.
4. **Attribution completeness needs a consistency check.** Existing failures
   for unreadable dumps, owner-table drops, unlisted `.run` markers,
   uncounted launches and unresolved addresses are sound. Compare summed
   per-kind totals with owner rows plus declared drops for the same covered
   dumps, after accounting for allowed `test_sys`/`test_pystats` resets.
   Otherwise a parser mismatch or an omitted owner line can look like zero
   debt. If this invariant cannot hold because of current stats emission,
   document the exact mismatch and add an equivalent coverage counter. A
   synthetic missing-owner-line fixture must fail. Check that CORE rows have
   the site attribution shape and boundary rows have an owner; this prevents
   a wrong owner kind from being accepted as an external exit.

These are checker tests, not a request for another broad runtime test cycle.
Run the focused checker tests and self-test, then use the repaired gate at the
final merged stats integration point. Keep the full debug/release suites at
integration points as the process already requires.

## G2 decision

The seven-round matched timing on old base `a1ecc0eb4f3` gives base/candidate
geomeans 1.0100 for A4 and 1.0119 for B3 over 29 kernels. Their difference
is 0.19 percentage point; most per-kernel spreads are several percent. A4 is
small and preserves the format, but carries an extra handler parameter that
GCC spills. B3 lowers reduced-workload instruction counts more, but changes
the IR encoding, all decoders, the verifier and the `.pyc` magic. These data
do not establish a robust elapsed-time advantage for either, and the current
PyValue work has moved since the prototype base. A merged baseline measured
without G2 gives a clean decision point for a short post-checkpoint matched
experiment. The class-bit design remains an explicit option, not an implicit
part of the accepted current encoding.

## W5 interface boundary

The W5 survey's kernel-first split is sound for integration. For bulk work,
the written rule should state exactly where a value becomes owned, which
object-form exit it replaces, and the failure path. Mechanical conversion is
safe only for calls where ownership is unchanged; callback and `_decimal`
paths need hand review. For each increment, inspect the complete staged and
unstaged diff, regenerate producers, compile release, run the touched modules'
tests, then repeat all 29 kernel counts on the merged build at the kernel
landing. The final suite tail and static completeness belong to a later W5
gate increment.
