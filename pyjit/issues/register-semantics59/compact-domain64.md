# Compact execution domain64

Root and frames source-reviewed architecture, 2026-11-14. Bounded implementation
authorized and underway; no native qualification. Implements only option 1 of
[parity64](../../design/proposals/interpreter-parity64.md), subject to its
128-byte simultaneous native-stack and 16-KiB hot-text stop gates. The
[review64](review64.md) history and rejected Invoke/role/engine results apply.

P's `pyjit/parity64-compact-domain` began directly from selected57
`0dcba75bb3df40c32137db453b001f7e8f101de0`; the attribute branch and its
`035aa32e16cd833ef0470b934475750a87117f93` fixture remain preserved. No losing
runtime packet is a dependency. Proposal preparation ran no test, generation,
build, target execution or native analysis. Runtime source work is confined to P.

## Physical boundary and source ownership

Generate one ordinary-C `compact_domain_run(tstate, frame, word_index)` in
ceval's translation unit and one NCSR `_TAIL_CALL_COMPACT_DOMAIN` adapter.
All active aliases point to that same adapter in `generate_ir_targets`;
they retain distinct ordinary formats and semantic mappings. Pending aliases
point directly to the existing ordinary FOR/LOAD_ATTR handlers. Do not emit
one adapter or body per path, and do not replace the selected backend.

The executor holds frame, slots and current word/index in native locals.
The first native artifact uses **no cross-word borrowed mirror**: the local
argument reload remains and earns no saving. This stays within the at-most-one
mirror bound without introducing a carry certificate before evidence warrants
it. All semantic owners remain in ordinary homes between cases. A temporary
owner between a reached acquisition and its publication cannot cross an
internal edge or helper call. The append argument is explicitly transferred
to its consuming provider, not retained as an unrooted domain owner.

Use fresh lexical scope and the existing two-pass view/owner analysis for
each operation. Factor the envelope of `emit_handler` from its reached body
emission, so ordinary and domain emission share `write_uop`, actual parsed
parts, `IRStack.consume/store`, producer contracts and their invalidators.
A small domain emitter changes terminal dispatch, pre-effect cut and final
publication only where its checked contract permits. No emitted-C reparsing,
second opcode body, generic plan interpreter or address-taken state array.

Keep declarations in the existing `ExecutionVariant`/`SlotLayout` path:
local_fields and temp_fields still mean fixed physical roles; a consumed field
narrows only the reached scratch TAKE. A finite domain-kind field selects the
target-table adapter and the matching domain case. It is not an effect or
ownership fact. A pending alias has ordinary emission and only carries its
cold signature. `generate_handler`'s two passes return the same reached
owner/canonical/error receipts for the ordinary and domain lowering, with
explicit substitutions for final publication or a pre-effect exit. The
FOR-specific final BODY action is the only window-result recipe in scope.

At each internal case retain `frame->instr_ptr = current_word`, as the
ordinary handler does. This preserves existing frame/remote/home-map
checkpoints; the first packet claims **no PC-store removal**. Keep debug
frame/executing-copy assertions and count each semantic instruction once.
No bridge arrays, value boxes, frame entry, suspension or exception handling
belong inside the executor.

## Finite entries

Proposed private IDs below are the only emitted reservations. 404–409 remain
unused. All aliases are execution-only, non-pristine, with exact ordinary
base/shadow/event/format mapping. No byte-sized wrapper stores a private ID.

| ID | Alias | Exact source/geometry |
| --- | --- | --- |
| 393 | `COMPACT_FOR_LOCAL_PENDING` | Ordinary FOR; A is a writable Python local, B is scratch window |
| 394 | `COMPACT_FOR_WINDOW_PENDING` | Ordinary FOR; A=B+2 scratch window result |
| 395 | `COMPACT_FOR_LIST_LOCAL` | Actual FOR_ITER_LIST, local recipient |
| 396 | `COMPACT_FOR_LIST_WINDOW` | Actual FOR_ITER_LIST, A=B+2 |
| 397 | `COMPACT_METHOD_PENDING` | Ordinary method LOAD_ATTR; B local, A/A+1 scratch |
| 398 | `COMPACT_METHOD_NO_DICT` | Actual guarded NO_DICT method producer |
| 399 | `COMPACT_METHOD_WITH_VALUES` | Actual guarded WITH_VALUES method producer |
| 400 | `COMPACT_ARGUMENT` | Actual LOAD_FAST_BORROW; B local, A scratch |
| 401 | `COMPACT_APPEND` | Actual LIST_APPEND; A/B distinct scratch homes |
| 402 | `COMPACT_FORWARD` | Actual JUMP_FORWARD |
| 403 | `COMPACT_BACKWARD` | Actual polling JUMP_BACKWARD |

