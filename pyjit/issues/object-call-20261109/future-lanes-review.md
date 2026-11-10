# Review of the next generator and cell lanes

Read-only peer review, 2026-11-10, against runtime `0a35a382909` and
[generator-cycle-proposal.md](generator-cycle-proposal.md) plus
[value-native-cell-proposal.md](value-native-cell-proposal.md). No implementation,
native run or measurement was performed by this reviewer.

**Approve the generator census and contract map, not an EMPTY_TEMP implementation
yet. Approve the bounded cell getset proposal for implementation after the
coordinator's current checkpoint validation and approval.** The cell proposal
has a concrete removable representation round trip and reuses an existing ABI.
The generator proposal has a useful whole-cycle ledger but not yet evidence to
choose its preferred optimization.

## Generator: the preferred experiment is still unproven

The proposal correctly separates creation, activation, input installation,
suspension, delivery, forwarding and completion. Its source-model counts expose
why recursive delegation amplifies resume/yield work and why construction-only
tuning cannot explain a generator with 1,000 yields. These counts must still be
confirmed against actual executed handlers on the current candidate: the
object/call changes can alter other work in the tree benchmark.

The EMPTY_TEMP mechanism, however, is materially the same experiment as
[return-recipient-options-after36.md](../frame-call-model-20261109/return-recipient-options-after36.md).
That earlier proposal already identified SEND's cleared B+2, warned that updates
and deliveries are approximately paired, required one recipient representation,
and listed debugger/remote-reader implications. The new recurrence and current
profiles improve workload understanding but do not resolve its core break-even
question. Do not call this a new structural optimization or promote it to the
preferred implementation solely because the ledger is now complete.

The current sink's old-owner load/test is a plausible target, but the competing
implementation adds descriptor decode and potentially branches/masks to every
consumer, plus encoding at producer sites. A general one-store saving is not an
amortized benefit when the descriptor is rewritten for almost every delivery.
All native/general, local and discard routes affected by a shared decoder belong
in the accounting. The FOR-only `generator_resume` micro has no SEND benefit;
it can still regress if its shared sink acquires another check.

Before selecting the experiment, produce this weighted ledger from actual
counts and retained compiled code:

`net instruction change = sum(route_count * changed instructions per route)`

Routes must include recipient updates, incoming installation, yielded delivery,
completion, redirects and newly affected general sinks. List added work as well
as removed work. Count one complete activation/yield pair rather than presenting
the delivery-side saving alone. Measure branches, cycles and elapsed time too:
the existing NULL old-owner test may be cheap and well predicted, and instruction
count alone cannot establish a throughput gain. Reject EMPTY_TEMP if the weighted
normal-path ledger is nonnegative, or if a small local saving causes a repeatable
regression in FOR/native/general routes. A negative ledger authorizes a bounded
prototype and paired measurements, not automatic integration.

## A consistent resume convention without universal overhead

There is already substantial unification. `IR_ENTER_GENERATOR` emits the shared
VM SEND/FOR commit sequence in `Tools/cases_generator/ir_generator.py:3901`.
Generator terminal completion shares `complete_step_impl`. Yield delivery and
incoming installation use `_PyFrame_SetReturnValue`. Another universal dispatcher
or per-frame mode field would not, by itself, make these paths more coherent.

The useful next artifact is one explicit resume contract with concrete policies:

| Fact | SEND | FOR | Native send/next/throw |
| --- | --- | --- | --- |
| Incoming ownership | Move from caller B+2 | Supply None | Duplicate borrowed input; exception entry may apply |
| Parent recipient | Cleared B+2 | Encoded local/temp/discard | Interpreter/native result route |
| Activation | Existing VM frame/exception/recursion commit | Same VM commit | Native entry owns its evaluator boundary |
| Outcome | Yield or delegation completion | Item or exhaustion | NEXT/RETURN/ERROR classification |

Use those facts as emitter-time policies or existing producer proofs whenever
possible. Keep the same commit-before-retire ordering and common completion
transaction; avoid a runtime classification step for a distinction already known
from the opcode. Common semantics do not require every provider to execute every
other provider's checks. Native `gen_send_value_ex` has a distinct status boundary
and handled-exception setup; it must not acquire VM overhead merely to share
more source text.

The census should select among three opportunities, without preselecting one:

- Remove one repeated action in the shared activation/yield cycle, if it is
  proved redundant under the existing contract and helps SEND and FOR.
- Fold a precisely recognized delegation forwarding continuation if dispatch
  counts justify it. This is potentially more substantial than a recipient tag,
  but requires a separate proof of suspension, throw/close, `gi_yieldfrom`,
  observation and real continuation reconstruction. A live delegation frame
  cannot simply disappear from those protocols.
- Encode SEND's empty recipient only if its measured net ledger wins. Reuse the
  existing recipient abstraction; do not introduce another completion ABI.

These are decisions after census, not three changes to implement together.
Direct owning generator construction remains independent and should advance
only when construction/copy costs, rather than yield counts, justify it.

## Cell: approve the concrete boundary port

The proposed cell getter/setter pair uses `_Py_VALUE_GETSET` and the existing
typed callback record. Source inspection confirms `getset_get_value` directly
invokes `d_value->get`, while the shared object thunk supplies the owned object
result for object callers. No new descriptor machinery or cast is needed.
Converting both callbacks is correct: a typed record with a missing setter would
not preserve the legacy writable path.

Ownership is sound. `_PyCell_GetValue` returns an independent owner; return it
directly without closing it. Empty attribute access must still raise precisely
`ValueError("Cell is empty")`, whereas empty `PyCell_Get` retains its different
no-exception contract. The setter must use `PyCell_SetValue`, which duplicates
the borrowed input, swaps the canonical home, then retires the old owner.
Self-assignment, deletion and reentrant replacement therefore retain their
existing commit order.

The source-derived boxing ledger is a credible admission hypothesis for inline
3000. Verify controlled deltas rather than process totals: the CELL site is
shared, and the object descriptor thunk's box attribution changes to ATTRIBUTE.
Do not require `entry_unbox` to fall by the read count, and do not mistake a
counter moving sites for eliminated object-API traffic. The retained public
object getter must still box when needed and propagate allocation failure.

The proposal's tests cover the material risks: value-native and object paths,
empty/deletion behavior, retained mortal results, descriptor receiver checks,
numeric subclasses, nonlocal/attribute coherence, cycles and nested finalizers.
The optional forced-boxing-failure extension must resolve the descriptor before
installing the failing allocator and restore it before assertion/cleanup.
Cell itself is not subclassable; testing stored numeric subclasses is still
necessary. Production scope should remain `Objects/cellobject.c`, with only the
stated existing test fixture extension if needed.

This is a consistent completion of the canonical-value boundary, not a cell-only
alternative value model. It is reasonable to retain a narrow port with a proven
bridge reduction and no material regression even if its whole-application share
is small. Report that limitation rather than expanding to unrelated getsets to
manufacture a larger gain. Neither this approval nor the generator ledger
authorizes edits to the currently measured checkpoint.
