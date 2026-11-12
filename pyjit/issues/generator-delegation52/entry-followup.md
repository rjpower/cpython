# One bounded followup: move resume-entry preparation off the unobserved path

Status: implemented and rejected. Candidate `e85171923c0` moves the targeted metadata work cold, but six paired runs lose on all three targets and all29 is 2.0% slower than golden51. The [final report](report.md) owns admission. The proposal and review record below preserve the original hypothesis; no further runtime changes are queued.

The six-pair comparison with round51 confirms regressions in generators (1.043694), coroutines (1.046551), and deltablue (1.026699). Every pair loses in those three kernels. This proposal removes one demonstrated source of new common-entry work. It does not predict that removing it will recover those regressions.

## Evidence and scope

In the current release ELF, `_TAIL_CALL_DELEGATE_GEN` at `0x379970` has size `0x4f5`; current legacy `_TAIL_CALL_SEND_GEN` has size `0x47c`, versus `0x2e7` in frozen51. Sizes alone do not identify execution cost, but the entry instructions do:

- `0x379ad0`: load the resumed frame's saved instruction pointer; `0x379ad7`: load its executing word.
- `0x379ae0`: check the frame owner; `0x379aea`: load `frame_obj`; `0x379af4`: test it; `0x379af9`: conditionally load `f_lineno_generation`.
- Only then do `0x379afd` and `0x379b08` compare the actual handler with `INSTRUMENTED_LINE` and `INSTRUMENTED_INSTRUCTION`.

This release has no DTrace entry hook and no compiled LLTrace entry hook. Nevertheless, generation preparation survives before the wrapper test on the ordinary path. Frozen51 reaches its guarded RESUME skip without that preparation. The new common emitter entry affects legacy generator iteration too, so it is broader than DELEGATE alone.

The unchanged generators kernel delegates through exact child generators from `Tree.__iter__`. Its balanced tree and lack of observers suggest many forwarding-eligible ancestors per value; this is a source inference, not a measured hit count. Its terminal yield still performs the real per-frame suspension transitions. The coroutine Fibonacci kernel returns through recursive awaits without yielding, so yield-side forwarding supplies no benefit there. Entry overhead therefore matters even when the forwarding loop works correctly.

## Exact factoring

Keep the full existing `IR_FINISH_RESUME_ENTRY` sequence when `Py_DEBUG` or `WITH_DTRACE` is defined. Using all `WITH_DTRACE` builds is a conservative compile-time gate; the effective hook guard is narrower (`WITH_DTRACE && !Py_BUILD_CORE_MODULE`). Do not select this path using runtime probe enablement. This preserves the existing snapshot before DTrace and LLTrace, then preparation after the last escaping hook, with same-PC generation intent and changed-PC redirects winning over skips and continuation targets.

In other builds both entry hooks compile to no-ops. Factor the macro as follows, using the existing helper and status processing:

```text
site = frame->instr_ptr
handler = atomic_relaxed_load_handler(*site)
if handler is INSTRUMENTED_LINE or INSTRUMENTED_INSTRUCTION:
    generation = resume_entry_generation(frame)
    result = prepare_resume_entry(tstate, frame, site, generation)
    next_instr = result.next
    if result is error: enter normal error path
    if result is REDIRECT: do not skip
    else: IR_SKIP_RESUME()
else:
    next_instr = site
    IR_SKIP_RESUME()
```

Use the same atomic word-load policy as the existing preparation helper. Do not decode the immutable opcode, inspect frame objects, or snapshot generation before recognizing an actual outer wrapper. The existing helper may retain its defensive owner and handler checks on this cold branch; eliminating those repeated checks is outside this increment.

Both call sites remain covered: native `start_frame` and emitted `IR_ENTER_GENERATOR`, including exact generator/coroutine entry through DELEGATE, SEND, and FOR_ITER. Fresh-call `_PUSH_FRAME` stays unchanged. No new evaluator, per-frame cache, or allocated entry record is introduced.

## Preserved sequencing and admission

