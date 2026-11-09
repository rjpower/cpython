# NULL PRE: emitted facts and consumer checklist

Read-only preparation against runtime488 plus the frozen decoder-only diff.
No runtime change or native execution. The parent brief and independent H/A
direction reviews remain the admission boundary.

## Emitted transfer ledger

Here B denotes the decoded BASE home, not necessarily raw IR_B: COPY/SWAP
encode BASE in A and their argument in B; FOR encodes BASE in raw B.

| Word/outcome | Actual emitted operations | Cold fact |
| --- | --- | --- |
| COPY | generated_ir_cases.c.h TARGET(COPY): load window[0], PyValue_DUP, publish window[oparg] | Destination B+arg transports old B. All other homes preserve. COPY's actual required read remains VALUE when NULL is not accepted; transport does not license copying NULL. |
| SWAP | TARGET(SWAP): load both endpoints into C values, exchange, publish window[0] and window[oparg-1] | Both destinations transport from the old row simultaneously. Middle homes preserve. |
| STORE_FAST | TARGET(STORE_FAST): assign IR_SLOT(A) to GETLOCAL(B), explicitly set IR_SLOT(A)=PyValue_NULL, then close displaced local | Scratch A becomes NULL on success. A is not inferred from a read mask. A named-local alias is outside tracked scratch. |
| FOR generic exhaustion | TARGET(FOR_ITER): found==0 calls CompleteStep with _PY_IR_FOR_ITER | CompleteStep detaches receiver/cursor, sets window[0]=window[1]=NULL, and does not publish the completion value to destination. B/B+1 are NULL. |
| FOR specialized exhaustion | FOR_ITER_LIST/RANGE/TUPLE/VIRTUAL exhaustion routes call the same CompleteStep | Same normal-edge fact; a guard miss/deopt is not the exhaustion edge. |
| FOR instrumented exhaustion | TARGET(INSTRUMENTED_FOR_ITER) found==0 calls the same CompleteStep | Same committed physical fact. PRE callback error does not receive this success fact. |
| FOR VM child completion | FOR_ITER_GEN stores parent return_pc then enters child; CompleteGeneratorReturn selects FOR and calls CompleteStep | Same B/B+1 clearing. No independent guessed VM completion layout is needed. |
| FOR body | Generic and instrumented handlers publish cursor and item; result may move to local or sink | Cursor/item are affected, not a general NULL fact. Conservative UNKNOWN is enough for first witnesses. |
| Other output/consumed homes | Exact destination mapping and opaque child footprint identify possible writes | UNKNOWN unless an audited final write proves a clear. Untouched homes preserve. |

The FOR success fact is after the whole outcome. CompleteStep may observe PRE
before clearing; failure or redirect there cannot use the exhaustion row.
Its successful POST observes cleared homes. A newer redirect changes the actual
phase/generation and must be recaptured, not interpreted as that original edge.

Generic WINDOW_DST_FIRST metadata alone does not prove a home was cleared.
The move/discard macros clear a source slot only for their actual local/sink
branches; temporary destinations can leave the result in that same home.
For this first query, unproved generic output homes remain UNKNOWN. H owns
exact query cases and their output-alias tests.

## Concrete storage and ordering

Extend RegisterResumeAnalysis with the forward must-NULL matrix and the two
backward requirement matrices. Own every allocation through the existing
analysis clear function. Keep analysis.required and analysis.safe for the
conservative ALL projection consumed by old strict adapters. Use size_t for
row/byte products, explicit nonzero allocation counts, and the existing cold
analysis budget applied to aggregate new storage. Do not add a second graph.

After handlers and register_resume_graph(), compute ordinary-entry forward
facts before classifying reads in register_resume_liveness(). Reuse edge_heads
and reached/queued storage; process at most one pending queue entry per node.
First reachable ingress installs a row, subsequent ingresses intersect it;
entry keeps its implicit initial-state predecessor even with a backedge.
Only an ordinary nonsuspending entry gets this new fact. Other analyses keep
their existing invariant-NULL behavior, without a fabricated resume seed.

Normal edges use the shared query from one immutable old row. Error edges
initially forget all scratch facts; this includes source errors unless their
complete publication is independently certified. That conservative choice
needs no additional source-record association field or partial-error decoder.
Payloads are therefore UNKNOWN. The three first witnesses have no error edges.

Refactor the existing backward scan into two demand lanes, with one fixed-point
schedule and the same successors/source index. Parameterize the error-needs
prefix join by the lane being read, rather than reading analysis.required
implicitly. Normal and source-success requirements undergo the selected edge's
transfer before ordinary PRE-error requirements join. COPY/SWAP map both lanes
back through old sources simultaneously. A transport is not a fresh-def kill.
Preserve immutable constant omission and the previously valid whole-code NULL
omission. After convergence, required=VALUE|NULL_ONLY and safe tests that ALL
row, not VALUE alone. A required physical alias classifies that read VALUE;
separate CFG paths may retain both bits, which is a refused NULL-aware target.

Add one NULL-aware target predicate in register_resume_plan(), before the old
strict recipe only for a newly eligible real-LINE PRE request. Require context0,
both save vectors empty, no VALUE target demands and at least one NULL_ONLY
demand. Every demanded home must be NULL in the stopped forward row and in the
actual frame. No preserve owner is added for NULL. Saved-binding safety and
actual-reader capability remain independent checks. Old PRE strict, neutral,
nonneutral POST and pending-child paths still see ALL requirements.

The existing warning reanalysis must rebuild these matrices and reread executable
handler capabilities. It must not retain a pointer into the old analysis.
Generation and physical NULL checks run again before the existing detach/install/
commit/invalidate/close application. No new hot or post-close work is needed.

## Small fixture packet

Keep the three original peepholer methods unchanged. Add bounded source-shaped
cases with encoded witnesses, not helper-name or lexical-method admission:

- Positive: COPY writes a scratch home, STORE_FAST clears that same home, then
  the target CALL uses it as NULL self. Assert actual home equality and one
  redirect/call, arguments and result identity.
- Positive: a completed FOR reestablishes that same NULL cursor home after it
  held an index. Pin exhausted edge and optional-self home; bound callback count.
- Required owner: the target is a method receiver use in the same physical home;
  it must refuse without changing PC, binding or receiver ownership.
- Alias: one physical home aliases a nullable self and required callable/value
  operand; required use vetoes NULL capability. Use the existing private encoded
  fixture style only if ordinary source cannot express that alias safely.
- Join: one predecessor establishes NULL and another a value; must-NULL must be
  absent. A separate VALUE/NULL_ONLY join conflict must refuse even if the actual
  stopped home happens to be NULL on this invocation.
- Unknown/error: an unproved transport or error ingress cannot manufacture a
  NULL fact. A small H contract fixture can cover a transfer not generated by
  ordinary Python instead of adding a broad debugger program.

Use existing warning getter/newer-redirect controls and actual-handler/all-alias
contract tests where they already cover reproof. Add new versions only for a
new path they do not exercise. First gate is the three unchanged originals plus
the bounded positives/negatives; larger observer/frame gates follow meaningful
passes. Root executes all native gates.
