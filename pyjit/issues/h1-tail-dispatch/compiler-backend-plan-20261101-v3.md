# VM0 revision 3: unchanged-runtime compiler/backend control

State: review. 2026-11-01. **Host/source proposal only.** No compiler, target,
probe, workload, source patch, new worktree or activation is authorized here.
The accepted revision 2 remains byte-exact at
`build/vm-design-canvas-20261101/vm0-plan-v2-eb51a54.md`, SHA256
`eb51a54a0a56fbadfffeda4e22901157f6664d3ecf5c462de8ef06db6810a6b7`.
Its original 04:18:11 window expired. Its helpers and activation fields must
not be reactivated. This is a separately reviewed future scope.

## Why this control now

The actual comparison-region package lost 10.86%/14.11% on coroutines and
16.21%/13.74% on the four-row geometric mean. Complete linked paths show the
intended home/dispatch removal, extra admission conditions and frame/IP/slot
stack reloads. They do not establish the frequency or cause of that loss.
Before another local IR rewrite, test the compiler and existing backend on
unchanged runtime source. Tail dispatch is a hypothesis, not an assumed win.

Keep source `417a9443a599e879c76c34c13ae76943bd6f6b3e` in the existing managed
`build/worktrees/vm-compiler-backend`. Its runtime paths equal qualified e9
`e9e10d5b503eaf219797a5db44766eeab6fad4dc`; current source is clean. Rebind all
actual files, products and selected driver/kernel bytes before execution.
No region40498, slot-base ABI, GC span, admission, value-layout, handler or
frame-publication change enters this experiment.

## Three contrasts, one fixed runtime

| Arm | Exact release configure arguments | Purpose |
| --- | --- | --- |
| G | `CC=gcc` | reuse qualified e9 computed-goto O3 image after exact bridge |
| C | `CC=/usr/bin/clang-21` | compiler plus existing compiler-selected source policy |
| T | `CC=/usr/bin/clang-21 --with-tail-call-interp` | existing tail backend under the same compiler |

C/G is the compiler contrast; T/C is the backend contrast; T/G is the combined
configuration that matters to users. Report all three. A T/C gain with a T/G
loss earns backend evidence, not adoption of the slower combined configuration.

All scored arms are Linux/GIL/mimalloc, O3, nonstats, no shared libpython,
PIE/ET_DYN, no JIT/PGO/LTO/ASan. Preserve the existing frame-pointer and zero-init
policy. GCC's source-selected evaluator -fno-gcse and x86-64 no-SLP workaround
remain; do not feed unsupported GCC flags to Clang or remove maintained GCC
policy to create an artificial match. Record exact expanded compiler/link
arguments and configuration differences. Clang22's source guard for huge
computed-loop inlining does not apply to Clang21; that fact predicts no result.

Reuse G's actual release image SHA256
`289cf16e3baf69e7cbc364a9b40a46f713e6604ae1fa8e6bf94afad7857f586f`.
The existing qualified GCC O1/debug image can supply its historical correctness
control after the same complete bridge, without a new GCC debug build or fresh
method credit. Reuse failure stops; it does not authorize replacement builds.

The installed path `/usr/bin/clang-21` resolves to
`/usr/lib/llvm-21/bin/clang`, observed SHA256
`412bbe8c60571a1eb06f48fde89635033621caeb01a9b4ee76d46711bae8e932`.
This is file identity, not an executed capability/version witness. Actual
version, frontend/shared-library dependencies, integrated assembler and selected
linker/tool bytes remain readiness fields for a later authorized preparation
step. Do not relabel GCC's cc1/collect2 discovery as a Clang protocol. No compiler
query or tool probe ran in this revision.

## Literal cheapest correctness gates

First bind G's source/config/products/tools/ELF and accepted historical evidence.
Then propose these fourteen serial rows under **one 45-minute inclusive native
window**, with the inherited 210-second terminal reserve:

1. G release: focused21; canonical stale6×100 OFF.
2. C debug: fresh build; focused21; canonical stale6×100 OFF.
3. C release: fresh build; focused21; canonical stale6×100 OFF.
4. T debug: fresh build; focused21; canonical stale6×100 OFF.
5. T release: fresh build; focused21; canonical stale6×100 OFF.

Thus four new Clang images, five focus rows and five stale rows; **105 selected
outer method invocations**, with exactly three nominated computed-backend skips
(G release, C debug, C release). Tail debug/release must execute all21. Canonical
stale exposure is **30 original child controls / 3000 internal iterations**.
Count actual nested children separately from outer methods and ordinary workers.
No stats build, stats census or extra verifier row is required before elapsed.
The previous TD/raw-counter plan is preserved and deferred to selected-backend
landing qualification, including its extra lastopcode ABI.

