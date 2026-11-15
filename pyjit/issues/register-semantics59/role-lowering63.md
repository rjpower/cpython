# Rank 1: fixed storage-role lowering

Source-reviewed unit, 2026-11-14; phase 1 implementation authorized. Root and frames independently approve the direction. The FOR item must remain BODY-only in exported state; unsupported conditional graphs remain UNKNOWN. Consumed roles only narrow an actual reached TAKE/owner transfer. No implementation or native qualification is implied by this review. Read against the current [grouped design](../../design/proposals/interpreter-parity63.md) and [review63](review63.md). Selected source is `BASE/worktrees/call55-integrated` at `0dcba75bb3df40c32137db453b001f7e8f101de0`; canonical checkout runtime is not the source baseline.

The phase-one proposal used P on branch `pyjit/parity63-role-lowering`, created directly from selected57. Its frozen source and measured rejection are recorded below; the handoff owns current branches. The prepared branch remains at `6e07c1a89532fde13e4a6ca96fef9c37f96cf566`. No round59–62 runtime packet is a prerequisite. No production source, generated product or executable was changed or run for this proposal.

## Unit and interface

Generate five used storage signatures, initially list iteration/append and then three attribute consumers. Keep the ordinary DSL bodies, guards, providers, semantic words, PCs and recovery identities. There is no dispatch removal, persistent descriptor, per-hit role table or new frame field.

Extend the existing Python-only `ExecutionVariant` and `SlotLayout` with:

```python
consumed_fields: tuple[str, ...] = ()
local_result: bool = False
```

Existing `local_fields` means a borrowed local input and `temp_fields` means a scratch result; neither already means a consumed scratch input. `local_result` selects only the checked FOR body-edge recipient in the first unit. Do not generalize all window results. A declaration is a requested lowering, not ownership evidence. Validation checks that fields are actual IO roles, local and consumed roles are disjoint, and every changed action has the same reached source receipt as ordinary emission.

`IRStack.consume` emits the existing take directly for a consumed scratch field, with its actual VACATE/owner event. `take_view` emits that acquired owner directly for TAKE, avoiding the subsequent nullable-owner choice; for a fixed local it emits the existing XDUP and OWNER is NULL. It still spends the current input and uses two-pass view/owner checking. Missing acquisition, a second TAKE, alias overwrite, escaped address, reload or unsupported control must deny the refinement. Do not seed `Local.owns_value` from the variant declaration or a canonical-output annotation.

Use existing canonical publication and ordered retirement primitives. A new spelling is justified only if an existing macro embeds the role branch being removed; factor its shared action rather than duplicate its owner algorithm. In particular, a fixed local writer must retain `IR_RETIRE_LOCAL`, including immediate nonfinalizing close and deferred finalization. A fixed scratch writer must transfer the owner exactly once. No result becomes readable or canonical because of its destination role.

## Concrete signatures and source order

| Executing signature | Lowering and source evidence | Work retained |
| --- | --- | --- |
| `FOR_ITER_LIST`: A local, B iterator window | Checked body-edge item acquisition; commit item directly to A after cursor publication, without item publication at B+2 or its subsequent load/move | Item XDUP, list/type/bounds checks, ordinary cursor normalization, physical B+2 vacancy, old-local retirement, completion/error exits |
| METHOD_NO_DICT / WITH_VALUES: B local, A/A+1 scratch | Existing guarded class-value DUP and receiver TAKE; fixed local receiver produces XDUP and no scratch-take decision; both results use reached canonical owner stores | Both acquisitions and stores; all lookup guards. Existing canonical outputs earn no new normalization credit |
| `LIST_APPEND`: A list scratch, B item scratch | Reached `item = TAKE(v)` becomes scratch take/clear, passed to unchanged `_PyList_AppendTakeValue` | Consuming provider, capacity/resize, error PC and cleanup; no append inline rewrite |
| Nonmethod `LOAD_ATTR_CLASS`: A=B scratch | Read current receiver, acquire cached canonical result, take receiver owner, publish result into vacated same home before retiring receiver | Type/version guards, receiver callback retirement and revalidation; no borrowed result or extra count |
| `STORE_ATTR_SHAPE`: A receiver local, B value scratch | Actual TAKE moves value to the attribute; receiver OWNER is NULL; retire displaced attribute only | Shape/type guards, attribute publication, callback/error/redirect handling and reload |

