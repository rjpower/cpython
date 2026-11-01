# VM0: compiler and existing backend controls

State: review. 2026-11-01. Host-only plan; execution, worktree creation and
helper adaptation require later coordinator grants and independent review.
This experiment does not implement the proposed passed-frame ABI.

Revision 2 preserves the exact first plan, SHA256
`29d73c1c4b340e7ec8a7574ba0f791b560c852f8aea4f882a6054c316d78d392`, at
`build/vm-design-canvas-20261101/vm0-plan-v1-29d73c1.md`. It corrects executable
terminology and makes the existing stats child's retained raw witness concrete.
The arm, build, method, process and sample counts are unchanged.

## Question and frozen source

Compare GCC computed goto, Clang computed goto, and the existing Clang tail
backend on unchanged runtime source. The first contrast measures compiler and
its maintained source-selected policy; the second isolates backend selection
under one compiler. The direct tail/GCC contrast determines the combined user
effect. Do not attribute a compiler benefit to tail calls.

Source checkpoint: `417a9443a599e879c76c34c13ae76943bd6f6b3e`. This checkpoint
has no changes outside `boil/` relative to qualified historical source
`e9e10d5b503eaf219797a5db44766eeab6fad4dc`. Fixed internal driver and kernel
bytes are also unchanged. Later documentation commits may be bridged by exact
input equality; no runtime or test correction may silently enter the arms.
Keep GC, values, handler definitions, specialization and frame publication
identical. No nursery option, new runtime flag, ABI patch or compiler pass is
part of VM0. The separately reviewed GC adoption work remains separate.

| Arm | Configure arguments | Runtime mode | Products |
|---|---|---|---|
| G | `CC=gcc` | existing computed goto, O3, nonstats | reuse qualified historical `build/release` only after exact bridge |
| C | `CC=/usr/bin/clang-21` | computed goto, O3, nonstats | fresh owned build directory |
| T | `CC=/usr/bin/clang-21 --with-tail-call-interp` | existing tail convention, O3, nonstats | separate fresh owned directory |
| TD, correctness only | `CC=/usr/bin/clang-21 --with-tail-call-interp --with-pydebug --enable-pystats CFLAGS=-O1 -g` | tail, assertions and extra stats argument | separate fresh owned directory, never timed |

These are exact argv elements, with the final CFLAGS assignment one element.
Use the existing Clang21 installation only: `/usr/bin/clang-21` resolves to
`/usr/lib/llvm-21/bin/clang`; observed binary SHA256
`412bbe8c60571a1eb06f48fde89635033621caeb01a9b4ee76d46711bae8e932`.
No compiler was invoked to obtain this host-only identity. Version, selected
linker and actual driver subcommands remain unobserved readiness fields until
a specifically authorized native preparation step. No flags search or PGO.

## Matched policy and GCC reuse

Retain default configure O3 policy, GIL, mimalloc, no shared libpython, no JIT,
no LTO/PGO/ASan, and nonstats for all three scored arms. The existing G image
has `OPT=-DNDEBUG -g -O3 -Wall`, empty CONFIGURE_CFLAGS, frame pointers,
leaf frame pointers, zero-init, and `CFLAGS_CEVAL=-fno-gcse`. Its source also
disables evaluator SLP specifically for relevant GCC/x86-64 versions.
Those GCC-specific source policies stay in place. Do not feed `-fno-gcse`
to Clang or remove GCC's maintained workaround to manufacture identical flags.
The first contrast therefore includes the compiler's normal source-selected
policy. Report expanded effective compile/link arguments and any configure
feature differences. O3 without PGO does not settle a later optimized-release
comparison; no post-failure PGO rescue run is authorized.

The existing executable is ELF64 little-endian ET_DYN (PIE), not a statically
linked executable: its first18 bytes end in e_type `3,0`, and the retained ELF
receipt records PT_INTERP plus DT_NEEDED libm/libc. The Makefile has
PY_ENABLE_SHARED=0 and LIBRARY/LDLIBRARY naming libpython's `.a`. Match this
no-shared-libpython/PIE policy and retain actual external ELF dependencies for
the new arms; do not conflate static libpython linkage with static libc linkage.

G reuse requires clean historical source, exact non-boil source equality,
explicit classification of every changed boil path, and exact equality of all
selected driver/kernel/build-audit inputs. Changed broad `boil/bench/run.py`
is not used. Bind actual Makefile/pyconfig, selected toolchain, complete product
index, library/extension origins, ELF interpreter/needed libraries and existing
qualification receipts. G image SHA256 is
`289cf16e3baf69e7cbc364a9b40a46f713e6604ae1fa8e6bf94afad7857f586f`.
Mismatch stops reuse; it does not authorize a replacement build or policy
change. Future managed worktrees and build directories are assigned only after
the coordinator grants materialization.

## Build, root and configuration gates

