# Cold R4 source review

2026-11-07. **Request changes: two bounded checker gaps.** Reviewed the actual
nine-file runtime diff on parent `225480a4250fa8b9760f355875aac30ec76c40c3`
before reading the proposal or prior review. R4 was uncommitted when inspected;
this report does not accept a later commit or repair. Runtime source was
read-only. CPython's AI policy and applicable agent instructions were read.

The current six producers follow the claimed publication order. These findings
concern the new checker's rejection boundary, which is the main new behavior
of this packet. Neither finding establishes a failure in the unchanged current
interpreter bodies.

## Findings

1. **P2: generic completion accepts an already consumed receiver or result.**
   `result_publication_contract()` validates the receiver's one `TAKE()`
   initializer and the final output assignment, but checks no consumption of
   the helper inputs before completion. With the candidate release interpreter,
   both `handler_contracts()` and `generate_ir()` accept this fixture and emit
   the positive publication phase:

   ```c
   inst(LOAD_ATTR, (owner -- attr)) {
       PyValue receiver = TAKE(owner);
       PyValue pending = PyValue_DUP(receiver);
       PyValue pending_self = PyValue_NULL;
       int method = 0;
       PyValue_CLOSE(receiver);
       IR_COMPLETE_ATTR_READER(pending, pending_self, receiver, method);
       attr = pending;
   }
   ```

   The emitted close precedes publication; `value_attribute_complete()` then
   receives the stale owner and closes it again. Replacing that close with
   `PyValue_CLOSE(pending)` is also accepted and passes a consumed result to
   the helper. Reject known direct consumption of `pending`, `pending_self`
   and `receiver` on paths that continue to completion. Preserve the actual
   terminal error-cleanup branch ending in `IR_ABORT_VALUE_READER()`. A narrow
   check of known release/take operations and direct mutation is sufficient;
   arbitrary C ownership inference is outside this packet.

2. **P2: explicit publication skips result use within its assignment statement.**
   `_sole_top_level_assignment()` counts `=` tokens, while the result-use
   scan starts with the following statement. Both contract construction and
   emission accept:

   ```c
   attr = PyValue_DUP(PyValue_None), PyValue_CLOSE(attr);
   PyValue receiver = OWNER(owner);
   IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
   ```

   The constant makes this particular close harmless, but the source shape
   permits the same consumption of a mortal result from another C-owned value.
   The generated code publishes `attr` after its close. Require the assignment
   statement's result occurrence to be only its left-hand side; add a negative
   fixture using a source that does not incidentally trigger the separate S7
   input-view check.

## What the source and evidence establish

The shared declaration record replaces the five-name emitter branch. The
explicit marker still clears inputs and flushes the temporary result before
receiver retirement. A folded-local result stays C-owned until the final
`IR_STORE_DST`, and failure passes that C-owned result to cleanup. The generic
helpers publish the first temporary result before closing the receiver and
refreshing. Existing effect bits remain whole-handler summaries.

A receiver callback can change public locals. The handler delays its own final
result store until retirement succeeds; it must preserve callback mutations
on failure. `IR_STORE_DST` reads and retires the value actually in the local
after successful callback return. Neither helper restores a stale old value.

Independent normalization of `pycore_ir_metadata.h`, removing only the new
`result_phase` initializer, reproduces the parent file exactly. Generated
interpreter bodies have no diff from the parent. The native `_opcode` table
contains six positive phase rows with `dst=-1`; other metadata fields and
effects are unchanged.

I inspected `r4-checks.json` and the actual release/debug test logs: regeneration
and both builds returned zero, both builds checked 117 modules, and all eight
focused files passed with 1,100 tests and one skip per profile. The release
value-API lint log ends with 306 units, zero failing findings and zero parse
errors. These are focused checks, not full-suite qualification.

The build logs contain warnings in unchanged generated cases, `ceval_macros.h`
and unrelated sources, including musttail-address and unused-variable warnings.
Earlier incremental logs are not a comparable warning baseline for this shared
header rebuild; warning counts do not establish a regression or improvement.

No performance conclusion is supported here. The basic R4 screen and final
commit identity were pending at this review boundary. A repaired candidate needs
a separate review record, matching regeneration and native evidence, and an
identified commit before acceptance.