The argument source remains LOAD_FAST_BORROW's actual admitted read/DUP, not
an inferred unbound-local optimization. FOR-local excludes hidden constants.
FOR-window is deliberately only A=B+2; arbitrary scratch relocation is not
another signature. No JUMP_BACKWARD_NO_INTERRUPT entry is added: its body may
be reused as the parsed tail of polling JUMP_BACKWARD, but await/yield loops
are not an admitted execution domain.

Select unpublished executing words during the existing common semantic
replay, **after final fixed point**, with optional executing-copy output in
`_PyCode_BuildSemanticPlan(co, &plan, words)`. Compiler, constructor, replace
and marshal share this path. Fixed field partitions are necessary, not owner
evidence. Fresh scratch outputs need individual positive NO_HOME and no
MAY_OWNER; there is no global-complete or incoming physical-NULL requirement.
Original operand admission remains in force. Unsupported source contracts
omit nomination, preserving ordinary valid synthetic analyses.

Cold pending FOR and METHOD aliases carry these single-word facts through
adaptive specialization. The source recipe and ordinary base handler, rather
than the alias spelling, authorize the eventual domain case. No adjacency,
line table, function name or benchmark identity licenses entry. An arbitrary
ordinary CALL inside an eligible CFG is still an effect cut.

## Operation and owner contracts

| Case | Reached actions shared with ordinary source | Exit before consuming / after commit |
| --- | --- | --- |
| FOR list/local | Exact-list guard, unsigned bounds, actual getter followed by XDUP, cursor increment; canonical item owner bound to current output | Type miss or exhaustion leaves untouched FOR prestate. A possibly deallocating displaced local leaves **before item acquisition/cursor increment**. Body success publishes cursor, writes B+2=NULL, replaces A and nonfinalizing-closes old A |
| FOR list/window | Same guarded acquisition and cursor conversion | Exhaustion/type miss ordinary. Body success publishes cursor and canonical owning B+2 once |
| Both methods | Existing type/version guard and managed-values-valid check where present; guarded class-value DUP, local receiver TAKE/XDUP | Every miss before acquisition leaves ordinary current word. Publish both independently fresh canonical owning outputs with write-only stores; no borrowed callable/self in homes |
| Argument | Existing local read and reached DUP | Publish one canonical owning argument to proved owner-free A. No result normalization or destination-role branch; read and count remain |
| Append | Existing actual scratch CONSUME/TAKE; original `_PyList_AppendTakeValue` | Publish append PC before transfer; item home becomes NULL exactly once. Provider consumes on success **and error**. Success continues with no live native owner; failure returns reached error exit without replaying append |
| Forward | Parsed JUMPTO with unchanged target | Target alias admitted: internal dispatch; otherwise ordinary target dispatch |
| Backward | Original polling condition and parsed backward target | Pending work leaves at the **current backward word before its ordinary poll/jump**, with homes current. Empty poll fast path takes the same target internally |

FOR's bounded window recipe is new but has a concrete consumer. Resolve
external iterator/cursor/item IO and the complete actual guard/bounds/
IR_COMPLETE_STEP/acquisition/increment source, then require the reached item
owner and canonical receipt at the final BODY flush. Changing acquisition,
exhaustion, retirement order, aliasing, address escape or an intervening effect
declines the recipe. Do not infer a tuple/list result from an assertion or
from a generic OBJECT helper effect. Cursor normalization remains ordinary;
its tagged result does not inherit the item fact. Kill the terminal item
Local after transfer; local A publication cannot claim B+2 holds the item.

The local precheck uses the actual live old home and `_PyValue_MayDealloc`
(`pycore_value.h:101`), not a guessed type/refcount. On the GIL path, between
that check and nonfinalizing close the checked getter/XDUP path cannot execute
Python or reduce the old owner's count. Acquiring an alias may increase it.
If source effects violate that interval, leave ordinary before doing work.
The nonfinalizing-close action is the existing IR_RETIRE_LOCAL action narrowed
by that reached precheck, not a dropped close or a second retirement algorithm.

Methods reuse `canonical_output_sources` and actual reached DUP/TAKE ownership;
the declaration alone seeds neither. Consume scratch equivalence only for the
actual append TAKE, as reviewed in role63: scratch OWN equals original
nullable-own-or-XDUP result, including NULL, without an additional INIT grant.
All ordinary semantic input requirements still apply. No rejected checked
descriptor entry, inline getter or inline append rewrite is imported.

The list getter remains out of line. Append success may resize but the actual
provider has no Python callback on success; failure may finalize its consumed
item. No mirror or native owner survives that call. The first domain is GIL
only. Decline FT and Emscripten entry: this avoids silently omitting QSBR or
Emscripten periodic effects when lowering the empty backedge poll. Factor the
existing atomic pending-event predicate into a shared pure inline query used
by `check_periodics` and the domain. On pending work, ordinary JUMP_BACKWARD
performs the existing full poll exactly once; its repeated pure predicate is
charged on that cold exit. No callback is serviced inside the executor.