Reuse `build_once`, the accepted run-row supervisor, full worker/result contract,
raw-spooling stale wrapper and paired quiet protocol. Narrow adaptation points
are strict per-arm CC/config/backend admission, Clang-selected tool identity,
three scored runtime bindings and rotated three-arm ordering. Do not introduce
another executor. Existing GCC-only/no-tail validators must be updated, not
disabled. Hash their old/new bytes and replay positive plus altered-config,
source, product and timeout-output rejection controls before activation.

Every build executes existing `make -j4 all`, including the C root checker and
linked precise/inline audit. Preserve frame/unwind metadata, precise sections,
preserve-none classification and generated entry inventory. Unknown outlines
or audit failures stop the arm; no opaque exemption or audit relaxation.
`gc_inline_audit.py` contains Clang DWARF5 address handling, but that source
capability is not proof that the actual new image passes.

Require `_Py_TAIL_CALL_INTERP` true only for T/TD, computed dispatch true for
G/C, correct compiler path and Py_DEBUG/Py_STATS only for TD. Confirm actual
config and executing origins, not only requested configure arguments. Tail
handlers retain slots/tstate/IP/word plus lastopcode under Py_STATS; frame is
still reloaded from thread state. Preserve entry sentinel, throw/error labels,
predicted joins and early caller-result publication.

## Exact focused behavior roster

The following 21 methods run on G/C/T/TD, once per image in OFF/N0, for
84 selected method invocations. The sole expected skips are the last method
on G and C; it must execute on T and TD. No other skip is accepted on this host.

1. `test.test_ir.AssemblerTest.test_exception_from_a_pushed_frame`
2. `test.test_ir.AssemblerTest.test_frames_when_instrumented`
3. `test.test_ir.AssemblerTest.test_yield_to_each_kind_of_resumer`
4. `test.test_ir.RegCfgTest.test_call_result_to_a_local`
5. `test.test_ir.RegCfgTest.test_pushed_frame_result_to_a_local`
6. `test.test_ir.RegCfgTest.test_generator_keeps_the_loop_variable`
7. `test.test_ir.RegCfgTest.test_frame_holds_the_handler_depth`
8. `test.test_generators.ExceptionTest.test_except_throw`
9. `test.test_generators.GeneratorThrowTest.test_exception_context_with_yield_from`
10. `test.test_generators.GeneratorCloseTest.test_close_releases_frame_locals`
11. `test.test_generators.GeneratorDeallocTest.test_escaped_frame_preserves_mutable_payload`
12. `test.test_coroutines.CoroutineTest.test_stack_in_coroutine_throw`
13. `test.test_coroutines.CoroutineTest.test_call_generator_in_frame_clear`
14. `test.test_monitoring.MonitoringStorageTest.test_uninstrumented_generator_throw`
15. `test.test_monitoring.ExceptionMonitoringTest.test_throw`
16. `test.test_monitoring.TestBranchAndJumpEvents.test_callback_set_frame_lineno`
17. `test.test_gc.TracingCollectorTests.test_eval_loop_frame_is_not_a_root`
18. `test.test_gc.TracingCollectorTests.test_precise_frames_are_not_roots`
19. `test.test_gc.ManagedScanningTests.test_call_and_exception_frame_values_survive_collection`
20. `test.test_gc.TracingCollectorTests.test_finalizer_work_oom_recovers_next_collection`
21. `test.test_gc.StackWalkTests.test_tail_call_handlers_precise`

TD additionally runs all six `test.test_pystats.TestPyStats` methods:
`test_raw_ir_handler_entries`, `test_stats_toggle_on`,
`test_stats_toggle_on_thread`, `test_thread_exit_merge`, `test_stats_dump`,
`test_stats_clear`. Total: 90 selected outer method invocations, 2 nominated
backend skips. Validate the literal class/method/child roster from the frozen
source before helper acceptance. Count actual children only; parameterized
source templates or a parent success are not invented child telemetry.

The existing raw-entry stats test accepts dispatch-mode 0 or 1. Its child dumps
one raw `.txt` file into the method's TemporaryDirectory; the method parses it
and then deletes that directory. Preserve this SAME child and its actual raw
file before that cleanup, with no extra child, call, workload or test-source
change:

- Narrow the existing typed `_assert_python` observer to the active
  `test.test_pystats.TestPyStats.test_raw_ir_handler_entries` case and its one
  exact source-derived child-code hash. Retain natural script_helper isolation
  semantics; do not force `__isolated` or change its PYTHON_STATS_DIR argument.
- After the original `_assert_python` returns its real successful
  `_PythonRunResult`, and BEFORE returning that same object to the test method,
  inspect the original call's PYTHON_STATS_DIR. The child has exited and the
  parent is still inside the TemporaryDirectory context. Require the exact
  nominated case/code/child token, typed rc0 and one regular `.txt` file there;
  retain its byte-for-byte contents in a new exclusive evidence file and record
  its original path, content hash, returned stdout hash, child PID/token and
  observer origin/hash. Do not rewrite, rename or extend the original file.
