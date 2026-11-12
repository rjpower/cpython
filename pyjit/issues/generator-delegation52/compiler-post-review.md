# Independent compiler source review

2026-11-12, source review of uncommitted R52 compiler slice against selected
round51 `0e5e61f4c6b`. This is not native compile or executed qualification.
Coordinator owns regeneration, compilation and candidate execution. Runtime C
and emitter sources were frozen during this review; only this report changed.

## Finding

No blocking compiler-source issue found in the inspected slice. Approval is
conditional on real release/debug compilation, constructor admission and
near/far/source/exception tests; those remain pending with the coordinator.

The compiler makes the semantic conversion after register allocation and
existing completion lowering, preserving frontend owner effects until their
physical homes are known. `lower_delegations` checks SEND/YIELD/RESUME/backedge
shape, exact receiver-window/incoming-recipient relation, delegated resume bits,
separate suspension handler and shared CLEANUP_THROW completion, and rejects
normal or exceptional ingress to its middle operations. Matching does not use
source-coordinate equality. Plain yields convert separately. Unconverted SEND
remains available for async iteration.

The final operation records preserve a direct completion destination from the
frontend END_SEND, while RESUME_DELEGATE retains flags in A and its independent
absolute DELEGATE target in CD. The removed backedge is marked wordless only
after its target has been retained. Source assembly emits the new resume's taken
edge and suppresses its nonexistent fallthrough. New semantic operations use
IR-format validation; remaining IS_PSEUDO_INSTR/IS_NO_FALLTHROUGH_OPCODE checks
are equality predicates and do not index legacy tables outside their domain.

Far layout keeps DELEGATE/SUSPEND/RESUME_DELEGATE adjacent. A two-word resume
allocation places the real IR_JUMP at s+3, with DELEGATE D=2 targeting it from
s+1. Its normalized target is exposed only by an exact bounded four-word
recognizer; ordinary execution still dispatches the adapter and can observe its
INSTRUCTION event. The adapter inherits resume's outer exception context,
separate from SUSPEND's synthetic throw context. This matches the source/runtime
continuation contract; native tests must still verify completion-destination
aliasing/discard and callback failure there.

Semantic outcomes retain the logical B+2 yield definition followed by consuming
SUSPEND for cold allocation/recovery analysis. The runtime's proved unobservable
fusion is responsible for equivalent physical stopped state; this compiler
review does not grant physical POST observation of a nonexistent B+2 owner.
Constructor and observer consumers remain a separate review boundary.

## Evidence reviewed

Inspected changes to `Python/assemble.c`, `pycore_regcfg.h`, `pycore_ir.h`,
`pycore_ir_outcomes.h` and compiler-owned `test_ir.py` fixtures. Checked macro
implementations, physical encode/layout ordering, source-edge assembly and
exception-table range assembly against selected runtime. Tests add exact
near/far triple structure and provider-error versus injected-throw behavior.
Runtime host-only interpreter/metadata generation succeeds with the shared
formats, but does not execute the C compiler or establish constructor admission.

During reciprocal runtime review, the compiler worker found an independent
LLTrace legacy opcode-array access for new IR-only IDs. Root authorized the
small `ceval.h` fix; names/argument display now use IR metadata for that domain.
Consumer will add a debug subprocess case. This was outside the compiler slice
and does not change the compiler judgment above.