The fields above are the selected format's fields, not stack-position guesses. `compute_formats` and `slot_layout` resolve the actual IO names/positions. Add no variant for another geometry merely to increase a census count. Existing SHAPE_LOCAL_TEMP/POLY_LOCAL_TEMP and canonical subscript/arithmetic handlers remain controls, earning zero new saving.

### FOR body-edge receipt

This is the one missing producer contract in phase one. Selected `canonical_output_sources()` rejects windows, and `emit_final_outputs()` currently flushes B+2 before `IR_MOVE_TO_DST`. Neither can simply be overridden by a family name.

Add a bounded recipe resolved from the actual reached parts and external IO. It requires the unchanged iterator/cursor inputs and outputs, the actual exact-list exit check, the existing unsigned bounds/exhaustion branch with its `IR_COMPLETE_STEP(FOR_ITER, ...)` terminal, the canonical list-element read followed immediately by XDUP, the existing cursor increment, and no intervening callback, mutation, address escape or alternate item assignment. Bind the acquired item to the current output Local. Only that item gets owning/canonical evidence; the cursor gets none from this recipe. Deleting/changing a guard or acquisition must make the requested form unavailable or fail its emission check.

Reuse the existing source effect/AST traversal and checked element acquisition machinery for those receipts. If the list getter needs a typed canonical-borrow result contract, attach it to the actual canonical container getter and reached XDUP, with its existing native preconditions. Do not infer readiness from asserts, helper spelling alone, GET_ITER ancestry, or a generic OBJECT result. Do not introduce extra type/tag/bounds checks to make a declaration true. The existing `PyList_GET_VALUE` out-of-line boundary is unchanged in this unit; the rejected public getter-inline experiment is not imported. FT uses ordinary execution initially.

On the body arm, final flush publishes the ordinary iterator/cursor state first, preserving cursor normalization. It then writes B+2=NULL without reading its old bits, replaces A with the acquired canonical owner, and retains/releases the displaced owner through the existing local-retirement protocol. This single physical clear is retained in the first implementation even if some incoming states prove NULL. It avoids another NULL-dependent variant and permits positive NO_HOME-only admission. There is no intermediate item store or reload. The item acquisition precedes replacing A, so receiver/item aliases and list mutation in the displaced callback remain safe.

The exhaustion arm reaches the existing completion adapter before item acquisition and does not publish A. The storage model records B+2 as vacant, A as the recipient, and the item native owner as spent. A final store return value must not claim the frame holds the item at B+2; kill the terminal Local after the commit. Preserve the existing phase/PC at any displaced-owner callback. Unsupported observation recipes run the ordinary handler.

### Attribute retirement and aliases

`LOAD_ATTR_CLASS A=B` is consumed-alias-after-TAKE, not vacant-before-entry. Require its readable receiver and actual receiver-owner acquisition. Never demand NO_HOME at its input. The existing `IR_CANONICAL_HOME_DUP` and `IR_RETIRE_PUBLISHED_RESULT` publish the result before callback-capable close; the fixed TEMP variant must use that publication as its only success store. It must not repeat the old final store from stale C bits after reentry. Preserve existing published-node/current-home receipt checks and reload/redirect exits. A missing receipt leaves this consumer ordinary.

Method outputs are fresh destinations: each needs positive absence independently, plus reached current DUP/TAKE ownership. Use a write-only canonical publication for positively owner-free scratch destinations; do not read stale raw bits merely to assert NULL. No method-to-CALL span, pending call alias or native callable binding is included. Shape store publishes the moved value before the old attribute can finalize, keeps the receiver local alive under the existing callback protocol, and never closes a fictitious receiver owner.

## Common admission and adaptive writers

Select in the unpublished executing copy from final fixed-point common replay. Compiler output, constructor, replace and marshal share this path; compiler nomination is not authority. No line/column gate, global-complete requirement or added proof interpreter.

- Local roles require the actual local partition, excluding hidden constant homes.
- Consumed scratch inputs retain ordinary opcode input admission/readers and require the reached consuming action; require distinct input roles except the explicitly checked class A=B result alias. For LIST_APPEND, root and frames independently approve narrowing the existing scratch CONSUME/TAKE expression without extra replay INIT/NONNULL: the taken owner equals the current scratch value; if NULL, the original XDUP(NULL) alternative also returns NULL. No callback intervenes. This removes a choice without granting initialization, canonicality, readability or fresh-output authority. A lost replay fact does not by itself deny this equivalent lowering.
- Fresh method output homes and FOR B+2 require positive NO_HOME and no MAY_OWNER. NO_HOME never grants INIT, NONNULL or physical NULL.
- In-place class publication requires the source-proved take before replacement, not incoming vacancy. STORE_ATTR value consumption preserves its actual error outcome.