Use fresh owned C/T directories in the existing worktree; assign their names
only in a later materialization grant. Preserve all expired helper directories
and existing GCC stats image. Do not reuse stats products as nonstats images.
Per build ceiling600s, focus300s, stale180s; all clamp downward to the same
remaining window and reserve. Ceilings are not additive time promises. Stop at
the first unexpected source/config/build/import/test/skip/root failure. Preserve
raw streams and idle cleanup; no retry, fallback compiler or audit waiver.

The literal focused roster remains:

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

Use the unchanged source-bound six stale bodies and opaque-callback placement;
change only exact compiler/backend configuration admission in their wrapper.
Require actual OFF/extras0/N0, positive ordinary-PID collections, zero unwind
fallback, typed nonnegative report-only unpinned and complete cleanup. No count
can credit a skipped tail test. Preserve the current worker/observer tokens,
module/CAPI/source origins, hashseed0 and natural child isolation.

## Existing frame and root ABI is the contract

`Python/ceval_macros.h` passes slots, tstate, next_instr and one IR word in the
nonstats tail ABI; handlers still reload frame from tstate. Both indirect
DISPATCH and label transfers use musttail with preserve_none. No passed-frame
ABI is implemented. Preserve caller-result publication, current_frame, live
slots, IP/exception positions, throw/error and entry-sentinel handling, observer
visibility, generator suspension/resumption and breaker service.

Every fresh `make -j4 all` must pass the existing C root checker and linked
precise/inline audits. No unknown-outline exemption or checker weakening.
Tail handlers/labels must remain in the precise preserve-none section. Method21
checks actual handler PCs and register ownership both in an ordinary walk and
an error-label monitoring callback; requested tail configuration is insufficient.
Inspect complete emitted dispatch/receiving/call/yield/return/error paths from
saved linked products after behavior, with a separately reviewed finite static
capture if no existing capture suffices. Ordinary function-call recursion or
an unrecognized root/unwind boundary blocks scoring. Diagnostics are not speed
vetoes: actual elapsed and resource tradeoffs decide.

Reuse the accepted build_once/run-row supervisor, fixed-worker contract,
raw-spooling stale wrapper and quiet supervisor. Narrow changes are exact
per-arm compiler/config/tool admission, three runtime bindings and the frozen
three-arm order. Create and actually preflight every evidence/tmp/result output
directory. Exercise actual cold/cached import closure and raw timeout survival.
No new executor, flags search, global nursery option or runtime policy switch.

## Fixed elapsed decision after accepted native feedback

Before any backend score, freeze this roster: coroutines loops4, generators
loops2, comprehensions loops6000, Richards loops2. These retain the actual recent
pilot's global controls; no VM0 arm has yet been measured. Two fixed rounds,
five serial samples after one registered-loop warmup per scored process:
**24 scored processes /120 values**, plus **12 one-loop zero-sample origin
processes**, 36 total. Rotate G/C/T by workload index in round0 and reverse
that row order in round1. Freeze all literal argv/order before activation.
No kernel witness outside these12 origin processes.

Propose a separate 30-minute inclusive quiet window, with downward ceilings and
210-second cleanup reserve. Reuse CPU0/sibling8, both machine slots, all three
build locks, load ceiling4 and exact policy/origin checks. Keep raw monotonic
vectors, GNU-time whole-process RSS, spreads, before/after products and terminal
idle evidence. No overlapping native, compiler, bulk hash or probe work.
Instructions:u remains denied; no new probe, sysctl change or proxy veto.

For each of C/G, T/C and T/G report every row in both rounds. A useful contrast
requires coroutines<=0.99 and four-row geometric mean<=1.0 in both rounds,
benefit greater than summed within-cell MADs, and every one-sided leave-one-out
ratio<1. A repeated >2% regression on another fixed workload blocks adoption
until a separately bounded resolution. Borderline/inconsistent outcomes are
inconclusive; a clear loss is rejected without broad qualification. No automatic
retiming, extra samples or favorable subset. RSS/stack/text and linked-path
changes explain tradeoffs; no arbitrary instruction/text/RSS cap is added.

## GC deadline and later deployment

**VM0 is independent IR research, not GC parity credit.** The current user GC
budget froze GCC/O3, all29 registered workloads and cutoff **15:14:44 UTC**.
That budget and registry remain byte-preserved. Clang-versus-GCC pilot scores
cannot satisfy it. Any proposal to count a Clang deployment toward that decision
requires an explicit fair protocol amendment and matched upstream Clang all29
plan declared before backend results, with the same cutoff/registry/no-JIT
requirements. No such amendment is made here. Do not subtract a compiler pilot
speedup from an earlier GC result or combine isolated gains arithmetically.

A useful supported configuration still needs full selected release/debug/root/
stress/ASan and extra stats-ABI qualification before landing. A later coherent
all29 result must use its actual source/collector/compiler package and matched
upstream policy. Tail is not an architectural endorsement; a frame ABI or IR
rewrite is a separate source hypothesis after these controls.
