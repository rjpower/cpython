# One upstream checker compatibility control

2026-11-01. **Propose one isolated upstream successor with exactly one assignment correction, then the original three-method comparison.** This is a design for independent review. It grants no source creation, build, target run, gate amendment or RC qualification.

Keep the pristine321925 checkout, its two historical builds and the failed `build/upstream-stackref-control-20261101` intact. After design acceptance and a separate root grant, create one project-managed source worktree from321925, for example `build/worktrees/upstream-stackref-control`, and a fresh out-of-tree `build/upstream-stackref-control-corrected-20261101`. Do not reuse partial objects, edit the pristine checkout or modify RC.

The entire proposed runtime delta is `Python/frame.c:64`:

```diff
-    new_frame->f_executable = PyStackRef_DUP(new_frame->f_executable);
+    frame->f_executable = PyStackRef_DUP(new_frame->f_executable);
```

The patch artifact applies with `git apply --check` to the clean parent; it has not been applied. All other source bytes, methods, configuration and checker behavior remain unchanged. This is already the RC assignment, not a newly invented debug exemption. In symbolic token terms, MakeHeapSafe consumes sourceA and gives destinationB. The original assignment creates destinationC while sourceA stays closed; the eventual source clear then fails. The proposed assignment gives sourceC and preserves destinationB. The same reference ownership balances in the ordinary representation. No token check, close, report, finalization or GIL rule is disabled.

Freeze a clean exact successor and a whole-tree proof of the single1+/1− line delta. Independently review it before helpers/native authority. Do not transplant other RC frame/home/IR changes. Label every later result **upstream321925 plus the one ownership correction**, never unmodified upstream or a fully qualified upstream runtime. The original failed pristine attempt stays failed. The lack of a dynamic creation trace for55984 remains explicit; a successful corrected bootstrap would be causal intervention evidence, not proof of every subsequent failure's origin.

Use the same GCC/computed/mimalloc/O1-debug `CFLAGS=-O1 -g -DPy_STACKREF_DEBUG` profile and captured selected toolchain. No Boil shapes/IR flag, ASan, CLOSE_DEBUG, verifier, alternate compiler, external freezing Python or bypass of `_bootstrap_python` is allowed. An existing normal build Python used only to skip freezing would leave the faulty runtime ownership path in place and is not an adequate replacement comparison.

Reuse the existing two-row supervisor/gate/worker design in a new held helper directory: configure/make1200sec, then one standard regrtest process for180sec, plus210sec verification/cleanup per row, within one1800sec inclusive window. Start that window only at a future explicit root activation after exact source/helper readiness and actual native idleness. No continuation or reset of the failed attempt. Preserve the canonical outer lock and both machine slots/per-build lock. The narrow cleanup adaptation must include the prospective `_bootstrap_python` and `Programs/_freeze_module` paths alongside Python/_testembed, in the sweep, idle check and receipt. Missing not-yet-built paths are legal; executable identity/startticks/pidfds remain authoritative. Retain raw output before waiting and core-pattern/project-core metadata without deleting external dumps. No new executor or compiler-cleanup framework is needed.

The unchanged three exact standard methods are:

- `test.test_embed.EmbeddingTests.test_asyncio_task_during_interpreter_clear`
- `test.test_faulthandler.FaultHandlerTests.test_dump_traceback_later`
- `test.test_threading.ThreadTests.test_join_daemon_thread_in_finalization`

Their source/body hashes and decorators are frozen in `proposal-source-proof.json`; the embedding C entry file is also bound. Do not alter methods, daemon-thread behavior, last-GC behavior or faulthandler expectations. Preserve standard regrtest/JUnit output for all three even when methods fail. Require all three to run without skip/missing/abnormal runner result; classify actual standard failures as comparison data. Threading's two subtests are not two additional selected methods. No full upstream rerun, profiler fourth case, new warmup, timing or repeated sampling is proposed.

Compare actual diagnostics, phase and ownership domains against retained RC failures. A matching open-record/last-GC or no-GIL signature can support a separate narrow applicability decision; it cannot automatically excuse every failed RC module. If upstream passes a path that RC fails, isolate the relevant fork delta. If this one correction still cannot build or produce all three complete standard results, stop this baseline attempt and return the unresolved gate to root; do not chase further upstream fixes or weaken checks. Any RC gate change still requires explicit review of actual comparative evidence.

Read-only evidence and proposed patch: `build/rc-stackref-upstream-build-failure-review-20261101-v1/`. No source worktree, compiler or target was created/run by this design.
