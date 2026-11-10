# Candidate 41 inline-policy experiment review

**Approve the bounded experiment after frozen41 captures complete. This is not approval to promote candidate41 or the experimental successor.** The proposed single `Py_ALWAYS_INLINE` annotation on `_PyFrame_PushOverlapping` is an appropriate test of the observed overlap-frame outlining. No runtime edits or native workloads were performed for this review.

## Scope and architecture

Reviewed `regression-triage41.md`, the current shared frame helper, its three `ceval.c` callers and exact-argument bytecode caller, and the definition of `Py_ALWAYS_INLINE` in `Include/pyport.h`. The triage's ELF comparisons are the call worker's evidence; this review does not independently reproduce its disassembly counts.

The helper implements one ownership-sensitive frame commit. The three affected `ceval.c` callers enter it after their guards and detach owners under the existing no-callback/no-failure interval. Annotating its declaration leaves its body, owning ABI, initialization, default guards, and publication order intact. It avoids a duplicate binder or a handler-specific implementation. This is a local compiled-code policy experiment, not a new calling convention or a broader structural redesign.

The macro placement before the return type matches the documented form. In optimized GCC/Clang-family builds it requests always-inline; in `Py_DEBUG` it expands to nothing, preserving the project's explicit protection against excessive unoptimized debug stack growth. Other compiler behavior remains governed by the existing portability macro. No compiler flags or second compiler are needed for this test.

The exact-argument bytecode operation also calls this helper for thread-owned frames. Its reported already-inlined code is an additional control: inspect it after the annotation for unexpected register allocation or footprint changes. The three newly outlined sites do not constitute the helper's entire source-level footprint.

## What the evidence does and does not support

The reported new native boundary at the shared helper is a concrete emitted-code difference. Its prologue/epilogue and caller argument handling can plausibly cost successful overlap calls such as `logging_silent`. The third call site changing the compiler's inline budget is plausible, but remains a heuristic explanation rather than a demonstrated compiler decision.

The triage correctly separates the coroutine regression. The dominant generator-owned caller declines generic overlap before its prologue; DIRECT-clear fallback and inspected downstream providers show no added static work explaining the observed slowdown. Forcing inline therefore has no demonstrated direct mechanism for repairing the dominant coroutine path. A subsequent coroutine change could come from layout effects and would need measurement; do not describe it as saved overlap calls.

The annotation may grow three hot callers, change register allocation, expose further nested inlining, or move hot code. Removing a 0x20c shared body does not bound net text growth or prove lower instruction count. The current keyword handler's scratch-induced stack protector can remain. Forcing this helper also must not accidentally defeat shrink-wrapping on generic overlap misses. The existing project warning about inline-induced performance loss applies directly.

## Falsifiable gates

1. Keep frozen41 binaries and capture identities intact until their evidence closes; use a new candidate identity for the annotation. Change only the shared declaration and a brief explanatory comment if needed.
2. Release disassembly must show the intended three helper calls removed. Inspect resulting caller text, stack reservations, spills, canaries, and aggregate text size. Recheck the generator-owned early miss: it should still avoid the overlap body and its prologue. Disappearance of the standalone symbol alone is insufficient.
3. Complete the proposed release/debug builds and focused ownership/frame/default/keyword checks. Debug behavior should remain unchanged by the macro; compilation still validates the declaration. Preserve every current-default and keyword guard.
4. Compare balanced logging and keyword measurements against both frozen41 and golden39, retaining raw instruction and elapsed evidence. Treat coroutine and DeltaBlue as regression controls. A logging improvement tests the mechanism; unrelated movement does not establish it.
5. Retain the annotation only if saved application work produces a repeatable benefit without material new regressions or unacceptable hot-code growth. Restoring inlining alone is not a success criterion. Broader qualification remains necessary before promotion, and the unexplained coroutine gap remains open even if logging improves.

## Instruction helper follow-up

The revised `capture-micro-instructions.py` addresses the two earlier capture blockers. Preflight and command execution now record failures inside a protected region; postflight snapshot exceptions are separately retained; final status records `complete: false` on either failure. A candidate mismatch is added to snapshot errors. Missing candidate metadata may still raise inside the snapshot, but the outer handling now records that rejection rather than omitting status.

Every retained perf finish must now have zero exit status, no recorded error, exactly one instructions event, a positive event runtime, and running coverage between 99.9% and 100%. Coverage rejection preserves the raw record and leaves `coverage_verified` false. The success predicate requires verified coverage for all three roles and equal valid snapshots. These changes stay in the new wrapper and leave the frozen timing helper untouched.

Approve this helper for its first capture. Its finish/start length check is not a full result-shape audit: inspect the actual output for the expected three entries, four low/high pairs per entry, matching invocation identifiers, and 28 successful perf finishes per role before admission. Report the coverage threshold accurately as at least 99.9%, not guaranteed exact 100%. Raw and skeleton-subtracted instruction semantics remain as documented in `micro-instructions-wrapper-review.md`.
