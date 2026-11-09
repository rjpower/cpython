# Cold integrated source review

2026-11-08. Independent interpreter/IR review of runtime worktree
`build/worktrees/execution-boundaries-20261108`, HEAD `f2c5ac3052b`, including
the uncommitted combined boundary/discard changes. Initial combined diff SHA-256:
`ea3168a315b628991b9992732fecfa4752eb7ca62a65e5f43ca641e2dbae163a`.
References below are runtime source paths and line numbers at review time.
The source is being repaired concurrently; this review is not a receipt for a
later tree.

Read the operand commit against its parent and the combined diff against HEAD,
then the generated handlers and their producers/consumers. Read the CPython AI
policy and canonical project/lifetime guidance. No runtime source edits, native
jobs, builds or tests were performed by this reviewer. `git diff --check HEAD`
passed for the inspected source. The coordinator owns native evidence.

## Disposition

The static implementation is a reasonable bounded experiment. I found no
confirmed ownership or control-flow defect in the currently inspected paths.
This is **not admission**: the coordinator's retained
`boundary-combined-failure-v1-release-focus.log` reports failing disposal,
generator-selection and contract tests. In particular, the exact variant was
selected while the result finalizer saw POP's PC, and a generator callee acquired
an excluded variant. These failures need an explanation and repaired native
evidence; source inspection cannot turn them into passes.

The current generated exact handler stores the tagged recipient at
`Python/generated_ir_cases.c.h:7436`; `Python/ceval_macros.h:763` routes returns
through `_PyFrame_CompleteReturn`. The current selector excludes suspended
callee code at `Python/specialize.c:689`. Those facts do not explain the native
failures by themselves. The coordinator subsequently reports that isolated
native probing with retained `discard-probe-v1.py` selects exact disposal at
CALL after 20 iterations; switching its shared caller code to a different
callee leaves the old exact handler at 20 iterations, then selects general
disposal at CALL after 100. Independently, the source confirms that
`make_caller()` creates new functions sharing one nested code object, while the
fixtures assume independent feedback and only warm for threshold plus ten.
Guard decline and cooldown therefore explain the reported distinction without
requiring a runtime ownership change. Review the fresh-code fixture repair and
its new native receipt before closing these failures. Do not weaken the
direct-disposal assertion merely
because finalizer timing is implementation-specific: it also establishes that
the optimization actually executed.

No throughput benefit has been established. The new completion check affects
ordinary returns too; measurements must include that cost, code size and broad
workloads, not just dispatch removed at selected sites.

## Architecture and correctness

The operand variant preserves canonical words and existing family metadata.
Selection proves the receiver is a local and the destination a temporary
(`Python/specialize.c:705`); the generated handler keeps the local owner intact,
duplicates the attribute owner and writes the temporary directly
(`Python/generated_ir_cases.c.h:13432`). Guard declines precede consumption.
This is a useful specialization of the existing ownership protocol, without a
second value representation. LOAD_ATTR is not in the DISCARDING family list
(`Tools/cases_generator/ir_generator.py:246`), so the temporary test does not
accidentally admit the call-specific `IR_NO_DST` sentinel.

The boolean variants keep the logical NOP and change selected incoming edges.
Both canonical and live successors must save IP
(`Python/specialize.c:618–631`). The generated guard executes before either
continuation advance (`Python/generated_ir_cases.c.h:15366`). This is crucial:
a failed guard must replay the original branch without a pre-applied skip.
The branch's JUMPTO uses an absolute target, so advancing fallthrough first does
not bias the taken edge (`Python/ceval_macros.h:240`). Instrumentation restores
dependent predecessor variants before patching anchors
(`Python/instrumentation.c:1963`). Debug disables shared boolean variants because
lltrace belongs to a frame; free-threaded selection is also disabled. These are
coherent limitations for the selected GIL/release experiment.

The call tag is completion state, not a register index. The ordinary setter
rejects it (`Include/internal/pycore_interpframe.h:188`); completion removes it
before release or publication (`:213`). Selection requires the same temporary
and source line in CALL/POP and excludes suspended callers/callees
(`Python/specialize.c:659`). General-call transaction destinations remain numeric
through preparation; tagging happens only on Python child commit
(`Python/ceval.c:3055`, `:3142`, `:3244`). Native fallback therefore retains the
existing materialized destination. Exact entry writes the tag only after its
guards and frame construction. Generator-specific return-slot consumers remain
outside the intended scope.