The entry macro still runs only after input publication, pending-redirect resolution, current-frame/exc/recursion links, and the saved semantic PC are coherent. The no-hook fast branch performs no escaping operation between loading that selected PC and its existing guarded skip. A redirect caused by earlier input retirement has already selected this PC; the branch does not restore an earlier cached one.

Outer LINE/INSTRUCTION wrappers still enter the existing preparation helper before their first callback. A stale RESUME_DELEGATE wrapper therefore receives instrumentation synchronization first. Errors retain the helper's selected-site-plus-one cursor. Redirect status, including same-PC intent during synchronization, suppresses skip. Preparation never selects the encoded continuation target or emits PY_RESUME.

Plain RESUME_DELEGATE and its inner instrumented twin retain their existing operation-level synchronization before resume events. Ordinary entry retains `IR_SKIP_RESUME` unchanged: its opcode, eval-breaker/epoch, TLBC, and Emscripten guards remain authoritative. The DELEGATE owner guard, constructor proofs, held-owner forwarding predicate, cold suspension landing, completion path, and throw/close behavior remain unchanged.

## Acceptance and measurement

Review the actual generated entry at both call sites and the compiled release DELEGATE_GEN/SEND_GEN handlers. The ordinary no-wrapper branch must no longer load `frame_obj` or `f_lineno_generation`, or execute the owner check solely needed for that snapshot. Wrapper recognition and the existing guarded skip remain. Debug/DTrace source must retain the full old hook sequence.

Retain the current stale-wrapper regression coverage, including instruction and line observers, callback/error and same-PC redirect cases, native/exact entry, and debug instrumentation sanity. Run the already established focused release/debug suites; this proposal adds no new semantic state requiring another test family. Root chooses the build/test window and reruns the same paired comparison with this followup as the sole candidate change. No promotion is justified by assembly simplification alone.

Runtime peer review: accepted by the runtime lead. The conservative all-`WITH_DTRACE` guard is sound; the fast branch contains no escaping call between selecting the PC and the guarded skip, and the cold branch retains synchronization's redirect/error authority. Compiled placement and paired measurements remain pending.

Actual implementation peer review: accepted in the separate `delegation52-entry` worktree. The sole runtime diff is `Python/ceval_macros.h` (+29 lines); the prior macro body is unchanged under the conservative compile-time gate. The new arm recognizes actual wrappers with the existing atomic-load policy before any generation snapshot. Both native and generated exact-entry call sites retain the existing coherent input/frame boundary. Whitespace checking passed; no build or native execution was performed by this reviewer. Compiled placement and paired measurements remain pending.

Compiled placement: passed for followup commit `e85171923c08b0d68ee169119e761d7583bbcb26`, using read-only inspection of `delegation52-entry-release/python`. DELEGATE_GEN loads the selected PC/word at `0x379b40`/`0x379b47`, recognizes wrappers at `0x379b50`/`0x379b5b`, and otherwise reaches RESUME/RD recognition and the existing epoch skip without a frame-object or generation load. Owner, frame-object, and generation preparation occur only on the wrapper branch at `0x379bf8`, `0x379c06`, and `0x379c15`. SEND_GEN has the same placement (wrapper tests `0x37b290`/`0x37b297`; cold owner/frame-object/generation at `0x37b2e8`/`0x37b2f1`/`0x37b300`). Native start_frame likewise tests wrappers at `0x34a0f4`/`0x34a0fb` before cold preparation at `0x34a138`/`0x34a140`/`0x34a14d`.

Total symbol sizes grew (DELEGATE_GEN `0x521`, SEND_GEN `0x4a8`, start_frame `0x29b`), while the targeted ordinary-path work was removed. This establishes placement only; performance acceptance remains pending root's paired measurements. No candidate code was executed for this inspection.

Final outcome: release imports 117 modules and passes 462 focused tests with one skip. Debug/stats were configured only after the release performance failure. Six-pair golden51 ratios are 1.009530 generators, 1.113011 coroutines and 1.118550 DeltaBlue. All29 ratios are 1.020473 versus golden51 and 1.172901 versus upstream. Compiled placement passed; throughput admission failed. Golden51 remains unchanged.
