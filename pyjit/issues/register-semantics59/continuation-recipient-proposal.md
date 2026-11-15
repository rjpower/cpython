# Continuation recipient action

Rejected executable: generation, both 117/79 builds and 112 methods per role
pass at 14bf4eb, but its eight-case screen loses important workloads. See the
[trial report](report.md). This is trial 4 of
the [semantic design](../../design/proposals/register-semantics59.md#4-encode-the-recipient-action-in-the-continuation),
reviewed against selected57 `0dcba75bb3df40c32137db453b001f7e8f101de0` in
`R/call55-integrated`. The independent builder candidate is a separate gate.

## Contract and encoding

The recipient belongs to the frame **receiving** a value: a caller awaiting a
child, or a suspended generator awaiting its next input. It does not describe
the child, its arguments, the value's representation, or a readable home map.
`return_pc` remains the executable continuation. Existing code ownership,
overlapping arguments, reservation frontier and child retirement remain intact.

Replace the existing `uint16_t return_slot` with `uint16_t return_recipient`
at the same offset. Use bits 0–11 for the home and bits 12–15 for the action:

| Action | Encoding | Publication and retirement |
| --- | --- | --- |
| INVALID | `0x0000` | No armed recipient; execution must not consume it. |
| VACANT_SCRATCH(h) | `0x1000 \| h` | Write h without reading or retiring its old bits. |
| REPLACE_LOCAL(h) | `0x2000 \| h` | Read old local, publish new value, then retire old owner. |
| DISCARD | `0x3fff` | Form no home address; retire the incoming owner. |
| EXTERNAL_ENTRY(h) | `0x4000 \| h` | Write the checked native entry cell once; no code lookup. |

Home actions require `h < IR_NO_DST`; the sentinel is not a home. No other
encoding is valid, including action zero with a nonzero home or DISCARD with
another payload. Zero initialization consequently cannot accidentally arm
scratch zero. This is one fixed encoding trial, not a search over layouts.

VACANT_SCRATCH is an owner-free write capability, not a physical NULL or read
capability. Preserve the existing actual admission/reservation proof for the
recipient. Do not infer vacancy from a missing scalar graph, caller ownership,
or action bits supplied by an untrusted image. The arena's physical NULL
initialization/cleanup contract is unchanged. The action also supplies no
canonical-value fact; the existing canonical versus normalizing transport
decision remains independent.

Put action constants and pure decode/encoding checks beside the frame layout,
accessible to local and remote readers. Assert the 12-bit mask matches
`IR_NO_DST` in the runtime header. Keep frame-aware binding/publication in
`pycore_interpframe.h`. No new frame field, owner plane, per-site table, or
serialized instruction field is needed. Preserve the 112-byte GIL/64-bit
header and phase offset 110; preserve current FT and 32-bit sizes as well.

## Binding and common completion

One `_PyFrame_BindReturnRecipient(frame, home)` classifies an already admitted
ordinary destination using the executing frame's pinned code/layout. Sentinel
testing precedes bounds or home addressing. A separate external-entry binder
accepts only the existing checked interpreter entry storage; it must not
manufacture an ordinary code/bitmap provider for a native frame. Pure accessors
return action and home; consumers do not repeat masks or nlocals comparisons.

Also permit the same packing/store operation with an already proved action.
Use `_PyFrame_BindScratchReturnRecipient(frame, home)` for the validated scratch
producer, `_PyFrame_BindDiscardReturnRecipient(frame)` for a constant sink, and
the external binder above for native entry. All use the same pure encoding
operation; scratch bounds/partition assertions remain in debug builds. These
are typed producer interfaces, not runtime action-table lookups.
The existing checked SEND entry recipe supplies a scratch B+2 recipient: its
validated input/protocol window is above locals. That producer may bind
VACANT_SCRATCH directly, without classifying it again. The receipt is the
actual window/entry recipe, not the SEND handler name. A source form without
that receipt retains general binding; the setter's spelling cannot establish
the proof. Public created-generator
DISCARD and native EXTERNAL_ENTRY(1) are also constant actions. General call A,
FOR body A and yield's next-input A can name a local, scratch or discard; they
still need classification. Do not add new execution variants or a per-site
action table to make this trial look cheaper.

Bind at the existing callback-free handoff commit, after the last refresh and
before the child becomes current. If binding preparation can run callbacks,
refresh and revalidate first. Classify against the caller's executing code,
not the callee's code or a mutable function's current `__code__`. The action is
stable through named-local value mutation because that does not change the
frame's local/scratch partition. No new FT execution claim follows from it.

`_PyFrame_SetReturnValue` continues to consume exactly one owned PyValue. It
resolves pending redirect first, then loads the current descriptor and performs
the action above. Local publication precedes displaced-owner close; discard
and its callbacks remain at the current point. Scratch/external publication
must not read old bits. Phase-state reset and the helper's redirected result
retain their existing meaning. Invalid action is an internal invariant failure;
untrusted remote descriptors fail with the reader's ordinary error instead.

All saved-continuation return/yield/resume paths share that publication meaning.
Immediate native CALL transactions already publish scratch results directly
and delay local/discard commit through retirement and the periodic check
(`ceval.c:1694–1867`). Do not route them through an eager continuation store,
add header writes to calls that push no child, or move their retirement phase.
Their destination action has the same ownership meaning but a different commit
schedule. Only the transaction's Python-child handoff binds the saved field.
Likewise, the existing FOR/SEND completion outcome decides **whether** a result
exists; a recipient action must not turn FOR exhaustion into an A publication.

## Writer and reader closure

| Boundary / current source anchors | Required port |
| --- | --- |
| `interpframe.h:397,495,844`; `pystate.c:1491` | Copy descriptor unchanged. New ordinary frames and the base sentinel start INVALID. Trampolines explicitly bind their real ordinary destination after code/owner/base initialization. |
| `ceval.c:2595` | Native evaluator entry binds EXTERNAL_ENTRY(1), which targets existing `entry->stack[0]`; never classify through NULL `f_executable`. |
| `ceval.c:3863,3904,3983,4090,4343,4410` | Init shims, overlap/default/keyword plans and fallback bind the caller at their existing final handoff. Preserve argument transfers, staged roots and publication order. |
| `bytecodes.c:1227,5558`; generator disjoint-child lowering | Port `_BINARY_OP_SUBSCR_INIT_CALL` and `_SAVE_RETURN_SLOT`, their checked source templates and receipts. No raw field write may escape the shared binding contract. |
| `bytecodes.c:1743,5279`; generator suspension/created-copy templates | YIELD_VALUE/YIELD_DELEGATED bind the generator's next-input destination before publishing suspended state. RETURN_GENERATOR binds the copied frame's admitted first-input recipient. |
| `ir_generator.py:5652` | Generator entry binds the parent to explicit A for FOR, and B+2 for SEND. These are different roles; retain the existing checked entry recipe and normal/error/WAIT receipts. |
| `genobject.c:297,1278` | Public send/throw uses the saved descriptor; frame-stealing constructor explicitly installs DISCARD for first None input. Preserve state checks, exception input and exact close order. |
| `frame.c:220`; `frameobject.c:3564,3843` | Pending-call debugger redirect remains limited to an ignored-result CALL/CALL_KW. Test exact DISCARD, never compare the packed word with IR_NO_DST. |
| `ir.c:708` | Delegated receiver recovery decodes and validates action/home against immutable yield A and the frame partition before reading the receiver. No masked-home-only acceptance. |
| `Modules/_testinternalcapi.c:5241` | The synthetic protocol-frame probe uses the ordinary checked binder after initializing its real code/base; it must not fabricate a packed descriptor by assigning A directly. |
| `ceval_macros.h:878`; return/yield/native completion helpers | Keep normalizing versus canonical input paths and source/error origins unchanged; both consume the same admitted action. Native step completion still uses its immutable FOR/SEND outcome. |
| `ceval.c:2386` and frame retirement | Error completion publishes no successful result. Keep pending-redirect cancellation and existing unwind origin; a stale, unarmed descriptor is not a reason to touch a home. |

The existing pending redirect has an important narrower contract than “rebind
any destination.” Its admission requires the pending call's destination be
DISCARD. `_PyFrame_CompleteRedirect` installs the new continuation and clears
scratch only after the child releases its overlap. Preserve that rule and the
DISCARD assertion. Load the descriptor **after** this callback-capable cleanup;
do not cache the pre-redirect action across closing callbacks. A future used
result redirect would need its own recipe, not this encoding change.

Materializing f_locals, kwargs, a frame object or capture does not itself arm or
reclassify a continuation. Copy/takeover carries the descriptor alongside
`return_pc` and existing state; GC visits values using its current home provider,
not this action. Unstarted generators must get an explicit first-input action;
finished/cleared frames must not decode through a released code pointer.

Port `pycore_debug_offsets.h`, remote offset validation and asyncio unwinding
as one internal format change. Rename the exposed field, retain its 16-bit
width/offset, and increment the remote ABI format discriminator so an old
reader cannot silently interpret encoded actions as raw slots. Remote readers
validate kind, canonical sentinel, owner, partition, bounds and immutable yield
A before deriving an address. Update `remote_debug_layout` and corruption tests
for invalid kinds, wrong local/scratch action, DISCARD mismatch and out-of-range
homes. No remote pointer supplied by a descriptor is trusted.

Update all generator source templates, source-effect matchers, ownership
receipts and exact-C fixtures together; do not grant transport authority because
a new binder name appears. Include ordinary, instrumented and transparent forms.
Whole-tree `return_slot` census must leave no raw writer or semantic comparison.
Generated products must retain existing format/admission/observer/error metadata
except evidence required by an actual changed source spelling. There is no
instruction encoding change and therefore no bytecode-magic bump for this trial.

## Qualification and falsifiers

Source gates cover every writer and malformed descriptor above, preserve exact
frame offsets, and show both canonical and ordinary completion. Poison the
binder source/IO/recipient role and require the existing typed contracts to
revoke; do not add name-only exemptions. Runtime focus: general/empty return,
exact/default/keyword and disjoint-child calls; local/scratch/discard and native
entry; generator send/throw/close, delegated yield/coroutine completion; local
replacement finalizer mutating f_locals and running GC; ignored-call redirect,
entry callback failure, frame copy/takeover and remote asyncio corruption.
Use the existing return, frame-call, completion, delegated-yield, debugger-entry
and external-inspection modules rather than a parallel completion framework.

The coordinator must inspect complete bind-plus-return native paths. Removing
the code/nlocals lookup from publication can merely move the same lookup to
binding; one call normally binds once and returns once. Count the added masks,
action branch, binder loads, total instructions, spills, text and helper calls.
No release debug scan or new call overlap/direct return is a saving here. The
rejected FOR result experiment demonstrates why fewer source stores are not a
native or throughput result.

### Retained native cost inventory

The selected57 ELF is `E55/comp57-dictionary-native/python`; the following is
static disassembly, not a candidate cost or execution-frequency measurement.

| Existing path | Actual operations | Proposed replacement / installation cost |
| --- | --- | --- |
| RETURN_VALUE_EMPTY `0x3987a1–0x3987b1`; YIELD_VALUE `0x34e495–0x34e4a5` | Owner-byte compare and branch; recipient code-pointer load; nlocals memory comparison and branch. Five instructions, including the dependent code/nlocals reads. Discard testing and home-address calculation precede these. | Action decode/test replaces owner/partition inference; home masking is added. Discard, redirect, actual store and local retirement remain. Exact instruction/branch count awaits candidate code. |
| CALL_NO_SELF_PY_EXACT_ARGS `0x387fa0–0x387fbd` | Decode/mask A and save its raw slot. There is no release caller-destination nlocals comparison to reuse. Earlier callee-code loads do not describe the caller's local partition. | General binding adds caller classification and action packing here. Charging only the removed return-side load would overstate the benefit. |
| SEND_GEN `0x396bc4–0x396bcf`, then `0x396c42–0x396c59` | Save parent B+2 without partition classification; on resumed-child input publication, compare owner and load child code/nlocals. YIELD later classifies the parent again. | Bind the proved parent scratch action directly, with packing but no parent code/nlocals query. The generator's yield-A classification moves to suspension, so it is not independently eliminated. |
| `call_transaction_native`, `ceval.c:1801,1845` | Existing `to_local` selects immediate scratch publication versus delayed local/discard. This classification runs on the native-result route. | It is not an already computed action on the disjoint Python-child handoff route. Keep it and its commit phases intact; do not compute/store an extra saved descriptor for native-only calls. |

In a normal SEND/yield cycle the parent B+2 action is known scratch, whereas
the generator's incoming A can be local or scratch. The first partition query
can disappear; the second generally moves from the next resume to suspension.
Both publications gain packed-action decoding. That is a concrete opportunity
and a concrete charge, not two eliminated classifications. Similarly, a plain
Python call normally installs one action and consumes it once: without reuse
or cheaper instruction selection, classification has merely moved.

`call_transaction_begin` does not already provide a usable release destination
classification: its destination/local bounds checks are assertions. Current
call role variants describe self/kwargs geometry, not A's local/scratch role.
The only existing execution variants with `local_fields` are attribute-load
variants. Do not claim those unrelated facts for calls.

Acceptance requires unchanged header sizes/ownership/recovery and measurable
ordinary call/resume benefit after charging installation. Use unchanged hot
and broad workloads under the standing measurement gates; inspect DeltaBlue,
coroutine/generator paths and logging explicitly. Reject if action installation
duplicates classification, immediate calls gain staging, native entry needs a
fake frame provider, or new spills/branches cancel the return-side reduction.
Do not grow the descriptor, introduce a universal per-yield lookup, or search
alignment/layout seeds to rescue a losing result. Root owns implementation
authorization, native qualification and selection.

## Source packet

The complete source closure is implemented in `R/call55-ir`, branch
`pyjit/reg59-recipient-action`, commit
`7b295bf6caf74a03414f9995f0f6e4523223d49b`, based on selected57. It includes the shared
16-bit action contract, all constructor/handoff/suspension/copy writers,
publication and redirect readers, ABI2 remote decoding, source matchers and
fixtures. The only binder emitted directly by the DSL is explicitly classified
callback-free; a binder call alone still supplies no typed home/error grant.
Six focused host source methods pass at
`E59/recipient-source-host/callback-classification.log`; independent actual
source/native review is GO. Root's final generated candidate is
`14bf4eb1784a8312fd2a6d89f2f63d191547bfe5`. Both 117/79 builds and 112 focused
methods per role pass, including a corrected actual-local constructed YIELD
across all four public image routes. The first misidentified scratch witness
and encoder-name-shadow correction remain documented in the report. Three
remote descriptor/corruption methods pass per role on the same implementation.
The eight-case screen rejects the executable; this contract is unselected.