Child retirement precedes result completion. A live instrumented POP or lltrace
causes materialization into the original home instead of skipping it
(`Include/internal/pycore_interpframe.h:214–221`). Clearing the pending bit on a
successful debugger line change preserves the debugger-selected continuation
(`Objects/frameobject.c:2248`). Local finalizer re-entry uses a separate
interpreter entry frame (`Python/ceval.c:2188`), rather than intentionally
overwriting the caller's pending recipient.

Python does not mandate the old decref position or prompt finalization. Direct
disposal at CALL is a legitimate chosen policy if state is valid on re-entry,
supported observation behavior is defined, and ownership/resource quality hold.
The tests' exact finalizer PC and release order check this implementation's
chosen protocol; they are not language requirements. No frame/region delayed
cleanup design is implemented by this packet, and this packet supplies no
evidence against such designs.

## Follow-up recommendations

1. **Define observation activated by the result's own finalizer.** Completion
   checks the live POP before `PyValue_XCLOSE(value)`, then returns `next + 1`
   unconditionally (`Include/internal/pycore_interpframe.h:214–225`). A result
   finalizer can enable instruction monitoring after that check. Existing tests
   enable it during child cleanup, before the check
   (`Lib/test/test_ir_discard_completion.py:78`). If direct disposal is one CALL
   transition, beginning observation after the dormant POP may be the intended
   contract; state that explicitly and test the next observed instruction. Also
   cover a supported debugger line change from re-entry during result disposal.
   This is a contract/test gap, not a demonstrated language violation. The
   reviewed follow-up below supplies the trace and monitoring case.

2. **Exercise no-save-IP cleanup with a real finalizer.** The runtime rejection
   fixture covers IR_JUMP (`Lib/test/test_ir_boundaries.py:79`); metadata tests
   also cover END_FOR and INSTRUMENTED_END_FOR. Add a focused native case in
   which an END_FOR successor retires an object and observes the caller PC.
   This checks the reason for the restriction, including re-entry, instead of
   only checking the handler flag. Keep the fixture semantically valid and
   require the intended specialized branch to be selected.

3. **Keep the variant mechanism narrow.** ExecutionVariant now combines operand
   modes, branch edges and call completion (`ir_generator.py:148`). This remains
   manageable for six aliases; avoid turning it into a general optimizer.
   Exact token-body checks (`:3928`, `:3936`) fail closed on source changes but
   couple safety to spelling. Before extending the system, express the checked
   transition as a small named contract and validate relevant uop order and
   re-entry properties. In particular, a future re-entrant guard before the
   boolean branch must not silently preserve a static edge proof invalidated
   by instrumentation during that guard. No such guard exists today.

4. **Centralize canonical execution identity when more aliases arrive.**
   `fb_put_row` and `fb_record` separately normalize all three variant groups
   (`Python/specialize.c:419`, `:551`). Both currently agree. A generated shadow
   helper for execution variants would remove a place for the lists to drift;
   do not blindly normalize unrelated structural/fused IR handlers. This is a
   small maintenance improvement, not an integration blocker or a reason for
   broad dispatch refactoring.

Retain the first failed native receipt. Re-review the repaired final diff and
native results before commit, then decide admission using matched measurements
against immutable golden.

## Independent review of fixture repairs

Read the worker's three-file repair in `boundary-ir-20261108` after the initial
review. Source disposition: approve, pending the coordinator's native rerun.

- `test_ir_discard_completion.make_caller` constructs a function from
  `caller.__code__.replace()`, giving each fixture fresh executable feedback.
  This fixes the demonstrated shared-code cooldown contamination instead of
  increasing arbitrary warmup counts or relaxing route/PC assertions. Varargs
  select the general entry path through the existing function-kind policy.
- `test_ir_boundaries.edge_function` resolves names through the union of public,
  specialized and IR opmaps before numeric encoding. Thus existing canonical
  inline load words remain intact; the fixture no longer asks a helper that
  only recognizes ordinary/IR names to resolve specialized names.
- `test_ir_contract` uses an alias's `row.shadow` where the source oracle has
  that canonical specialization. Exact discard then has exact call's zero
  external writes instead of generic CALL's single write. The general alias
  retains the existing self-publication assertion, and the expected canonical
  mismatch set is unchanged. This updates affected consumers rather than
  exempting new aliases from contract checks.
- The added result-finalizer case covers both trace and local instruction
  monitoring. It requires the exact discard variant, requires the finalizer's
  caller PC to be CALL, and requires observations to begin after the dormant
  POP. It makes the selected transition policy explicit without asserting a
  Python language finalization guarantee.

No additional native work was performed by this reviewer. The original native
failure remains retained evidence; acceptance requires the repaired receipt.
