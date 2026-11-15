# IR59 source peer

Pre-implementation direction **GO with VACATE_SOURCE**, as agreed with the coordinator. The actual [proposal](../../design/proposals/register-semantics59.md) and [Astra ledger](review-astra.md) were read after their first-lane freeze; they incorporate the source/admission corrections below. This reviews selected57 source `0dcba75bb3df40c32137db453b001f7e8f101de0` in `BASE/worktrees/call55-integrated`, not canonical C's stale runtime files. No runtime edit, native job, generation or test was performed. Actual implementation still needs post-review.

## What is already selected

FOR_ITER already has edge semantics: `_PyIR_Outcome` maps BODY to A and exhaustion to retirement of B..B+1 (`Include/internal/pycore_ir_outcomes.h:338`). The generator classifies completion-bearing window handlers as `_PyIR_OUTPUT_OUTCOME` (`ir_generator.py:8174`). Do not describe IR59 as discovering these outcomes or replacing WINDOW_DST_FIRST for FOR_ITER.

Existing exact calls overlap argument homes; ordinary child return is direct; native call transactions already publish temporary recipients directly and hold folded-local results until input cleanup and periodic checks finish (`ceval.c:1693–1867`). Generator return already uses canonical input and immutable SEND/FOR decoding (`ceval.c:3492`). The initialized frontier, physical NULL arena contract and closed zero-eligible COPY-web census remain. There is no new PyValue rollout or permission for borrowed frame cells.

The residual stack-shaped contract is earlier in lowering. Window `SlotLayout` has no explicit result binding; `IRStack.address` uses `window[p-base]`, and `IRStack.store` always publishes window outputs there (`ir_generator.py:2387,2889,3002`). `emit_final_outputs` subsequently moves the selected result to A (`:6485`). Selected FOR_ITER_LIST consequently publishes its updated cursor, publishes `next` into B+2, then moves/clears B+2 for local A (`generated_ir_cases.c.h:14833`). RegCFG already selects that local recipient, but preserves the canonical jump-output identity and slot reservation (`regcfg_passes.c:393`; `regcfg.c:1080`). The first increment should remove the redundant staging, not change allocation or invent new semantic outputs.

## Implementable first binding

Use one generator-side declarative final-output binding, for example `(BODY, output_index=2, destination=ROLE_DST, phase=FINAL_COMMIT, source_policy=VACATE_SOURCE)`. Derive its value identity from the actual external output and carry the declaration through SlotLayout, final Storage publication and emitted receipts. Existing outcome, ownership and observer consumers must agree with this binding; no separate helper-name or opcode-name bypass should route memory independently.

Initially enable the direct route only for an admitted local recipient in FOR_ITER_LIST. Preserve the getter, owner acquisition, type/tag/bounds guards, normalization, cursor update and exhaustion path. At the ordinary final BODY commit:

1. Publish the updated cursor as before.
2. Write physical NULL to B+2 without interpreting its old bytes.
3. Publish the acquired result directly into local A, using the existing publication and displaced-owner protocol.
4. Finish displaced-owner retirement with the same continuation, error and refresh behavior.

Keep temporary/discard recipients and unconverted or instrumented handlers on their existing staging route unless separately admitted. This first packet adds no borrowing, canonicality, frame-readability or broader normal/error certificate.

**Do not redirect `IRStack.address` globally.** Storage can flush before normal completion, error handling or observation. Only `emit_final_outputs` may install the final binding, and only the matching output store may consume it. Prior result publication, reload, output address escape, alias use or a deferred-observer commit must reject the direct route or use the unchanged staging protocol. Suppress only the corresponding final move; unrelated window writes and input retirement remain.

Actual receipt truth is mandatory. The local publication is an A write; the B+2 vacancy is a distinct physical operation. Do not label the vacancy as an A VACATE or invent a scalar SAVE/token for a window graph. If the existing graph cannot represent this action pair, retain its UNKNOWN state while recording the bounded output-binding receipt. No full-home authority follows from the declaration alone.

## Required correction to KEEP_NULL

KEEP_NULL was not justified by current admission. Compiler compaction leaves the folded jump result unoccupied (`regcfg.c:1080`), but constructor geometry only bounds the full window (`codeobject.c:1940`); the unknown FOR semantic path does not prove incoming B+2 physically NULL (`:2668,3065`). The observer's NULL description is committed post-state, not an incoming proof. Logical NO_HOME is also not physical NULL or readability.

VACATE_SOURCE therefore remains in the first packet. It preserves the existing committed NULL state without needing a new constructor proof. Removing that store later needs actual reproof for constructed images, plus equivalent observer, copy, cleanup and arena guarantees. Do not silently assume compiler provenance for public constructed images or reduce N/frontier fills in this increment.