Static LIST_APPEND can select its private form before the no-feedback early return. Adaptive FOR/ATTR/STORE forms use private cold pending aliases with ordinary dispatch targets. Centralize live replacement in the reviewed bounded writer source mechanism, stripped of rejected spans and private heads. These signatures cover one word: no predecessor search or span lookup is needed. Retain the pending role signature through supported family specialization; unsupported members and instrumentation map back to ordinary semantic handlers. Generate mappings from the finite role declarations. Keep FT CAS/instrumented races unchanged and decline cold role selection under FT initially.

Move the existing geometric SHAPE_LOCAL_TEMP choice into that same selection interface without enlarging its admission or claiming its savings. Preserve its PIC admission and learned-only rules. Reserve private IDs only after root coordinates them; no numeric assignments in this proposal. Export them as execution-only, reject public private IDs, and close base/shadow/disassembly/introspection/monitor restore consumers. Wrapper opcodes stay byte-sized ordinary IDs.

Variant metadata must describe actual actions. A fixed role narrows physical choice but does not invent normal/error/owner certificates. Compare the complete ordinary and variant receipt sets with only the declared storage substitution; preserve source-derived callback, allocation, release, polling, synchronization and error effects. FOR's body-edge description must not overwrite exhaustion semantics with an unconditional scalar publication. If existing exported graphs cannot express that distinction, retain honest UNKNOWN authority for that portion and generate only the bounded cold nomination facts needed by this consumer.

## Closed implementation sequence

1. Add the two fixed-role choices, format validation and reached take lowering in `Tools/cases_generator/ir_generator.py`. Add the bounded FOR recipe and final body-edge commit at `emit_final_outputs`, with storage truth and local retirement preserved. Declare only FOR-local and LIST_APPEND-consumed forms initially.
2. Add final-replay nomination in `Objects/codeobject.c`; generated role selectors/base mappings in `ir_metadata_generator.py` and `pycore_ir.h`; central replacement/restore in `Python/ir.c`, `specialize.c` and `instrumentation.c`; introspection export in `_opcode.c`; registry private IDs. Root generates all products.
3. Review and qualify list/FOR/append as a complete path before extending the same choices to both method producers, class consumed alias and shape store. No new role field is expected for phase two; a missing source receipt must be reported rather than patched with a family exemption.

Likely source files also include `ceval_macros.h` for a factored fixed-local commit and the targeted bytecode producer declarations if a typed FOR acquisition marker is necessary. Keep changes confined to the named actual producers. There are no frame/header/provider/allocator changes or per-code allocations.

## Focused evidence and stop gates

Host source checks, both record modes: ordinary/variant complete effect and receipt equivalence; renamed producer positive; removed XDUP/TAKE and changed canonical read negative; declaration without reached owner negative; overwrite/copy/address/callback/control negatives; FOR missing bounds/changed completion/zero-iteration body; wrong local/temp geometry; private base/shadow/flags and real selector/header emission. Check every source-derived marker registry if a marker is added. Do not repeat earlier mistakes by registering ordinary native functions as global DSL replacers.

Root native minimum: canonical int/float/object list items, empty/exhausted/mutated lists, item/list/old-local aliases and displaced finalizer with f_locals mutation+GC, callback redirect/error preserving B+2 vacancy; append capacity/growth/OOM consume-on-both-outcomes and mortal item lifetime; all four public construction routes and live/unknown/constant-home rejection. Reuse `test_ir_semantic_completion`, `test_ir_window_result_transfer`, `test_ir_operand_modes`, admission and owner-effect controls. Phase two adds receiver/result alias and old-attribute finalizers, method mutation/miss, and enable/disable instruction monitoring with ordinary restoration. Observe useful poststates and eventual release, not incidental exact destruction timing.

Native review must show the actual role tests and redundant publication removed over complete success/error/retirement paths, price new clears, prologue state, branches and spills, and measure duplicate text. No new provider hop, per-hit selector load or extra guard is acceptable without an explicit whole-path justification. FOR keeps one B+2 clear and cursor normalization; append keeps its ordinary out-of-line provider. These costs cannot be credited as removed. If a role merely replaces one branch with another, leave it ordinary.