- Validate the RETAINED raw bytes against the child's actual stdout target-ID
  JSON: coverage version2, dispatch mode1, matching handler identities and
  nonzero execution/pair counts for the actual per-operation, shared-feedback
  and relaxed targets. The original method must also succeed. Missing raw
  evidence or wrong mode fails the row; config alone cannot supply this witness.

This is parent-side artifact capture after the counted child exits, so it
introduces no allocation into that child's cleared/on/off stats interval.
The standard child prefix's existing perturbation remains explicit. Scope the
addition to that named method only; return the original result object unchanged
and leave all production assertions, source child bytes and target counts
intact. No new runner layer, settrace hook or GC/setter wrapper is proposed.
At helper review, host mocks must prove byte equality and capture-before-context-
cleanup, exact-case/code/directory binding, unchanged typed-result identity,
rejection of missing/duplicate files or mode0, and retained failure artifacts.
No extra ordinary-method or child-coverage credit is earned by copying a file.

Run the six unchanged canonical stale controls 100 times each on G/C/T in OFF
and TD in OFF then KILL:pymem,structs, for 30 control cases and 3,000 internal
iterations. Preserve their sorted opaque-callback placement. Only the stale
harness's no-tail configuration admission changes to exact arm expectation;
all six function bodies remain byte-identical. Retain raw stdout/stderr before
waiting, actual PID/mode/extras/N0, positive collection roots and zero unwind
fallback. Expected KILL auxiliaries, if any, get separate evidence and no
ordinary-test credit. No counters claim coverage for a skipped backend.

## Finite native and quiet windows

Propose one serial 60-minute build/focused-feedback window: G bridge/focus/stale;
C build/focus/stale; T build/focus/stale; TD build/focus/stats/stale-OFF/stale-KILL;
then complete-path emission capture. Three fresh builds, each capped at 15
minutes and clamped to remaining time; each behavior row capped at 5 minutes,
stale row 3 minutes, emission 3 minutes, all constrained by the same global
60-minute deadline. Row ceilings are not additive promises. Stop first
unexpected failure and preserve products, raw output, process tree and cleanup.
No retries, fallback compiler, missing-audit waiver or source correction within
the same accepted input set. G/C test21's exact skips are expected controls.

Only independently accepted behavior/emission permits a separate serial
30-minute quiet activation. Hard cap both windows at the original overall
2026-11-01 04:18:11 UTC deadline. If readiness plus remaining qualification
cannot fit, report/defer rather than silently expanding the clock.

The scored roster is frozen: coroutines loops4, generators loops2,
unpack_sequence loops4000, spectral_norm loops1. Two fixed rounds, five serial
timed calls per cell, one registered-loop warmup per scored process. Three
arms x four workloads x two rounds =24 scored processes and120 timed calls.
Twelve separate one-loop, zero-sample origin processes give36 total processes,
not registered-loop origin coverage. No kernel witness runs before this plan.
Use cyclic arm rotation per workload and reverse it in round2; freeze the
literal24-cell order in the execution manifest before observing a score.

Retain the accepted direct driver hot interval, CPU0/sibling8 reservation,
machine and three build locks, load ceiling1.5, fixed hashseed0/OFF/N0,
library/native origins, raw monotonic timing, peak RSS, final product hashes,
bounded timeouts and cleanup. No compiler, hashing, other native job or probe
may overlap scored time. Instructions:u remains denied under the retained
readiness receipt; no new permission probe, sysctl change or proxy veto.

For each G→C, C→T and G→T contrast, report all paired per-workload elapsed
ratios in both rounds. A beneficial contrast requires coroutine ratio<=0.99
both rounds, absolute benefit greater than summed within-cell MADs, and every
one-sided leave-one-out ratio<1 in both rounds. A repeated >2% regression in
any other original workload blocks adoption without a bounded resolution.
No automatic retiming, significance claim or favorable subset. Report absolute
RSS and code/stack/dispatch diagnostics; no arbitrary code-size veto. If tail
beats Clang-computed but loses GCC, report a backend effect without adopting
the slower combined configuration. Choose one useful supported backend, not
permanent experimental product options.

## Focused evidence versus landing

This is compiler/backend selection feedback, not full collector/evaluator
qualification. TD exercises debug assertions and the extra stats ABI, but does
not qualify every compiler x backend x debug/stats combination. A selected
backend needs its own exact later release full suite, debug KILL full suite,
stress1000, applicable ASan, complete generated-handler/tooling checks and
supported-build matrix before landing. Do not inherit those claims from G or
from the rejected slot-base image. An ABI change is VM1 and requires separate
source, review, controls and elapsed evidence.