## Entry, exit and observation

First entry requires `owner == FRAME_OWNED_BY_THREAD`, current frame/executing
copy identity, current instrumentation, and no tracing/profile/monitoring,
PEP523/value hook or LLTrace. Do not substitute “thread resident”: AWAIT and
heap-owned frames always unwrap to ordinary current execution. These gates
are charged once per adapter entry; no per-case role/proof table lookup.
Successful internal cases cannot enable observation. Every actual callback
cut ends the segment, so the next entry checks current state again.

Use exactly two machine words for the ordinary-C return:

```c
typedef struct { uintptr_t word_index; uintptr_t exit_kind; } CompactExit;
/* kinds: DISPATCH_CURRENT, APPEND_ERROR */
```

No owner, frame pointer, word pointer or address-taken state is returned.
In Py_STATS builds pack the final semantic lastopcode into unused bits of
exit_kind and restore it in the adapter; do not add a third word or count the
adapter as another semantic instruction. Count/miss controls must distinguish
an unexecuted fallback word from a completed internal instruction.

DISPATCH_CURRENT resolves the semantic index against the pinned frame's
current executing base, reloads slots/word, and uses the finite generated
`compact_ordinary_handler(id)` mapping to tail-transfer directly to the
ordinary handler. Preserve the original fields in ir_word. **Never dispatch
through the still-private table entry on exit**: list exhaustion, type misses,
last-owner local fallback, pending poll and denied entry would recurse back
into the adapter. Pending aliases likewise unwrap to ordinary generic heads.
CALL and other unsupported targets need no map entry: ordinary dispatch at
that target retains all existing guards, normalization, roots and cleanup.

APPEND_ERROR returns the saved numeric append index after its provider has
consumed the item. The adapter uses the existing `value_operation_refresh`
and actual error-origin contract, then the existing `_PyEval_ProcessError`
path with NULL bridge record. Resolve against current executing storage after
refresh; preserve the operation exception and any source-error recovery.
Do not read the spent item, reexecute append, or use a cached provider-era PC
as a validity certificate. The resulting handled dispatch/error unwind is
the existing macro behavior, outside the ordinary-C domain. Any unsupported
refresh disposition fails closed rather than resuming the internal loop.

Keep instruction observation exact by denying/revoking aliases before its
wrappers execute. Debugger redirects, new monitoring, profiling and cross-
thread inspection see ordinary current semantic PC and all owning homes.
No private native address or return record enters public frame state. The
domain neither activates awaits nor performs terminal relocation.

## Writers, metadata and file closure

Frames confirmed the shared interfaces: `int _PyIR_ReplaceHandler(co, words,
i, handler, specialize)` and optional executing-copy semantic-plan output.
Live specialization/deopt and instrumentation writes use that central API;
retain FT compare/exchange and instrumentation-race behavior even though FT
does not nominate this unit. Single-word mappings retain the pending signature
only through supported family members. Unsupported members restore ordinary
identity. Instrumentation, wrapper restoration and execution-copy rebuilding
do not manufacture a new positive vacancy proof; cold replay can renominate.
There are no predecessor searches, reverse maps, extra exec fields or per-hit
geometry rescans because the first packet has no cross-word mirror.

Each alias keeps the ordinary semantic action metadata. Reached domain-specific
store/consume receipts are checked against those ordinary outcomes; do not
export a stronger graph just because native lowering has fewer instructions.
FOR BODY A publication remains conditional; retain UNKNOWN when existing
graphs cannot express it. Source-derived callback, allocation, release, poll,
error and synchronization properties remain truthful. Private IDs reject on
all public image routes and are exported to introspection as execution-only.

| Source | Concrete change |
| --- | --- |
| `Tools/cases_generator/ir_generator.py` | Finite declaration table, shared handler envelope/body factor, reached source recipes, two-pass domain emitter and target-table mapping |
| `ir_metadata_generator.py`, `ir_numbers.py` | Ordinary/pending/active mappings, exact IDs, private export, honest effects and generated selector; no unused handlers |
| `Python/ceval.c`, `ceval_macros.h` | Executor include and single NCSR adapter; reached error exit reuse; shared pure periodic predicate and fixed local commit action |
| `Objects/codeobject.c`, `Include/internal/pycore_ir.h` | Final common replay nomination with optional executing copy, public-ID denial and common writer declaration |
| `Python/{ir,specialize,instrumentation}.c` | Cold selection before feedback early return; finite one-word live writer, restoration and FT race closure |
| `Modules/_opcode.c` | Execution-only introspection export |
| Focused source/runtime tests | Actual body/receipt equivalence, alias mapping and owner/error/observer behavior |

