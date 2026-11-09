# Compact executable IR and source metadata

Revised proposal, 2026-11-09, against runtime `4b3d739d867`. No runtime edits
or native jobs. This revision replaces the initial over-preserving design:
there is **one public instruction-offset domain**, the compact real program.
Removed NOP/POP operations have no instruction event, adaptive handler or result
recovery home. Do not preserve a dead stack program in metadata.

## Decision and API basis

The canonical program is a compact array of semantic register instructions.
Source-only line events and cleanup recipes are metadata on semantic control
transitions. Branches/continuations address compact instructions. f_lasti,
traceback instruction offsets, dis, instruction monitoring and co_positions
keep that domain. Ordinary handlers acquire no new source cursor lookup/store
and do not interpret metadata on dispatch or edges.

The official [monitoring API](https://docs.python.org/3/library/sys.monitoring.html#sys.monitoring.events.INSTRUCTION)
defines instruction events around actual VM instructions; removed operations
need no event. [PEP 626](https://peps.python.org/pep-0626/) separately requires
accurate source-line tracing, including consecutive passes and backward edges.
The [data model](https://docs.python.org/3/reference/datamodel.html#codeobject.co_lines)
permits zero-width co_lines ranges for eliminated source lines. Thus source
metadata is needed, not a second public instruction graph. Zero-width ranges
alone are not a dynamic schedule; edge-specific records preserve path/order.

Remove the three bool-skip aliases and checked-NOP fallback, the two Python
discard aliases, IR_POSTCALL_DISCARD and post-return POP decoder. Assess the
independent shape operand mode on its own cost, without multiplying boundary
variants. Python does not require prompt finalization or historical destructor
PCs. No benchmark names or workload guards enter runtime decisions.

## Source facts

_PyRegOp already separates source location/exception block from word/nwords
(Include/internal/pycore_regcfg.h). regcfg_passes.c:531 manufactures NOPs when
deleting producers/POPs; :1191 retains PUSH_NULL line anchors. Replace those
with source records. assemble.c:334 sizes locations by executable width and
:166 builds exception ranges similarly. Keep ordinary tables in compact-PC
coordinates, adding zero-width source records and sparse transition data.

assemble.c:1307 uses exit+1 in relaxed loop layout; bytecodes.c:335 gives
END_FOR a stale-PC policy used by POP_ITER. Replace those assumptions with
explicit loop/send completion targets/cleanup. _PyInterpreterFrame_LASTI
(pycore_interpframe.h:20) remains pointer subtraction; instrumentation patches
actual words. frameobject.c:1648's stack-effect scan needs register-state and
source-resume maps, not a second public offset type. Existing PyFrameObject
f_lineno overrides used in legacy_tracing.c and instrumentation.c can describe
source-only callbacks without adding a hot interpreter-frame field.

## Minimal immutable metadata

- Existing source/position ranges indexed by compact PC, with zero-width
  co_lines entries for eliminated source lines. co_positions still describes
  actual instructions only.
- Sparse source-transition records keyed by semantic source PC and edge kind:
  entry, fallthrough, taken, call return, loop/send completion and suspension.
  Records contain ordered source spans/line events, callback exception context
  and a real resume target/state. They are not virtual opcode lists.
- Register/debugger state at admitted line-jump/resume points: occupied owner
  homes and exception state. Share states/sparse deltas; do not store a full
  slot bitmap for every source column.

Compose records as empty CFG blocks disappear. They must be edge-specific:
attaching all eliminated lines to the next real PC emits lines from unrelated
predecessors. Untaken edges emit no events. Backward-edge line events survive
even when the line is unchanged. Preserve real scheduling polls and explicit
with/finally/exception-context operations; those are semantic execution.

## Observed execution without a hot metadata tax

Normal branches/dispatch use compact words and direct targets only. They do
not look up transition records, increment an event cursor or test for NOPs.
Instrumentation patches semantic handlers into observed forms that process
records for the actual edge. INSTRUCTION events occur only for real words.
There are no dependent empty-boundary aliases to invalidate.

The observed engine owns its ephemeral cursor. During a source-only callback,
use the existing frame-object f_lineno/source-span override; f_lasti remains
the last actual instruction offset, or -1 before first entry. Advance the
cursor before invoking code, check redirects/version afterward and restore
the source override according to the committed state. Preserve the correct
last executed source line on suspended/completed frames, including a trailing
eliminated pass. No ordinary-handler cursor updates are introduced.

A callback exception uses the transition record's exception context, not the
preceding/next word's range. Preserve its explicit traceback source line/span
where the compact PC cannot identify it; tb_lasti stays compact. Nested
callbacks save/restore overrides and cannot replay a partly consumed record.
This is observer error handling, not an ordinary per-operation table lookup.

f_lineno assignment selects an admitted source resume state, validates live
register/exception compatibility, then installs the real PC and an observed
source cursor only if needed. A redirect abandons the old transition. Do not
resurrect a discarded result or old stack depth to make an obsolete POP
jumpable. Reject incompatible states with ValueError and document deliberate
changes to CPython-only acceptance rules separately from language semantics.

Monitoring enabled inside a callee/finalizer affects the next supported event.
Frame transitions perform the existing epoch handoff or choose an observed
continuation; they do not inspect a future POP/NOP word. Enabling observation
inside result disposal creates no retroactive POP event: it is no longer a VM
instruction. Frame worker and instrumentation owner must agree this handoff.

## Results, owners and suspension

Lower unused call results directly to plain IR_NO_DST, across callee kinds and
source-line boundaries. A continuation stores real resume PC and result
recipient separately from source data. Hold one owned result across safe
child retirement, then publish or dispose of it. No tag, live-word decoder or
hidden original-result recovery home remains.

Extend dead-result sinks to other single-result producers. Pure total work may
disappear; potentially raising local reads/attribute operations still perform
their required checks/effects. A bare unbound local expression must still raise.
Retire outputs in a semantic producer/consumer or bounded region/frame cleanup,
not a dispatched release. Slot reuse accounts for its previous owner; do not
accumulate an owner list on each loop iteration.

END_FOR/POP_ITER and END_SEND become loop/send completion work, including
receiver/iterator release and stop-iteration/branch events. POP_EXCEPT restores
semantic exception context and remains real work despite its name. Other
cleanup-only POP_TOP/clear words become transition cleanup. Release branch-local
owners only on their owning edge. GC sees canonical owners/NULL or an explicitly
specified virtual binding, never stale owning bytes treated as dead by convention.
Delayed cleanup is language-permitted but needs bounded retention/GC accounting.

Generators/coroutines save real resume PC and semantic phase. Throw/close and
completion no longer assume adjacent cleanup words or exit+1. Remote/debugger
offsets stay compact; source-only overrides need documented slow readers.
Specialization/deopt changes handlers without changing PCs. Future JIT recovery
uses semantic PC/phase and owner maps, not invented release instructions.

## Tools, persistence and implementation split

dis displays real instructions, optionally annotating source-only ranges; it
does not synthesize POPs. Marshal, construction/.replace, equality/hash and
copy APIs serialize source-transition/state metadata with compact words. Validate
targets/ranges/states and advance cache magic. Changed co_code must not silently
retain incompatible maps. Synthetic code without maps gets a validated trivial
map with no source-only transitions, not an old executable-stream fallback.

IR worker owns regcfg sinks/cleanup/empty-block composition, assembler layout
and source-record schemas, persistence, verifier and disassembly. Frame worker
owns guarded binding, continuation/result ABI, retirement/virtual locals and
suspension mechanics. Coordinator owns instrumentation and frameobject.c
line/debugger consumers unless explicitly reassigned. Agree generated hooks
and headers first. Frame worker supplies kwargs-materialization helper calls
to coordinator rather than concurrently editing frameobject.c.

Admission asserts actual executable arrays contain no NOP, POP_TOP or
cleanup-only aliases and every INSTRUCTION event identifies a real word. Test
source lines through empty/multi-predecessor blocks, callback errors/jumps,
late enablement, unwind/finally, suspension/throw/close and cache roundtrips.
Inspect generated release code for absence of ordinary metadata processing.

The measured logging gap is about 5,617 instructions per iteration. Empty
boundaries alone cannot credibly close it. Entry, frame clearing, method lookup
and return remain the main measured areas. Compaction enables the simpler frame
model; matched counters and elapsed time must establish logging parity and
repair repeated broad regressions before performance promotion.
