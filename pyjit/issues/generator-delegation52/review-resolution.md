# Architectural review resolution

2026-11-12. The round 52 candidate responds to the eight findings from the
single [Astra review](astra-review.md). This table records their architectural
resolution and obligations; finding 7 remains partially met. The completed
[report](report.md) rejects both candidates on performance.

| Finding | Resolution in [design](../../design/generator-delegation.md) | Implementation obligation |
| --- | --- | --- |
| 1. Pure complete admission, blocking | Owners and transitions: `can_propagate_delegate_yield` is nonescaping and pure; actual generator owner/continuation precede epoch, all-observers-off, redirect and NULL B+2 checks. No free-threaded fusion. | Recheck after hooks/per parent. On failure publish through existing recipient transaction; cold landing synchronizes stale instrumentation only with real homes/PC and an explicit failure recipe. Never dispatch a stale plain SUSPEND blindly. |
| 2. Actual generator ownership, blocking | Construction checks flags/entry shape; DELEGATE activation and dispatched SUSPEND separately require actual `FRAME_OWNED_BY_GENERATOR`. | Raise SystemError before owner moves or generator conversion on malformed thread/interpreter ownership. Exercise flags, RETURN_GENERATOR and exceptional-entry mutations without assertion/crash. |
| 3. Terminal owner and hooks, blocking | Shared interfaces: IR_SUSPEND receives one already-detached canonical owner; dispatched observation occurs with B populated, then B is taken/NULLed; fused B+2 is never taken again. Explicit resume assignment. | Every frame retains state/link/recursion transaction; exactly one terminal publication/discard/error cleanup. No fallible observer with an absent expected owner. LLTrace and DTrace are escaping boundaries, retain ordering/coherent PC and reject/recheck fusion. |
| 4. Delayed resume target, blocking | Real words: retain actual RESUME_DELEGATE and ordinary next cursor until every fallible resume operation succeeds. Select CD only with unchanged PC/generation. | Redirect intent, including same-PC, wins. Extend skipped and dispatched paths with epoch/TLBC/Emscripten and native/exact/throw/close consumers together. Distinguish resume callback error from injected SUSPEND error. |
| 5. Far physical/semantic targets | Real words: s=DELEGATE, s+1=SUSPEND, s+2=RESUME_DELEGATE, s+3=IR_JUMP, D=2. Dedicated exact-shape normalizer; no instr_length grouping or arbitrary chasing. | Physical completion dispatches s+3 and keeps its INSTRUCTION event; branch/source report normalized CD done. Adapter outside synthetic SUSPEND region with committed completion EH/source phase. |
| 6. Generic providers | Owners: generic/virtual/asend execute through generic DELEGATE provider and materialized SUSPEND route; exact-generator specialization only initially. Existing completion, recipient, PEP523 and throw/close contracts remain. | Lost SEND_VIRTUAL/SEND_ASYNC_GEN specialization at converted sites is an explicit performance cost. No provider replay or thread-return-kind persistent delegation state. |
| 7. Actual emitted fusion proof | The intended bounded final state includes owner/link/PC state; the logical allocation row is never physical POST. | The DSL producer and C predicate are mechanically bound with mutation negatives. The terminal emitter is trusted compiler code, supported by selective output checks and source/native review, not a general AST/token mutation gate. Astra’s stronger emitter-binding obligation remains partial; see [cold review](cold-review.md). Preserve frontend stack/END_SEND/CLEANUP_THROW effects through allocation. |
| 8. Mechanism and overhead evidence | Gates: deep exact chain establishes iterative transfer; shallow/plain/FOR_ITER, generator-expression/comprehension and native cases expose failed-check costs. Coroutine fibonacci often returns without yielding, so no assumed gain. | Inspect emitted native loop and removed traffic; run unchanged paired target/broad gates. Preserve losing evidence and do not promote on architectural claims alone. |

## Frozen interfaces

- Eight-byte words: DELEGATE A=completion recipient, B=three-home receiver
  window, C=existing scaled unary feedback, D=signed forward completion distance;
  SUSPEND A=incoming recipient, B=outgoing home, C=yield kind, D=0;
  RESUME_DELEGATE A=existing resume flags, B=0, CD=absolute DELEGATE offset.
- Delegated triple s/s+1/s+2: SUSPEND A=B=DELEGATE.B+2, delegated C;
  RESUME_DELEGATE.CD=s. Keep separate send and synthetic suspension EH contexts.
  Far layout and physical/semantic target distinction are fixed above.
- Pure `can_propagate_delegate_yield(tstate, frame, continuation)` returns an
  actual SUSPEND pointer or NULL. Runtime owns its static implementation and
  exact version/image/observer authority. It cannot synchronize or move owners.
- Generated terminal `IR_SUSPEND(value, site, recipient, resume, kind)` consumes
  one already-detached canonical C owner and explicitly assigns the resume PC.
  The DSL producer and C predicate have mechanical checks; the backend emitter
  remains trusted and requires peer and native qualification for edits.
- Add literal `_PY_IR_DELEGATE` to `enum _PyIRStepKind` and existing entry and
  completion consumers. No per-frame record, chain cache or outcome allocation.
- Compiler-owned handwritten header helper:
  `int _PyIR_DelegateDoneTarget(const _PyIRWord *words, int count, int site,
  int physical)`. Bounds-check reads and normalize only exact far triple/adapter
  shape; otherwise return physical. Constructor separately validates the triple
  and ingress. Completion/source/branch consumers share this helper.
- Logical `_PyIR_Outcome`/completion-window rules mirror SEND and SUSPEND only
  for cold allocation/owner analysis. Bounded fusion owns its distinct physical
  final-state contract and cannot expose the logical intermediate row
  to observers. Post-emission contract verification remains future work.

## Candidate source

Golden51 remains selected. Initial candidate `58f9ea6cfee` lost six paired
comparisons by 4.37% for generators, 4.66% for coroutines and 2.67% for
DeltaBlue. Followup `e851719` passes native wrapper-first placement inspection,
but six-pair confirmation loses by 0.95%, 11.30% and 11.85% respectively.
Both candidates are rejected. See the [cold review](cold-review.md),
[report](report.md) and [next plan](next-plan.md).
