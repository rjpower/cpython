# Rank 1: fixed storage-role lowering

Source-reviewed unit, 2026-11-14; phase 1 implementation authorized. Root and frames independently approve the direction. The FOR item must remain BODY-only in exported state; unsupported conditional graphs remain UNKNOWN. Consumed roles only narrow an actual reached TAKE/owner transfer. No implementation or native qualification is implied by this review. Read against the current [grouped design](../../design/proposals/interpreter-parity63.md) and [review63](review63.md). Selected source is `BASE/worktrees/call55-integrated` at `0dcba75bb3df40c32137db453b001f7e8f101de0`; canonical checkout runtime is not the source baseline.

P is clean on new branch `pyjit/parity63-role-lowering`, created directly from selected57. The prepared branch remains at `6e07c1a89532fde13e4a6ca96fef9c37f96cf566`. No round59–62 runtime packet is a prerequisite. No production source, generated product or executable was changed or run for this proposal.

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
- Consumed scratch inputs require readable current owners and the reached consuming action; require distinct input roles except the explicitly checked class A=B result alias.
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
