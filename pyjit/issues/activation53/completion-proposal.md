# B: one canonical completion terminal

Reviewed proposal, later implemented, qualified and rejected on performance;
see the [final result](completion-result.md). Retired runtime lane:
`BASE/worktrees/activation53-completion`, based on immutable golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. BASE and native roles follow HANDOFF.
This implements Astra's bounded B, not a continuation descriptor or resume cache.

## Interface and source boundaries

`IREmitter.return_to_caller` retains its existing ordinary return else branch.
Only its generator-return arm changes from a normal C adapter call/status
return to a musttail transfer into one handwritten evaluator terminal in
`Python/ceval.c`, immediately after `ir_targets.h` and before generated handlers:

```
static PyValue Py_PRESERVE_NONE_CC
_TAIL_CALL_complete_generator_return(TAIL_CALL_PARAMS);
```

The existing `ir_word` parameter transports the canonical result's raw bits on
this single internal edge, instead of an instruction word. The terminal adopts
those bits as its one PyValue owner; it does not publish a frame slot or allocate
metadata. It restores a real current parent instruction word before any error
transfer; successful DISPATCH loads its usual next word. The caller's other
arguments identify the already-current parent and its homes after child cleanup.
No live ownership crosses a non-tail call into a preserve-none function.
Py_STATS keeps its existing lastopcode parameter/accounting.

The source terminal is instantiated once, shared by RETURN_VALUE and
INSTRUMENTED_RETURN_VALUE. It uses the ordinary evaluator dispatch/error labels.
No handler ID, opcode or per-frame field is added. This explicit transport must
be reviewed as an ownership edge; it is not an arbitrary opcode-as-value cast.

In `Python/ceval.c`, extend the existing always-inline `complete_step_impl`
with a compile-time `observe` argument. Existing `_PyEval_CompleteStep` and
`_PyEval_CompleteGeneratorReturn` pass true. The terminal's admitted path passes
canonical_input=true and observe=false. Guard the existing PRE/POST observation
blocks with observe; keep the same commit/retirement source and ordering.
Provide its inline declaration before the terminal, as the existing known
binder already does for its later definition. The optimizer must eliminate
the internal result-struct/status scaffolding on the unobserved instantiation;
native inspection decides this, not the inline spelling.

## Admission, ownership and control

Entry occurs after `_PyEval_FrameClearAndPop` and reload of the actual parent.
The returned value is the only C-held incoming owner. Child finalizers have
already run, so admission reads current parent state now.

1. Interpreter-owned parent retains direct `_PyFrame_SetReturnValue` and its
   actual return_pc, including pending redirect/discard/displaced-owner handling.
2. A normal parent is eligible only when its code has no monitoring metadata
   and no pending redirect. Otherwise pass
   the untouched owner to existing `_PyEval_CompleteGeneratorReturn`, preserve
   its result/error status protocol, and jump to the existing error label on
   failure. No window effects precede this cold handoff. Do not add an epoch
   guard: the existing completion helper observes only code->_co_monitoring,
   and no callback occurs between this current-state test and semantic commit.
   Thus an absent metadata pointer folds exactly the currently false PRE/POST
   tests, including existing stale-instrumentation behavior; this is not a
   borrowed pre-child-cleanup eligibility fact.
3. Decode the immutable caller word at its actual saved site into SEND/FOR,
   B/A/signed-D, using the existing adapter's formula including far adapters.
   This decode is retained work. Call the shared always-inline commit body with
   canonical_input=true, observe=false and the appropriate SEND/FOR kind.
4. The shared body detaches receiver/cursor and SEND input, publishes a SEND
   result or retains the value for discard/FOR exhaustion, sets real return_pc
   and completed instr_ptr, then closes exception/sent/cursor/receiver/displaced/
   remaining-value owners in precisely the current order. FOR leaves the last
   loop-item binding intact. Overlapping SEND destinations remain supported.
5. Reload actual frame->instr_ptr after retirement; callback redirection is
   authoritative even at the same PC. A finalizer enabling hooks after commit
   affects later execution, not retroactive PRE/POST events. LLTrace and dispatch
   retain their established post-completion sequence.

No new failure is possible in the admitted semantic commit. Existing PRE/POST
failure and redirect recipes stay in the cold adapter unchanged. The observe
argument folds out observation code only after current-state admission, not
from a stale fact captured before child retirement. No completion state is
stored at resume. Ordinary-call setup and ordinary return publication stay intact.

## Expected source/native evidence and tests

Handwritten files: `Python/ceval.c`, `Tools/cases_generator/ir_generator.py`, and
bounded producer tests if needed. Root owns regeneration, generated products,
release/debug builds, native inspection and runtime tests.

Expected native: generator arm tail-jumps to one preserve-none terminal; on its
unobserved path there is no call to `_PyEval_CompleteGeneratorReturn`, no normal
C adapter prologue/epilogue or struct-status boundary back to RETURN_VALUE.
The body still performs immutable-word decode, canonical stores and required
owner retirement. The cold call/status branch must remain behind admission.
Ordinary RETURN must retain its direct recipient branch and no new admission
checks. Count terminal size, return-variant size, spill burden and new guards.
If the compiler reproduces the old boundary or guard/spill cost cancels its
removal, report a failed mechanism; do not claim success for moving code inline.

Focused semantics: nested SEND completion and FOR exhaustion; local/discard and
overlap recipients; far adapters; mortal receiver/displaced owner destruction;
child cleanup installing monitoring or requesting a redirect; PRE and POST hook
exceptions and same-PC redirects; escaped frames; coroutine/generator exception
and close/throw regressions. Negative producer checks should ensure ordinary
return stays direct and both generator variants use the same terminal, and that
changed canonical-transfer proof retains its existing Publish fallback.

No performance result is claimed. Separate B timing must precede composition;
the extra pending-redirect guard may outweigh a removed call boundary.

## Peer resolution

A independently accepted the corrected proposal before implementation. Root
accepted its source/ABI scope. The proposed epoch guard was removed: completion
already consults current code monitoring metadata, and admission-to-commit has
no callback. Both return variants retain the canonical provenance proof (or its
existing Publish fallback) before transferring result bits. The terminal adopts
that owner once and restores a real instruction word before observer/error
edges, including a fresh site word after a cold failure. Its source is explicitly
no-inline so the optimizer cannot copy the full terminal into every return arm.
No runtime observation state, handler number or resume store was added.