Retained structural counts are 1,686,000 list-local, 912,000 method-local, 864,000 append-consumed and 6,000 shape stores per 6,000 comprehension trays; Delta supplies 1,451,610 method, 942,090 class-alias and 91,530 store visits per 30 loops. They are monitored operand-shape ceilings, not admissions or hits. The design's illustrative 10,810 instructions/tray is about 3.9% of selected count, not a prediction, bound or parity claim. Root must collect actual nominations/hits and native whole-path costs before its unchanged broad screen; logging/ray/spectral/nqueens remain sentinels. Reject repeated broad losses rather than seek layout/flag variants.

Source anchors in selected57: `ir_generator.py:168–181,2333–2376,2400–2425` (variants/layout); `:2973–3115,4475–4525` (consume/store/TAKE); `:6495–6510` (window final move); `:6560` (scalar-only canonical output contract); `bytecodes.c:1239,2827,2989,3840–3885,4079–4116` (the five producers); `ceval_macros.h:535–626` (input ownership and publication); `specialize.c:670–700` (existing SHAPE role selection).

## Phase-two source refinement

Root and independent source peer approve this refinement; isolated implementation is under way after phase one loses its broad screen.
The following sequences refer to selected57, including its existing generated
publication and retirement paths. No new private IDs are assigned here.

- **METHOD_NO_DICT / WITH_VALUES, B local and A/A+1 scratch.** Keep every
  existing lookup guard. The reached guarded-cache DUP acquires `attr`; the
  reached TAKE of the fixed local acquires `self` by XDUP without consuming B.
  Match those statements through `canonical_output_sources`, then require their
  actual current owning results at emission. The annotation alone supplies no
  owner. Each destination independently needs final-replay NO_HOME and no
  MAY_OWNER. Publish both acquired canonical owners with the existing write-only
  `IR_PUBLISH_CANONICAL_TEMP`; never inspect stale destination bits for a NULL
  assertion. There is no callback between acquisition and these stores. Ordinary
  method handlers already use canonical stores, so the saving is fixed input
  role/TAKE selection, not normalization or either acquisition/store.
- **Nonmethod LOAD_ATTR_CLASS, A=B scratch.** Retain the type/version checks,
  actual canonical cached-result acquisition, and existing explicit
  result-publication contract. The reached `OWNER(owner)` consumes B and makes
  A physically vacant before publishing `attr`; this is an acquired vacancy,
  not an incoming NO_HOME requirement. Publish once before retiring the receiver
  through `IR_RETIRE_PUBLISHED_RESULT`. Its existing published-temporary receipt
  suppresses the final store, including after callback reload. No stale native
  `attr` bits may become a second owner or authorize replay. Fixing scratch and
  nonmethod roles removes local/sink choices. Keep the suffix's oparg test in the
  first packet rather than add a literal-oparg lowering. Ordinary scratch
  execution already publishes once; claim no removed duplicate store. Preserve
  the receiver release, callback phase and redirect exits.
- **STORE_ATTR_SHAPE, A local and B scratch.** Keep the guards and displaced
  attribute move. The actual TAKE clears B once and transfers its owner into the
  field before retirement. Fixed local OWNER is NULL; it must not acquire or
  retire a receiver count. Preserve SYNC_SP and the existing ordered retirement
  and reload path for the displaced field. The scratch TAKE choice has the same
  nullable equivalence as append; add no INIT/NONNULL or canonical grant. Only
  remove the known-NULL second retirement arm through the existing shared
  lowering. Provider, callback and error summaries remain unchanged.

No role field beyond `local_fields`, `temp_fields` and `consumed_fields` is
needed. Method output pairs do expose a missing writer path: their STORE_NEVER
format bypasses the current primary-destination `temp_fields` branch. Resolve
the fixed two-output scratch run from the checked external IO/layout and apply
its two cold vacancy proofs and two reached owner publications there. Do not
change all TEMP writes or widen SHAPE_LOCAL_TEMP admission. Phase-one's flat
consumed-source recipe also cannot certify these guarded macros unchanged:
extend its read-only prefix validation using the existing canonical-output and
explicit result-publication contracts, matching actual TAKE/OWNER statements.
Unsupported source omits the optional form; no family spelling grants ownership.

