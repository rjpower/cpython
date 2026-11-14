# Canonical element transport and multi-use borrowing

Status: the scalar VALUE foundation is qualified at integrated `8d0e3a8f22d`;
the list/tuple migration is source work awaiting its own freeze and native gate.
The foundation passes both native builds, the 52-case core in each role and
the 5+27 bootstrap/compile matrix. Its complete scalar normal graph now supplies
the transfer directly; an uncovered graph still uses the conservative transfer.
The chained native-result/error test includes a real child LINE callback and
the denied ancestor redirect. This is not complete M4 coverage.

This refines the [combined design](../../design/proposals/interpreter-performance56.md).
IRSC2 retained-transfer source remains a separate frozen component awaiting
integration. Root owns full generation, native qualification and measurements.

## First transport slice

Migrate exact list and tuple integer subscripting to one checked element-borrow
contract. Tuple subscripting is the new consumer; migrate the existing
`canonical_list_child_source()` proof and remove that body recognizer in the
same packet. Do not add a tuple body recognizer alongside it. Arithmetic proofs
remain separate until their own migration.

The concrete gap is `BINARY_OP_SUBSCR_TUPLE_INT`. Its successful body executed
`PyValue_XDUP(PyTuple_GET_VALUE(tuple, index))`, then publishes the result before
closing the tuple owner and commits the final recipient. The integrated handler
selected `IR_STORE_TEMP_DST` and `IR_STORE_DST`. Persistent tuple items
are canonical, but the C compiler sees a runtime payload and has no interface
that expresses that invariant. Exact list subscripting already selects the
canonical paths through a separate source proof.

This is a candidate for a recurring operation, not evidence that tuple indexing
is frequent in the measured hot set. Establish its actual hit count before
attributing a benchmark change to it. Literal boolean outputs are a weaker
first experiment: the native compiler may already remove their publication
checks. Ordinary local loads, some arithmetic results and unchanged returns
also already have canonical paths.

Root's corrected monitored-site census at `196284139a1` found no executed
`BINARY_OP_SUBSCR_TUPLE_INT` sites in hot6, while Richards list-subscript sites
had 85,090 visits. These are diagnostic site counts, not native instruction or
cycle savings. The tuple increment has no current hot6 benefit hypothesis;
all29, including tuple-heavy workloads, must establish its frequency. List
migration retains the existing canonical final store; its earlier temporary
store is also a candidate once the new producer fact is available.

## Producer, consumer and effects

The source implementation extends the existing `Local.ir_value` ledger. Checked
guard intrinsics establish the current exact-container and index dependencies.
`IR_DUP_CANONICAL_ELEMENT(container, index)` consumes those dependencies and
performs the anchored read and XDUP together. It supplies the existing
`result_owned`, `result_canonical` and `result_nonnull` facts; no borrowed C
temporary can escape between the read and acquisition. These are compiler
facts, with no new handler argument, runtime descriptor or frame field.

| Producer | Required facts | Result/effect |
| --- | --- | --- |
| Exact-type guard | Actual current object token and successful guard edge | Exact tuple or list, tied to that token |
| Index guard | Actual current index token; successful nonnegative and upper-bound edges | Index within this container's current extent |
| `IR_DUP_CANONICAL_ELEMENT` | Matching exact-container, bounds and current anchor facts | Atomic canonical element read plus XDUP; independent owning result, no exposed C borrow or callback |
| Existing successful NULL guard | Exact current result token | Normal-edge non-NULL refinement only |
| Actual Storage publication | Live owning result and canonical fact | Existing canonical temporary/recipient store plus the existing action receipt |

The emitter ties type/integer guards to actual value tokens and bounds to the
current container, subscript and lexical index. It owns the guard checks and
the read; source cannot substitute a different C pointer or helper. Branches
intersect dependencies, ordinary statements expire them, and source shadowing
of the lowering's native helpers or preprocessing is rejected. The actual
result enters the existing token graph through the shared helper-result fact.
The old list body matcher is removed in the same source packet.

The guard's successful branch supplies the fact. Mutation, reassignment,
shadowing, unknown address escape, incompatible join or an unsupported guard
loses it. A list bounds fact expires across mutation/callback. The owning
`XDUP` result survives independently once acquired. Tuple immutability does not
permit a borrow to outlive its container owner. Keep internal nullable tuple
construction and unchecked metadata reads outside this result contract.
The list interval currently uses the GIL; the new list guard takes the generic
path under FT. A future FT fact needs an actual lock/read/acquisition guarantee.

The emitter consumes the result at the existing `IRStack.store()` decision and
records the actual canonical publication in the same scalar action graph.
Delete the list producer-body proof only after both real list and tuple
handlers obtain their canonical-store selection from these facts. The generic
`BINARY_OP` family retains its conservative contract: a specialized member's
successful guards refine that member, not every sibling.

Canonicality changes normalization only. Keep input TAKE/vacate, the result's
owning duplicate, temporary publication before a possible finalizer, the final
destination store, discard cleanup, displaced-local retirement and continuation
refresh. A callback may change the named destination; commit and retire its
actual displaced owner using the current protocol. No borrow survives tuple or
list retirement, and no native private numeric result becomes canonical merely
because its type is VALUE.

## Falsifiable counts and controls

For each successful tuple-specialization hit, the source expectation is:

| Recipient | Executed publication sites eligible for canonical selection | Owner-count change |
| --- | --- | --- |
| Scratch | Temporary publication and final commit: two generated Publish expansion sites become canonical stores | Zero |
| Named local | Final commit: one executed Publish site becomes canonical store; temporary-publication condition is false | Zero |
| Discard | Neither store normalizes the ignored result | Zero |

