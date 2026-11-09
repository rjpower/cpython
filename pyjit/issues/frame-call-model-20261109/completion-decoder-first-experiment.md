# First completion experiment: decode the proved immutable site

Concrete proposal for root/B/H admission; no runtime edit or native execution.
This isolates decoder work from completion-commit specialization, continuation
layout and heap-frame creation. The shared `_PyEval_CompleteStep` implementation
and every owner/observer/error branch remain unchanged in this first experiment.

## Source shape

Only the VM-parent part of `_PyEval_CompleteGeneratorReturn` changes:

```c
_PyInstr *site = frame->instr_ptr;
PyCodeObject *code = _PyFrame_GetCode(frame);
int offset = (int)_PyFrame_InstrOffset(frame, site);
_PyIRWord word = code->co_words[offset];
int opcode = _PyIR_HandlerBase[IR_HANDLER(word)];
assert(opcode == SEND || opcode == FOR_ITER);
return _PyEval_CompleteStep(tstate, frame, site, IR_B(word), IR_A(word),
    frame->return_pc + IR_DS(word), value,
    opcode == SEND ? _PY_IR_SEND : _PY_IR_FOR_ITER);
```

The existing interpreter-entry branch stays unchanged. This removes the live
word load plus BaseOpcode/InstrOparg calls; it neither removes publication nor
changes the eight-argument Step ABI. A debug assertion that return_pc equals
site+1 may pin the existing physical continuation invariant if the full caller
census supports it. It is not a new runtime eligibility branch.

## Positive source and generated proof

1. `return_to_caller()` invokes this routine only after a generator-owned child
   returns. Native entry returns to an interpreter-owned parent, handled before
   decoding. VM generator entry is the common `enter_generator()` primitive
   reached by SEND/FOR providers; it saves the actual site's immediate next PC.
   The result remains canonical and C-owned across child retirement as before.
2. Frame ir_words/instr_ptr use the executing copy's coordinate system. The
   matching immutable code word supplies ownership operands even when the live
   site has become a transparent LINE/INSTRUCTION wrapper or specialization.
   Never classify the wrapper's handler as the semantic owner.
3. Generated `_PyIR_HandlerBase` already maps specialized and instrumented
   families to their semantic base. Host inspection finds eleven rows whose
   base is SEND/FOR_ITER; every row has A=DST, B=BASE, C=FB, D=IMM,
   imm=LOC_D and FORWARD. This includes SEND_GEN/FOR_ITER_GEN and
   INSTRUMENTED_FOR_ITER. No handler-name whitelist selects the optimization.
4. The existing mandatory `validate_completion_words()` uses exactly A/B and
   signed IR_DS for SEND/FOR geometry and target validation. It bounds the owner
   window, destination and `i+1+IR_DS(word)`. `_PyCode_InstrOparg` returns that
   same signed D value for these FORWARD/LOC_D rows; its generic feedback-arg
   branch is irrelevant to this positively declared family.
5. `return_pc+IR_DS(word)` keeps the **immediate physical** completion target.
   A far-relaxation IR_JUMP remains a real continuation and executes normally;
   do not normalize it to its eventual LINE landing. Caller source/EH site,
   STOP/branch observation and newer redirects remain the existing Step's job.

The helper does not become a general word decoder. Normal/exceptional frame
retirement, PEP523 routes, recursion and CREATED/non-None resume checks do not
change. Mutable code replacement creates a distinct code object; the executing
frame pins its selected code/exec allocation under the existing ABI.

## Minimal verification and isolation

Add one format-contract test that selects rows by generated semantic base and
asserts the complete A/B/C/D, FORWARD/LOC_D geometry above. Check specialized and
instrumented membership through metadata, not an enumerated optimization list.
Existing constructor malformed-window/target tests remain unchanged. Existing
native completion tests exercise FOR/SEND, local/temp/NoDst destinations,
monitoring payloads/errors, recursion, last-owner retirement and far relaxation.
Root runs the cheapest relevant selectors in both builds, then the agreed core
gate. No metadata/magic change or regeneration should be required by this source
change alone.

Inspect the compiled helper to confirm both decoder calls disappear and the
actual operand/continuation paths remain. Measure the isolated change with
frozen identities and the same paired generator/coroutine counts and elapsed
protocol; retain normal broad controls. Do not combine it with Step inlining,
new header fields or direct heap creation. A later shared commit specialization
would be separately reviewed and measured, retaining one publication/retirement
body with compile-time kind/canonical-result facts and a cold observer adapter.

## Source checkpoint

The bounded implementation changes `Python/ceval.c` only in the VM-parent
completion decoder. The interpreter-parent path and `CompleteStep` remain
unchanged. Immutable A/B/signed-D operands and the generated semantic base replace
the live-word load and two generic decoder calls. The physical continuation is
still `return_pc + IR_DS(word)`; no new return-PC equality requirement is imposed.

The focused selectors are
`test_ir_contract.ContractTests.test_generator_completion_operand_layout` and
`test_ir_semantic_completion.SemanticCompletionTests.test_monitored_generator_for_completion`.
The first checks every declared SEND/FOR family row. The second checks cold and
warmed generator completion through an instrumented FOR site, including exact
STOP payload identity and preservation of the loop local.

Both test files pass host AST parsing, and the three-file diff passes
`git diff --check`. H's actual-source peer accepted the substitution after the
unnecessary return-PC equality assertion was removed. Source is frozen for the
coordinator's release/debug build and focused native checks; these checks do not
yet establish compiled behavior or a performance benefit.