The one-word writer has two distinct LOAD_ATTR pending signatures: local B with
fresh scratch pair, and consumed scratch A=B with nonmethod oparg. Preserve each
only through its supported members; unsupported specialization or observation
restores the actual ordinary member. STORE_ATTR needs its local-A/scratch-B
signature only for SHAPE. Generate finite mappings in the same cold selector and
live replacement API; retain FT CAS and ordinary instrumentation restoration.
Pending metadata describes the ordinary member, while active metadata must
reflect reached actions and retain UNKNOWN wherever the existing graph cannot
express the publication/retirement path. Add no per-hit geometry/proof lookup.

Both record modes need reached-acquisition/rename positives and missing,
overwritten, escaped-address and shadowed-source negatives. Test both method
output vacancy proofs separately, class consumed alias without incoming vacancy,
and callback redirect after its single publication. Native cases cover four
public routes, receiver/result aliases, method guard misses, displaced-field
and old-local finalizers with mutation/GC, and monitor restoration. Native review
must price removed role branches against added text/spills; count retained stores
and acquisitions. If phase one loses, start phase two from selected57 and import
only fixed-role TAKE/OWNER lowering, optional recipe validation, the one-word
writer and private-ID admission/mapping support. Exclude its FOR/append heads,
IDs, `local_result` recipe and body-edge commit. No losing executable is a
prerequisite.

## Phase-one source qualification

Source `90cd71b4874e6229940ef7593f3f32624c52486a` contains only list/FOR and
consumed append. Root and independent source peer approve final packet SHA256
`e6a78274f8ee6f62221e8479821ecefe2aa93b5f771749c682c421de312e8260`.
Four focused host methods pass in both record modes. Changed optional recipes
omit the refinement without rejecting valid ordinary source. The reserved
emitted local `ir_old` is excluded from the rename-capable FOR recipe; an
ordinary renamed item remains accepted. No ownership evidence comes from names.

Root integrates source at `519205b2d57`, generation at `c7fecde25a4` and a
fixture-only correction at final `034f2b22f87bb4e9e54e689bb5ead8a252e60a97`.
Both builds complete 117/79 and 49 focused methods pass per role. Native review
confirms the mechanism and actual comp first hits, but the stable hot-eight
screen rejects logging 1.083074 despite comp .958819. Phase one is unselected;
see the [trial report](report.md) for all ratios, native identities and limits.
Source and native evidence remain under E59. Phase two starts independently
from selected57 and excludes the FOR/append private heads and local-result
recipe; its source implementation remains unqualified.

## Phase-two source qualification

Isolated source `c1066265668d0256f78eaf04e3894f011485184e` starts directly
from selected57. Root integrates it at `1fe09389531`. The eleven-path packet
has no generated products, list/FOR or append heads, or local-result recipe.
Final source patch SHA256 is
`374e0b26817d5ac95dad7c267adcf782567ba8dc7c113d913c0ee5073a28fc87`,
retained with the freeze manifest under `E59/attribute-role-source-host`.
Four focused source methods pass in both record modes; seven runtime methods
are AST-checked only at this gate. Root owns generation and native qualification.

Independent actual-source review approves the packet after binding a fixed
local's NULL-owner receipt to its matching pending OWNER and consuming it at
retirement. Existing rules already reject pre-retirement overwrite/address
escape. The amendment prevents a later acquired owner with the same C name from
inheriting that receipt; its later retirement remains. No selected attribute
body was shown unsafe. Source mutations cover both cases without granting a
fact from names alone.

The class fixture drops the sole external receiver reference, collects and
checks eventual release, result rooting and reentry. A class's own MRO/type
cycle makes this an eventual-lifetime control, not proof of an immediate
receiver-release callback. The displaced shape field supplies the immediate
callback/locals mutation/GC case. Independent cold no-nomination images for
each method output are not supplied; the two separate final-replay absence
checks are source-reviewed, and no malformed live-output image is executed.
Native and performance results are not yet claimed.

Root's final integration is `581c018a34a2c4d521cec789e1261a4b331c7dc8`,
including generated products and fixture-only correction `035aa32e16c`.
Both builds complete 117/79; all 54 focused runtime methods pass per role.
Independent native review and actual kernel first hits for all four bodies
pass. The balanced screen rejects comp .964506/Delta .968592 gains against
logging 1.222917, spectral 1.063393 and generators 1.017802 repeated losses.
Phase two remains unselected. The [trial report](report.md) owns identities,
fixture failures, native costs, all ratios and the decision. This closes rank
one's two runtime trials; no losing packet becomes a dependency of deferred
await or another execution design.
