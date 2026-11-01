# Next RC integration tranche

2026-11-01. **Plan only.** No execution, compiler, source-change, merge or collector-selection authority. Keep clean RC source `d7df12b1d81722927e71d9734c70eeafdccfee73` and its accepted debug/release products unchanged.

If the independent review confirms the latest GC pilot's declared losses, select the prepared RC route now and use the remaining time for integration qualification and the JIT foundation. The 15:14:44 GC cutoff is a maximum, not a requirement to spend the remaining hours on speculative rewrites without a surviving measured candidate. This recommends an engineering stop; it does not prove tracing cannot win or establish RC elapsed parity. Root decides after the actual GC review. Preserve tracing branches and all rejected evidence. The accepted fallback retains register IR and inline frame values while deferring later core-wide PyValue/container/native-ABI work and header shapes.

## What is already closed

The first 39 selected methods passed on each image. The separate compatibility tranche passed 50 debug and 48 release methods, zero skips: 98 invocations across 50 new distinct methods and 37 actual target processes. Its independent review is `rc-compatibility-native-outcome-review-20261101-v4.md`, SHA256 `6fa12a662b568744709df60d518476cda7eadd12f70774267a6b011139ed8d7b`; receipt `578eabe7efeda542908ea88235bf57755cec260f0d16184d1bcfc1064d943668`. Twelve embedding programs, two expected refcount aborts and the natural startup probe have actual retained evidence. These are finite compatibility results, not whole-runtime qualification.

Canonical Autoconf/autoheader reproduction is accepted and byte-identical to d7; it is no longer an outstanding merge prerequisite. The unchanged native test-support modules also imported successfully; do not carry their former host-only Python 3.16 parsing limitation forward as an unexecuted import gate. Coverage of other stdlib modules remains open. No known source blocker was found by the accepted RC source review; the next task is to expose broader ownership errors, not assert their absence.

## One small next batch

This is the **final targeted tranche** before the existing broad stdlib/sanitizer/extension integration matrix on one source. Do not create another intermediate bespoke cohort unless an actual failure identifies a concrete gap. Reuse the existing two images, gate/serialization/build locks, result provenance and pre-wait child streams. No build row, allocator/configuration sweep, additional abort row or verifier qualification. Exact IDs, source/AST hashes and decorators are frozen in `build/rc-integration-next-plan-20261101/roster.json`, SHA256 `b8ac7071ec5e2933cd23493c67cf3990a53d9f9dd73c241e1df13f96778061c8`.

| Serial row | Exact selection | Expected method executions | Expected target processes |
|---|---|---:|---:|
| Existing debug | All 35 new roster IDs |35|10|
| Existing release | Same 35 IDs |35|10|
| Existing debug, `-R 4:5` | Eight exact roster IDs |72 bodies across nine iterations; final returned stats cover8|4|

The 35 new methods comprise all eight `test_frame.ClearTest` methods, all eleven `test_capi.test_eval.Tests` methods, twelve named `test_exceptions.ExceptionTests` methods, and four `test_embed.AuditingTests` methods. The literal JSON, rather than a class wildcard, controls selection. None overlaps the accepted 89 distinct methods.

The exception cases cover named-exception cleanup, generator exception-state save/restore and close/throw, MemoryError/RecursionError cleanup and nested yield/raise. The four embedding cases are `test_get_incomplete_frame`, `test_main_interpreter_view`, `test_thread_state_ensure` and `test_thread_state_ensure_from_view`: one `_testembed` each, expected exit0. They expose native allocation callbacks, borrowed frame views, guard/thread lifetime and finalization ownership without another broad embedding matrix. Their C-created threads are not additional processes.

The eight leak selections are the four named frame-clear ownership cases and four exception/generator cleanup cases listed in the roster. They repeat intentionally to test reference stability. The process expectation is **24**: three drivers, ten module workers, three platform auxiliaries and eight embedded programs. No selected test body calls script_helper or requests an abort. Record actual PIDs/returns; do not substitute this source expectation for an observed ledger.

Keep the current Linux/GIL/GCC/computed/mimalloc/boxed-shapes profile, own RC extensions, seed0 and historical diagnostic OFF request. Expect zero selected skips. One 30-minute inclusive window, three serial 600-second row ceilings downward-clamped to remaining time minus the existing 210-second cleanup reserve. Stop on the first unexpected failure, leak, timeout, missing origin/child or skip. No automatic rerun or assertion relaxation.

## Leak measurement needs a narrow observer change

Do **not** run the current method/result/child observers inside `-R`. Their growing ID/token collections and suite-record writes occur inside `test_func` and can manufacture reference deltas. Reuse standard `libregrtest.refleak.runtest_refleak` unchanged, with only provenance/start/end and parent scheduling outside the measured interval; no per-test object retention or writes inside it. This requires reviewed helper preparation, not a runtime/test patch or new executor.

Retain actual matched filters, `HuntRefleak(4,5, owned_report)` configuration, source binding, standard verbose output, final returned stats and raw reference/block/fd diagnostics. The harness returns the last suite result, not the sum of nine runs. Validate repeated IDs from natural output where available; otherwise explicitly limit dynamic coverage rather than invent 72 observed IDs. Standard consistent-positive-delta failure remains authoritative; suspicious nonuniform deltas must remain visible. An all-zero vector is not printed by this harness, so do not invent one from absent output. A host control must demonstrate that observer instrumentation adds no objects or writes inside the measured callback and that a standard injected synthetic leak is rejected before any native grant.

Retain the v4 full positional argv validator before every gate/Popen, including the literal shell/gate/LOCK/PY/CMD roles. Adapt the reviewed child observer only to the four exact embedding commands. Keep dual Python/`_testembed` cleanup and writable exclusive output checks. Preserve the earlier malformed-launch failure and source restoration; no historical failure earns credit.

## Selection, merge and JIT are separate gates

Passing this tranche supports adopting RC as the next working foundation; it is not sufficient to call the restored core fully qualified. Before a primary integration claim, run the ordinary supported Linux stdlib suite with its declared resource/skip policy and investigate each compiler/ownership failure. A whole-suite pass cannot be inferred from these 124 distinct selected methods. Rebuild required third-party extensions against the RC ABI; locally rebuilt limited-CAPI tests do not establish upstream abi3 or old Boil binary compatibility. Keep incompatible tracing extensions and pyc caches excluded.

A separate fresh sanitizer image and native stack-reference-debug coverage remain memory-safety obligations: existing `Py_DEBUG` implies `Py_REF_DEBUG`, not `Py_STACKREF_DEBUG`. Review sanitizer/allocator compatibility and exact build flags before that grant; do not rename a normal-image test as ASan coverage. Ported extensions needed by the immediate JIT workload must pass their existing ownership smoke tests; unrelated Windows/free-threaded expansion is not a prerequisite for this explicitly Linux/GIL foundation.

JIT work can proceed on the selected isolated RC branch after these critical behavior/leak gates, with strong owned/borrowed homes at materialization, calls, suspension, errors and deoptimization. It must not assume restored container/native PyObject ownership has already become core-wide PyValue. Primary integration, sanitizer/full-suite closure, external ABI compatibility and performance remain explicit coordinator decisions and tracked obligations.