Two generated expansion sites do not imply two surviving native normalization
sequences. Record native instruction/branch and code-size differences for the
actual compiled handler and recipient paths. The experiment fails its work
removal gate if those differences vanish under optimization. Existing list
handlers should retain their canonical stores, owner operations and observation
order after migration.

Root's diagnostic should separate specialized hit counts by scratch/local/
discard recipient, misses and fallback. The upper bound on executed candidate
sites is `2 * scratch_hits + local_hits`; it is not a cycle estimate. Attribute
neither zero-hit workloads nor generic BINARY_OP hits to this change. Keep any
diagnostic counters out of timed binaries.

Source controls must revoke the new fact for a changed guard type, changed
container/index argument, incomplete bound, wrong extent, pointer escape,
shadowed helper, mutation/callback before acquisition, borrowed-result escape,
and a dead owner alias. A callback after acquisition must preserve the owned
result but cannot preserve list bounds. Keep a positive source rename control.
Replacing `XDUP` with an unowned assignment must never authorize publication.

Generated controls should check list and tuple, both record modes, scratch,
local, alias-sensitive recipient and discard. Require the same actual
publication/action receipts used by replay; suppressing either receipt must
revoke the relevant proof. Runtime controls cover exact integers and floats
stored in tuples/lists, inline and object values, out-of-bounds/negative-index
fallback, index/container subclasses, displaced-local finalizers that mutate
locals or collect, and monitoring/redirect behavior. Use existing semantic
fixtures rather than changing recovery authority to make a test pass.

## Multi-use borrowing follows this slice

Existing copy propagation already crosses `continued` blocks. Its pending-load
record contains one fresh register, folds only `reads_count == 1`, and requires
a popped input with a `borrow_mask` field. `reads_count()` also counts implicit
uses and outputs; weakening that count would incorrectly discard passthrough,
handler-retained or escaping values. The folded-load verifier and allocator
also store one `reader`/`reader_k` and reserve a folded value until that reader.
These are the concrete interfaces that must change for a multi-use pass.

The initial slice is a callback-free, same-source-line, same-exception-region
web rooted in a `LOAD_FAST_BORROW` local definition. Track compiler-only anchor
versions, explicit COPY aliases and unchanged passthrough identities. Inspect
all uses of the entire web before committing a rewrite. Each terminal use must
be an actual nonescaping borrowed field; an output is either a verified alias
or a real demand, not an ignored occurrence. Initially reject a web with an
owning escape, required window materialization, surviving implicit/EH demand,
observer boundary, anchor write/delete, suspension, poll or unknown callback.
Leave its original owning load/COPY materializations intact.

For accepted webs, rewrite every terminal field to the same anchor, delete only
COPY operations whose complete output use sets are covered, and remove the
corresponding owning load. Use the same logical equivalence for allocation and
verification; a COPY output is a distinct owner until this complete proof
removes it. An unchanged input/output pair preserves an identity but does not
by itself create another owner. Folded consumers' ordinary local-input
retirement returns no input owner; an explicit TAKE still duplicates when the
consumer needs an owner before a callback.

Replace the singleton folded-load certificate with a bounded compiler operand-
pool slice of `(reader, input_index)` uses and a final-use/reservation boundary.
The verifier checks every surviving reader and every intervening barrier;
compaction keeps any necessary physical reservation through the final use,
rather than freeing it at the first reader. Count/check allocation products as
for other operand slices. This is compile-time storage, not executable NOPs or
runtime liveness/version metadata.

This first pass can remove a load plus COPY duplicates even when several fields
of one consumer name the same snapshot. It must also handle several consumers
when their actual conservative effects permit the region. Many apparent cases
will fail: rich comparison, container construction and finalizing input release
are barriers, while repeated independent local loads may already fold today.
Do not report the number of source LOAD_FASTs as available savings.

The completed same-line/EH morphology census closes this initial slice with
zero candidates. It covers 426 CFG graphs from 21 benchmark source files and
3,092 fresh borrow loads: 1,798 roots were already removed, 1,068 had a
nonborrow terminal, 204 crossed a callback before the last use, 12 crossed
line/EH boundaries, and 10 had nonfresh COPY provenance. The retained evidence
is `multi-use-borrow-census/census-final.json`. This is a static diagnostic,
not a proof of callback safety or a measured owner-count saving. Do not add a
compiler pass for this zero-candidate subset.

The next borrowing experiment must identify a concrete surviving family under
broader equivalence/materialization rules before implementation. Existing
`PYTHONREGCFGDUMP` pre/post identities remain the way to exclude loads already
removed by single-use folding. Count actual terminal acquisition too: a folded
`TAKE(local)` can reacquire the same owner removed with the temporary. A
nonzero source-operation count alone would still not establish a net pair or
native work saving.

Next, extend the same facts across acyclic CFG joins. An anchor is usable only
when all predecessors prove the same local definition and value. Otherwise
materialize the already evaluated snapshot on incoming edges before the
invalidating effect and merge an owning value. Never reload a changed local.
Backward demand includes explicit retained EH identities and observer state.
Keep backedges/polls and cross-line events materialized until their separate
observer/loop proof exists. This is the next borrowing packet, not a dependency
on complete M4, IRSC2 qualification or arbitrary allocation.

## Architecture peer conclusion

The combined design's separation of canonicality, ownership, boundness and
effects is sound. Shared fact production is a migration requirement, not a
claim that the current scalar graph covers all C, arrays, WAIT or callbacks.
INIT preservation across a checked callback does not preserve local value
identity; it cannot justify a borrowed snapshot. Mutable specialization
feedback cannot strengthen a cross-instruction compiler region without guards
and a pre-effect materialization/miss contract.

Keep the tuple/list slice and multi-use pass separately measurable. Neither
claims complete static readability maps, non-stack allocation, new refcount
semantics or a speedup before native and dynamic evidence exists.
