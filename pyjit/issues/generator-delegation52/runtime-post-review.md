# Independent runtime post-review

2026-11-12. Source review in candidate
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/delegation52`,
against round51 `0e5e61f4c6b`. This review is independent of the runtime
implementation; the reviewer authored the compiler slice. No builds, native
execution, tests or runtime edits were performed for this review. Initial
coordinator regeneration is complete; executed evidence remains pending.

## Findings

**Fixed in source, reported to root and runtime owner:** `Python/ceval.h:230–236`,
`lltrace_instruction`, treats `_PyIR_BaseOpcode` output as a legacy opcode.
New IR-only primary operations retain IDs 326–331, but `OPCODE_HAS_ARG(shown)`
and `_PyOpcode_OpName[shown]` use the legacy domain. Debug LLTrace at a new
operation can index outside that domain. Port the displayed name and argument
classification through the actual IR format/IR-only name domain while preserving
legacy output. Root authorized a ceval.h-only fix before compilation. The
actual fix now selects `_PyIR_OnlyNames` and format `.imm` for the high domain,
and reaches `_PyOpcode_OpName`/`OPCODE_HAS_ARG` only for the legacy domain. This
closes the source finding. Debug execution remains pending. No test was run to
demonstrate the original failure.

## Source assessment

| Boundary | Actual source reviewed | Assessment and remaining evidence |
| --- | --- | --- |
| Admission and frame capability | `bytecodes.c`, `_CHECK_GENERATOR_OWNER` before generic/exact DELEGATE admission and SUSPEND take | SystemError precedes generator conversion and owner movement in the DSL. Inspect generated statement ordering after regeneration. |
| Terminal owner | `ir_generator.py:suspend` and `_SUSPEND` DSL | One detached canonical owner becomes `ir_yielded`; explicit resume assignment replaces incrementing the caller PC. Each iteration keeps that owner, performs actual suspension/link/recursion/state publication, then repeats or calls the existing recipient transaction once. No recursive C evaluation or outcome allocation is introduced. Verify generated home NULLing and native loop. |
| Canonical/fusion binding | `canonical_frame_transfer_input`, `delegate_yield_guard_proven` | The producer proof matches guard/TAKE/terminal AST-token bodies; fusion additionally matches the actual compiled C predicate body, including preprocessor arms. A changed predicate disables direct propagation rather than enabling unchecked transfer. Generated body and mutation evidence remain pending. |
| Pure failed checks | `ceval.c:2134`, `can_propagate_delegate_yield` | Actual generator/redirect and continuation checks precede code/epoch/observer checks; no synchronization, owner movement or source scan. It checks real image identity, epoch, physical NULL recipient, active DELEGATE kind, all monitoring/frame-object/tracing/profile/PEP523 exclusions and DTrace/LLTrace; free-threaded builds reject. Shallow callers still pay the new check, requiring measurement. |
| Escaping hooks | `ir_generator.py:suspend`, `ceval.h:maybe_lltrace_resume_frame` | DTrace return remains before unlinking; restored caller remains at its pending-child DELEGATE PC through LLTrace, with the held value absent from B+2. Eligibility is queried afterward. A failing query publishes through the ordinary transaction before selecting SUSPEND recovery. LLTrace is escaping, not assumed pure; its separate name-domain bug is above. |
| Redirect and cold landing | terminal emitter, `_PyIR_PrepareSuspendLanding` | Snapshot pending redirect/generation around the ordinary consuming recipient transaction; refresh the selected continuation on redirect. Cold synchronization occurs with the result in its real home. On instrumentation error the helper selects actual SUSPEND and the emitter uses site+1. The helper preserves previous PC on success for source-edge delivery; newly patched words dispatch afterward. Verify dynamic monitoring installation and same-PC/error tests. |
| Resume | `ceval.c:_PyEval_ResumeDelegate`, `ceval_macros.h:IR_SKIP_RESUME` | Fallible synchronization/PY_RESUME keeps site+1 until success. Callback errors with committed redirects use the actual committed PC; successful redirects beat CD, including generation-only intent. Uninstrumented skip uses the existing full breaker/version and TLBC/Emscripten guards. Delegated flags retain the innermost polling rule. Native/exact entries share that macro; runtime behavior remains to qualify. |
| Native protocol and completion | `genobject.c:is_resume`, generator entry emitter, `ceval.c:_PyEval_CompleteGeneratorReturn` | Close recognizes both new resume variants; yield-from receiver remains return_slot-2. Exact entry treats DELEGATE as send-style input detachment. Immutable DELEGATE completion uses canonical shared completion with its literal kind, preserving ordinary CALL. Throw forwarding and native throwflag entry remain unchanged. Generic providers retain materialized outcomes and their lost specializations remain an explicit cost. |
| Handler domains and stats | actual IR-only definitions, BaseOpcode consumer port, pystats headers | BaseOpcode deliberately preserves high primary/twin identities. Statistics arrays already use IR_NUM_HANDLERS, so new STAT_INC calls do not establish another legacy-256 indexing defect. LLTrace still did; full generated handler/metadata review is pending. |

## Review limits and required follow-up

Source structure supports the intended yield-side mechanism, subject to
native verification. This is not release/debug
qualification or a performance approval. Root must provide generated artifacts,
focused callback/owner/protocol results, emitted loop evidence and paired timing.
Append their actual identities/results here; do not infer a pass from source
intent or count the design's profile shares as saved work.

## Initial generated-source inspection

Inspected the coordinator-regenerated `Python/generated_ir_cases.c.h` and
`Include/internal/pycore_ir_metadata.h` in R52 while its release build was
running. This is generated-source evidence, not executed or native-code evidence.

- `TARGET(SUSPEND)` checks actual generator ownership before reading/taking its
  outgoing owner, writes `window[0]=NULL` once, asserts canonical `temp`, and
  initializes exactly one `ir_yielded` C owner.
- The actual emitted `for (;;)` applies each frame's suspension state/link and
  recursion transaction, runs the preserved restored-caller LLTrace boundary,
  then calls the bound pure predicate. A hit updates recipient/resume/kind and
  continues without writing the parent's B+2. A miss calls the ordinary
  consuming recipient transaction once and dispatches its selected continuation.
  No parent SUSPEND dispatch or result publication appears in the loop's hit
  arm. This establishes the intended generated mechanism, not a net speedup.
- `TARGET(INSTRUMENTED_SUSPEND)` invokes PY_YIELD with the actual input home
  populated, retains the existing full publication boundary and emits
  `ir_suspend_site=NULL`, so that instrumented terminal never grants fusion.
- Generic DELEGATE checks actual frame ownership before provider work; exact
  entry detaches its input and commits the real B+2 recipient and immediate
  SUSPEND continuation. Generic yielded providers publish cursor/result homes
  before cold landing synchronization. Completed providers use the literal
  DELEGATE shared completion path.
- Both generated resume handlers retain site+1 and delegate fallible sequencing
  to `_PyEval_ResumeDelegate`; neither selects CD before that helper succeeds.
- Metadata IDs 326–331 have the intended pristine/specialized/instrumented
  classifications and canonical family mapping. DELEGATE retains A/B/C/D and
  optional unary feedback; SUSPEND declares suspension, and RESUME_DELEGATE
  declares A flags, CD jump fields and unconditional jump behavior. Stats arrays
  already span IR_NUM_HANDLERS. The actual generated direct route is enabled;
  it did not silently fail its source proof and fall back to ordinary execution.

No additional concrete runtime blocker was found in this source/generated pass.
Release/debug protocol, callback, malformed ownership, failed predicate overhead,
native loop and matched timing gates remain required before promotion.

## Initial coordinator build evidence

Root reports the initial release C compilation passed. The build then failed
while `_freeze_module` compiled builtins and importlib._bootstrap_external:
`ValueError: code: invalid delegation or suspension contract`. Evidence is
`BASE/pyjit-link-wave1/generator-delegation52/initial-release-build.log`.
This is a failed bootstrap, not a qualified candidate or a native semantic pass.
The consumer owner is diagnosing the exact admission branch. Independent source
inspection identified an overstrict near-target check: rejecting every
`physical == s+3 && done == physical` also excludes a legitimate near done
immediately after the compact triple when its cold throw block is elsewhere.
Only an actual IR_JUMP adapter claim should require normalization. This was
reported to root/consumer. Their diagnostic confirmed that exact branch:
`async_callable_iterator.__anext__` at pc10 and `PathFinder.discover` at pc47
both had ordinary near D=2 completion. The consumer owner restricted malformed
adapter rejection to an actual IR_JUMP shape, preserving near done admission
and exact adapter field/target checks. Their bounded rebuilt freeze targets for
builtins and importlib._bootstrap_external exit 0 in
`consumer-bootstrap-repro2.log`; the diagnostic failure is retained in
`consumer-bootstrap-repro1.log`. I inspected the actual source fix and logs.
No compiler change was needed. This closes that admission mismatch; it is not
full runtime qualification.

## Compiled suspension inspection

Inspected release `_TAIL_CALL_SUSPEND` with `nm`/`objdump` after release-build-4
and debug-build-2 each checked 117 modules. The inspected release binary SHA256
is `9134e77c03cb23da47261ee029cec70fae019f5468e9e631ae1033f8c332dbab`;
its `Python/ceval.o` SHA256 is
`7744925eb5a2224ec85f20073c0441e6d59dd62ee8b3891fe98e6caf5679856e`.
This was binary inspection, not workload execution or forwarding-hit evidence.

The function starts at `0x344db0`, size `0x509`. Its incoming owner remains in
`rdx`; the initial outgoing home is cleared once. Admission is inlined. On a
successful parent match, `0x344fb7` jumps backward to `0x344e21`, reapplying the
parent's suspension state/link/recursion stores without publishing B+2 or
dispatching a parent handler. The ordinary publication store is later at
`0x344fef`; tail dispatch is at `0x345083`. Thus the compiled image contains
the intended iterative mechanism rather than repeated helper calls.

The coordinator's bootstrap crash exposed a separate terminal access bug:
generation snapshots read an interpreter trampoline's intentionally
uninitialized frame_obj. The actual fix now gates both generation reads by
actual generator ownership, and the emitted source does likewise. In the
inspected binary, interpreter/non-generator callers branch away before those
frame_obj reads; related pending_redirect/return_pc/return_slot are initialized
by entry construction. This source/native finding is closed, subject to the
coordinator's focused execution results. Shallow callers still incur admission
checks and the larger handler; this inspection establishes no speedup.

## Resume entry sequencing peer review

The debug observation failure showed that an INSTRUCTION wrapper at a real
RESUME_DELEGATE could run before that operation refreshed stale instrumentation.
Approved the runtime proposal for a shared `IR_FINISH_RESUME_ENTRY` at native
start_frame and terminal exact-generator entry: after incoming ownership and
frame/exception/recursion commit, snapshot actual PC/generation, run existing
DTrace then LLTrace hooks, prepare the immutable selected resume site's epoch,
and only then apply eligible RESUME skipping on nonredirected success. The
preparation performs no PY_RESUME work or CD selection. A redirect, including
same-PC intent, wins; an error uses the actual winning site+1 cursor.

Moving the skip after hooks preserves their visible frame PC, which was already
the real resume word. Fresh-call `_PUSH_FRAME` does not resume this continuation
and need not change. Peer conditions: never inspect uninitialized interpreter
trampoline frame metadata, and recheck PC/generation after fallible epoch
synchronization as well as before it. Keep the sanity check and real instruction
event. Implementation/generated/debug rerun review is still pending; this
paragraph approves the proposed sequencing, not an executed fix.

## Executed forwarding mechanism (runtime owner)

The coordinator authorized a minimal debug GDB run, separate from its semantic
suite and statistics build. The runtime implementation owner performed this
mechanism check; it is not an independent review or a throughput measurement.
Evidence under `BASE/pyjit-link-wave1/generator-delegation52`:
`mechanism52.py`, `mechanism52.gdb`, `mechanism52-debug.log` and
`mechanism52-identities.txt`. An initial local-variable inspection is retained
as `mechanism52-inspect.{gdb,log}`; `ir_yielded` was optimized out, while the
original canonical `temp` alias remained available. No source was edited.

The debug binary SHA256 is
`d473cce578267546f340426d4320c2b548af8b133ec677f1b42c42cd6bb66094`;
actual generated interpreter SHA256 is
`ea400d8e9308cbb71e4e456368880a646862698959db9531d466002dbdfb3ece`.
The script uses a plain leaf yielding a fresh mortal `object()` through four
actual delegating parents, calls native `next(top)`, checks outward state, then
closes normally. No monitoring, profiling or Python frame inspection is enabled
during the forwarding interval.

A conditional breakpoint at the emitted successful `ir_suspend_site != NULL`
arm hit twice in the same `_TAIL_CALL_SUSPEND` invocation, at two different
parent frame addresses. Both hits showed actual generator ownership, current
frame equality, recipient slot 3 still NULL, actual DELEGATE pc5 and actual
SUSPEND pc6. The held canonical object's address was identical, with refcount 1,
at both hits. This directly demonstrates the owner remains held in C rather
than occupying either intermediate transfer home.

At the first hit that parent was EXECUTING (state4) and owned current exception
state. At the second hit, the first parent had become SUSPENDED_YIELD_FROM
(state2), saved pc7 (RESUME_DELEGATE), previous=NULL and
`gi_exc_state.previous_item=NULL`; the recursion allowance had increased by one.
The second parent was still EXECUTING and owned the current exception link,
ready for the next iteration. Native disassembly at that arm showed direct
suspension/link/recursion stores inside the same function. After disabling the
breakpoint, all checks completed, the script printed `mechanism52 complete`,
and GDB reported normal process exit.

This closes the open question of whether the compiled bounded route actually
executes across multiple parents and preserves the intended owner/home phase.
It does not establish general callback correctness, broad qualification, elapsed
improvement or acceptable failed-check overhead; those remain separate gates.