Root owns generated cases/metadata/targets and native qualification. Ordinary
body generation must remain byte-equivalent apart from deliberate shared
primitive factoring. No frame layout, provider, calling convention, flags,
public semantic encoding or benchmark change belongs in this packet.

Selected source anchors (I at the pinned head above):
`ir_generator.py:168,2973,3002,6503,7046,9010` for variants, consume/store,
two-pass body and target emission; `bytecodes.c:1239,3471,4079,4099` for append,
edges and both method bodies; `generated_ir_cases.c.h:14766,18076,18227,18990,
20285` for the actual FOR, poll, append, method and argument envelopes;
`ceval_macros.h:119,456,570,588` for reached error, poll, write-only canonical
publication and local retirement; `ceval.c:1350,2244` for refresh/error ingress;
`ir.c:455` for executing-copy construction. These are inputs to the source
peer, not evidence that the proposed native executor meets its gate.

## Cost worksheet and gate

Retained comp has 3,348,000 candidate non-CALL visits of 13,674,014 visits;
these are monitored ceilings, not admissions or removed work. Per successful
mapping iteration compare the complete append→backedge→FOR→method→argument→
ordinary CALL path, plus first-entry and exhaustion separately.

| Cost | Selected | Proposed / measurement obligation |
| --- | --- | --- |
| Native boundaries | Five separate non-CALL NCSR envelopes around ordinary CALL | One NCSR adapter→ordinary-C entry/return on each segment; CALL unchanged |
| Internal dispatch | NCSR table dispatch + five-argument ABI packing | Finite C dispatch + word decode; count actual indirect/conditional branches, never equate syntax with removed jumps |
| C-call VM state | Getter/append may spill ABI state | Ordinary-C callee-saved residence hypothesis; saves paid per segment, report every spill and both C calls |
| Owners and storage | Item, method, self, argument acquisitions/stores; cursor update; append consumption | Required counts/stores remain; local/window choices specialize, FOR local staging disappears, one B+2 clear remains |
| PC/local read | Per-word PC stores and argument home reload | Both retained in this first artifact; zero credit |
| New gates | Ordinary handler guards | Entry observation/residence checks; local MayDealloc precheck; alias dispatch/unwrap; charged on full path |
| Poll/error | Ordinary polling and actual error ingress | Empty predicate internal; pending poll and consumed-append error leave once, with adapter cost charged |
| Text | Ordinary bodies only | Ordinary bodies plus one finite domain; include compiler-created hot fragments and mapping tables |

First native gate: adapter and executor **simultaneously live native stack at
most 128 bytes**, including saves/spills (report explicit allocation and saved
registers separately); hot domain text including fragments at most 16 KiB.
Require a two-register return, no repeated frame/slots/PC spill cycle and net
instruction reduction over complete entry→CALL and append→CALL paths. Retained
getter/provider instructions and generic CALL are included on both sides.
Count successful and fallback local-replacement paths and exhaustion. If this
small roster cannot meet the gate, stop: no mirror framework, arithmetic,
provider rewrite, alignment, flags or opcode expansion rescue.

This has no parity budget. At 108 iterations/tray, an illustrative 20 net
instructions saves 2,160 against the separate ~84,651-instruction deficit,
about 2.6%; it is not a measured saving or a bound. Root needs useful natural
domain entry/length/cut counts and unchanged broad sentinel outcomes even if
the tiny native mechanism passes.

## Minimal evidence

Host source checks in both record modes: exact ordinary/domain body and
owner/error/action comparisons; changed guard, missing DUP/TAKE, overwrite,
address escape, callback/control and renamed-producer controls; optional
recipe omission; real metadata/target generation including holes and shared
adapter targets. All aliases must unwrap without adapter recursion. Verify
statistics and private/base/shadow/effect fields, not formatting alone.

Root runtime minimum: one loop covering all admitted operations, a forward
edge and both FOR outcomes; local finalizer forcing ordinary fallback before
acquisition; int/float/mortal/alias items; both method layouts and guard misses;
append capacity/growth/OOM consuming exactly once with finalizer reentry;
poll enabling observation and redirect at a cut; every private entry from
outside, independent fresh-output negatives, every covered-word rewrite,
monitor enable/disable/restoration, and all four public image routes. No
malformed image execution. Include ordinary heap/AWAIT-denial and generator
controls without importing await activation. Recommend the new focused module
plus semantic completion, window transfer, operand modes, transfer admission
and native owner effects; no automatic broad call-suite expansion.

Peer the actual source and whole generated path before root's tiny native
build. Passing those gates permits the unchanged hot8 screen, not selection.
Stop on repeated sentinel loss and preserve the complete artifact.