## Admission and falsifiers

Source checks should generate the actual handler in both record modes and verify direct local publication, cursor-before-result ordering, B+2 vacancy before any displaced retirement, and exactly one result-owner transfer. Retain temporary/discard and instrumented controls. Mutation controls must cover an inserted callback or early flush after acquisition, output alias/address escape, changed output index, missing vacancy and altered completion edge; each must revoke the direct binding or fail generation. An arbitrary declaration or TAKE spelling is not ownership evidence.

Native tests should reach the warmed LIST handler with local A through all supported image routes, cover empty/exhausted loops, inline and mortal items, and a displaced local whose finalizer inspects and mutates `f_locals` and runs GC. It must observe the new binding and vacant B+2 at the completed continuation. Include monitoring/redirect/error fallbacks and an admitted constructed-image control; retain existing scratch/discard behavior. Test semantic ownership and cleanup, not exact upstream destructor timing.

The native acceptance gate is narrower than source line count: inspect actual eliminated staging stores/loads, unchanged getter/normalization/guards and no extra helper hop or harmful spills. A compiler may already forward some source-level loads. Report the surviving physical vacancy store honestly. This single increment is useful only if the complete path wins under the existing unchanged-workload gates; it does not establish a parity budget.

## Architectural limits and next use

The current allocator intentionally uses canonical target names and contiguous scratch windows to make joins and prefix unwind correct (`regcfg.c:522,867,991`). Separating semantic values from homes must not remove those constraints before an equivalent EH and observer reconstruction contract exists. Call-window acquisitions cannot disappear merely by calling a local a virtual alias: overlapping child locals are owners today. A borrowed-argument rival would change lifetime/readers and remains a distinct experiment.

The next reuse to examine after this packet is tuple/list BUILD success: the existing typed steal-on-success contract already identifies the point after inputs become dead and before final recipient publication. A final-output binding could remove result staging there while preserving allocation failure and input cleanup. It needs its own complete source/emitter proof and measured coverage. Generic native CALL is not automatically another staging opportunity: its selected transaction already has direct temporary publication and delayed local commit. Do not replace that delay with an early local write across callbacks or periodic failure.

## Builder integration correction

Root reviewed the complete compiler, geometry, constructor, outcome and emitter
packet. Full generation changes only BUILD_LIST/TUPLE and their format rows.
The initial native bootstrap rejects valid compiler images. Both workers
independently reviewed the diagnostic path and the correction:

- Consume a positive physical SET_NULL receipt instead of dropping it in an
  UNKNOWN semantic row. This grants no general initialization/readability fact.
- A fresh recipient requires logical vacancy/no outstanding owner; unlike an
  in-place publication, it need not inspect the previous bits. The checked
  `IR_STORE_FRESH_DST` short-circuits only the old scratch assertion. Existing
  macros retain their behavior; result normalization, new-value checks, local
  replacement and discard are shared.
- The emitter selects this mode only after the actual successful-steal and
  current-owner final receipt. Failure leaves inputs unchanged. The constructor
  still rejects live owners and unknown logical state. A NO_HOME-only destination
  may be written, while a preceding read remains rejected.

Both actual source peers are GO. The four-method host gate passes after the
associated fixture literals are ported; its first stale-literal failure is
retained. Root's second full generation passes. Candidate `d4584f8e45a` is
committed. Both 117/79 builds and 207 focused methods per role pass; the
standalone performance screen loses and does not earn selection. See the
[report](report.md) for exact evidence. The [builder proposal](edge-results-proposal.md) owns the
complete revised contract.

## Explicit native Invoke source

Root reviewed the actual source packet in `call55-pins` against selected57:
five paths in bytecodes, analyzer/generator and two fixtures. Source GO, pending
full generation, native qualification and post-review of actual emitted code.
Two explicit markers consume one checked immutable input/result/retirement
plan. The same vectorcall produces a separate result owner. NULL exits before
any input retirement; success publishes B0 before closing callable, arguments
descending and actual self. The absent-self gap remains untouched. The existing
periodic check and delayed local/discard completion remain.

The reservation audit supports omitting the second argument-clear pass: the
parent's full slot extent remains reserved during callback reentry. No borrowed
argument ABI, new provider or arbitrary scratch result layout is introduced.
The source fixture checks actual NULL-error ordering and both execution variants.
Root found that the first runtime root fixture retained another payload owner;
the corrected fixture removes that owner and clears the source mapping during
input retirement. The result must survive solely through the declared B0 root.
The amended host source gate passes three methods. The
[Invoke proposal](native-invoke-proposal.md) records actual source scope and
pending gates; no runtime speedup follows from this source approval.
