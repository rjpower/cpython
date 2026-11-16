# Execution72 carried-owner ABI: exact first source unit

Reviewed source contract; independent peer and root implementation GO received
on 2026-11-15. Based on selected57
`0dcba75bb3df40c32137db453b001f7e8f101de0`. Root owns implementation release,
integration, generation and all runtime/native qualification. P implements on
`pyjit/execution72-carried-owner`; W on `pyjit/execution72-compiler-abi-selected57`.
Both start directly from selected57. The cold-map unit lost its performance
screen and is excluded. This ABI retains existing reserved NULL homes, windows,
initialization and physical allocation, and materializes before effects; both
source owners confirmed that it does not need nonprefix exception maps. It also
excludes rejected71 canonical propagation and hot/cold arithmetic.

## Fixed representation

Keep the existing configured handler argument count and calling convention.
Rename the fifth scalar to `_PyIRPayload` (`uint64_t`, the existing packed-word
and PyValue-bit width, including on 32-bit hosts). Its interpretation is
**static in the native target**:

* WORD entry: packed `_PyIRWord`, exactly today's operand source;
* ACC entry: the bits of one canonical **owned** `PyValue`; the entry loads its
  semantic word from `next_instr` before its usual entry advance;
* a private effect/resume entry declares one of those same interpretations.

Named conversion constructors/accessors express the two meanings; there is no
runtime tag or test of value bits. Py_STATS retains its existing sixth
`lastopcode` argument. Ordinary dispatch still loads the whole next word and
selects its existing low16 handler. It passes that word for WORD targets or the
carried owner for a statically selected ACC target. No universal `IR_SLOT`
sentinel branch, extra ABI register, global dispatch AND, or new frame pointer
is introduced. ACC entry reloads its own word: that extra load/register pressure
is charged. Same-site deopt, instrumentation and pair accounting retain the
same PC and input mode; they cannot reinterpret ACC bits as a word.

Cold specialization and instrumentation writers preserve that placement even
when a child is running or a generator is suspended. LINE/INSTRUCTION monitors
use finite generated wrappers nominated by immutable site mode and monitor
opcode. The sparse nomination row is three uint16 fields (6B), with two rows
per admitted site mode; event-specific instrumented counterparts are also
generated before compiler selection. A wrapper retains its static payload
meaning, spills before invoking hooks, then executes the materialized underlying
operation while preserving its output placement. Ordinary dispatch and normal
completion do not consult this cold nomination table. Charge all wrapper text,
table rows, cold spills and mode-preserving specialization refusals. Root must
verify changes made while an ACC continuation is already armed, including
monitor removal and hook errors, rather than only instrumenting before entry.

The compiler keeps a normal allocated home for each initially carried logical
value. Its operand fields continue to name this **cold spill home**. The home
is reserved for the full owner lifetime and physically NULL while ACC owns the
value. The supported ACC native body statically reads/takes the argument instead
of that home. It never tests the index at runtime. Thus no initial frame-size,
NULL-initialization or slot-limit saving is claimed. One logical owner is either
carried or in that home, never both. Existing canonical bits are the ownership
representation; no borrowed result is admitted into the accumulator.

Replace the raw uint16 `return_slot` field's meaning with an explicit uint16
`_PyIRContinuation`: index12, kind2, reserved2 zero. Kinds are VACANT_HOME,
LOCAL_REPLACE, DISCARD and ACC. Keep `return_pc` unchanged. VACANT includes the
interpreter sentinel's known result cell; LOCAL applies only to bindings;
DISCARD has canonical index0; ACC names its reserved spill home. Constructors
are typed (`Vacant`, `Local`, `Discard`, `Accumulator`), not tests at every
return. No representation aliases IR_NO_DST with ACC. Frame layout keeps the
same 16-bit field and alignment; the actual offsetof/sizeof receipt is required
before claiming no growth. All old raw field readers/writers are ported.

Keep the eight-byte packed-word encoding. Add a dense uint16 **primary continuation map**
to executable preparation, indexed by semantic word. Its meaning is fixed by
the source contract: CALL/implicit child normal result; SEND completed result;
YIELD resume input. It never means SEND's yielded result. Non-provider rows
have a canonical invalid value. `_PyCodeExec` gains one pointer (+8B on target),
and its checked allocation appends `2*nwords` bytes after words and feedback;
the current ordering needs no extra alignment padding. Charge all words,
including nonparticipants. No extra frame
cache pointer. Binding loads the code/exec pointer and indexed uint16 row once
at the activation edge. This is a scalar load, not a descriptor interpreter,
search, per-return semantic opcode decode or per-op EH-map lookup.

The common builder derives kinds from admitted physical operands plus the
certified ACC edge. Local/vacant/discard classification is done at preparation,
not rebound dynamically from nlocals on every return. `_PyIRStaticExec`,
interpreter entry, init-cleanup trampoline and public frame constructors use
explicit typed constants. Allocation/free/sizeof and OOM cleanup cover the map
in the same executable allocation; no separate allocator family or singleton
indexing change. The map contains scalar indices/kinds, no borrowed pointers.

## Static lowering and finite first slice

Use existing logical IDs, owner-held use/def information, source IO roles and
effect facts. Select a carried edge when its producing edge has exactly one
owning consumer, the consumer's source role is supported, its cold home remains
reserved, and every intervening edge preserves that identity. Same physical
index alone is not identity. A join can carry a value only when every normal
predecessor supplies an independently proved owner at the same reserved home
and consuming input role. Multiple uses, incomplete ingress, unsupported read
roles and unknown effects materialize. Do not create SSA or another lifetime
engine.

Generate placement variants from one parsed handler and Storage/Local model.
An execution variant specifies at most one ACC input role and an ACC output
role, plus WORD/ACC payload entry and ordinary counterpart. Existing home
helpers and every unselected operand remain direct. A missing variant declines
selection before code emission; it does not add a dynamic runtime fallback.
Public images validate producer, consuming edge and mode together.

Initial supported source roles are:

* ADD/SUB single result, generic provider and exact-inline-int fast arm;
* LOAD_COMMON_CONSTANT owned result, including the initial None input to SEND;
* RETURN_VALUE/RETURN_VALUE_EMPTY `retval` (canonical owner passed into the
  existing WithResult retirement root);
* ordinary CALL normal result, including exact Python activation and the
  generic CALL provider fallback; unsupported adaptive providers stay in the
  same ACC-capable generic family rather than nominate an incompatible head;
* SEND's `v`, yielded value and completed result as separate edge identities;
* YIELD_VALUE/YIELD_DELEGATED outgoing value and resume input, separately;
* transparent control along these edges only where the source-derived fast
  path is non-reentrant; RESUME/backedge pending work goes through an effect
  entry before any callback. A transparent jump must have one proven owner on
  its selected edge; it is not a blanket pass-through permission.

The independently reviewed carrier amendment uses input role `CARRY=254` and
output edge `PRESERVE=16` in the existing eight-byte mode row. A carrier forwards
the same logical definition; it neither creates an owner nor consumes a Python
operand. Its variant's otherwise-unused B field has the distinct typed
`CARRY_HOME` role and names the reserved scratch spill home. Initial audited
bodies are RESUME (oparg A) and JUMP_BACKWARD_NO_INTERRUPT (target CD). There is
no previous-PC inference, new frame field or extra ABI argument. The compiler
stores one carried logical ID per participating operation, maps it through
existing allocation, and requires one definition per incoming edge and one
eventual owning consumer. The public validator checks B's bounds and agreement
with the producer; source/debug operand decoding retains the ordinary instruction's
meaning. Register actual carrier homes in the existing Storage graph; require
their physical NULL state, not just vacated ownership.

RESUME's fast path retains its real tool/poll gate. A reached effect spills B,
clears ACC_LIVE and enters the body-only WORD continuation without repeating
stats, guards or polling. The first implementation keeps that value materialized
through explicit WORD counterparts until its owning consumer; it does not
retake old B after a callback. Those cold counterparts select materialized
targets explicitly, so ordinary dispatch never passes WORD bits to an ACC
target. They preserve capability for newly produced outgoing values to carry
again. Charge this cold query/dispatch chain and its initialized-control owner
views; an immutable ACC graph cannot describe a now-homed owner. Same-PC return
alone grants no old continuation authority. The no-interrupt jump carries
directly. Preserve
ACC_LIVE across a statically known carrier chain without redundant writes.
STORE_FAST is an admitted owning consumer for the ordinary YIELD → RESUME →
local-binding path, using existing local publication/displaced-owner semantics.
This amendment closes actual generator control flow; tests and native receipts
must demonstrate it rather than treating an enum or variant as a reached hit.

Selection collects candidate producer-to-consumer edges before changing any
opcode. It then removes unavailable or conflicting native modes and incomplete
ingress groups until no further candidate is removed. This closes the initial
None and resumed-value paths of a SEND/YIELD cycle together. Different incoming
values may share a consumer, as at an ordinary control-flow join; each edge must
prove its own owner. Entry and exceptional ingress remain homed. No ordinary
dispatch lookup discovers a provider or repairs a partial group at runtime.
Tests must reject a missing predecessor, an absent producer output and an
incomplete group. Physical suspension snapshots precede the logical result of
the next activation: a waiting frame has neither that future owner nor live ACC.

The selection is reusable for any supported producer/consumer pair. Tests must
include more than named ADD-to-RETURN motifs. The next bulk port would add
list-append, compare/branch and arithmetic **input roles** to the same placement
lowering, not invent another ABI. Those roles and comp/Delta coverage are not
claimed in this first unit. Freeze the actual generated variant count and
added text before runtime release; no Cartesian alias matrix or all-handler
consumer clones are authorized.

## Direct consumption, one ordinary dispatch

Every normal producer supplies one canonical owned result to its actual
continuation. A common completion lowering switches on the typed continuation:

* VACANT: write the known vacant cell, transfer owner, clear local ACC;
* LOCAL: capture old binding, publish new binding, then close old;
* DISCARD: consume/close the incoming owner;
* ACC: keep the owner in the fifth payload and dispatch its actual ACC-input
  consumer directly.

There is no mandatory B0 publication, separate POP/STORE opcode, generic adapter
hop or second dispatch. Home/local/discard continuation consumption is generated
in completion as today's publication is; its branches, masks and code size are
charged. ACC source selection guarantees the actual next target accepts ACC.
Existing direct argument binding and overlapping CALL windows remain.

Close paths are effects, not hidden returning C edges: a nonfinal close uses an
executed `_PyValue_TryXCloseNoReentry` with a proved success condition and a debug
assert on the saved boolean. A potentially finalizing LOCAL/DISCARD transfer
tail-enters its generated close-effect continuation **before** mutation/close.
It publishes the replacement before closing the displaced binding and roots
all remaining owners. No restart after any publication or decrement. No claim
of a leaf native handler follows until emitted code confirms it.

The exact finalizing-close ingress carries the **incoming owned value as ACC
payload**, not an invented reserved scratch recipient. Before any callback the
cold native entry pushes the existing 40B EXTERNAL result root with that value.
For LOCAL, the old owner is still in its binding on entry. After pending
redirect handling and fresh continuation/PC selection, a callback-free commit
captures old, publishes incoming to the actual binding, and replaces the root's
result owner with old; then it removes that result from the root before the
actual consuming close. The intermediate root store may be elided when the
entire exchange and detachment are callback-free. A traversable root must not
retain the consumed pointer during deallocation. DISCARD similarly removes and
consumes the rooted incoming owner. No frame home is claimed to root the argument before that cold entry
pushes its record. An alias of old and incoming still represents two existing
references; no new duplicate is made. Nonfinal LOCAL admission examines old
before mutation; successful publication followed by executed Try is guaranteed
non-reentrant. A pending redirect itself tail-enters this rooted cold ingress
before `_PyFrame_CompleteRedirect`; continuation/index/PC are reloaded afterward.
The incoming result is never read from an old already-drained home.

Ordinary Python RETURN still unlinks the real child, credits recursion once and
uses `_PyEval_FrameClearAndPop[General]WithResult`. That existing root carries
the owner across teardown callbacks. Only after retirement does direct typed
completion consume it; callbacks may have changed the parent's continuation,
so reload the actual field/PC then. Implicit LOAD_ATTR property/getattribute,
SUBSCR_GETITEM and init-shim children use typed HOME/LOCAL/DISCARD bindings too.
THREAD owner alone is not a CALL discriminator. Interpreter exit consumes ACC
directly where its generated target supports it; otherwise its explicit vacant
cell remains a priced boundary, not a fictitious frame-size saving.

The generated ownership graph ends the child at the actual rooted-helper
handoff. Its terminal checkpoint requires an empty ACC; it grants no parent
home or result authority. The existing logical EXIT transfer describes the
child's end. A graph captured after switching frames cannot be presented as a
normal snapshot of the retired child.

Python return must not acquire the native call's periodic poll. Native provider
calls retain their existing reached retirement/poll order and get their result
continuation once. No replay of provider, setter, poll, monitor or stats.

For the generic native CALL provider, a source-selected `CALL_ACC_RESULT` flag
requests an owned-result return. Bind the primary continuation and return PC
before provider/monitor entry. Keep the result in the existing EXTERNAL root
through input retirement and native end polling, including LOAD_SPECIAL's
poll suppression. Successful status 2 returns that owner in the same union
word used for the child pointer; `_PyCallResult` remains at most 16B on the
target, rather than gaining a third field and a hidden return buffer. The
generated consumer uses the actual refreshed continuation and PC through
`IR_COMPLETE_CANONICAL`. An unchanged ACC landing dispatches directly; a
redirect, local or discard follows its existing typed completion route. There
is no destination publication followed by a retake. Error paths retain their
existing rooted consumption. Ordinary providers retain their existing result
path; the flag is selected by source lowering, not a per-return mode lookup.

## Three distinct generator continuations

1. **SEND yielded result:** before entering the child, bind the parent to
   `Vacant(base+2)` or `Accumulator(base+2)` according to the SEND execution
   variant's **yielded edge**. This is a source-declared constant kind plus the
   existing base index, not the primary map row. Receiver/cursor remain owners;
   yielded value is a new owner. `return_pc` is the actual yielded successor.
2. **SEND completed result:** on child return, the existing completion
   transaction owns the returned result and detaches/drains receiver/cursor and
   sent-input window. Its primary map row describes A at the actual completion
   edge. After all callback-capable drain/poll work it reloads actual frame,
   PC and continuation, then supplies the held owner to that edge. Never use
   the yielded-edge index/kind for StopIteration.value.
3. **YIELD resume input:** suspension stores the primary map row of that YIELD
   site as the generator's incoming continuation. Outgoing yield and incoming
   sent value have different logical identities. Resume claims state, publishes
   real frame/exception links and debits recursion exactly once before an
   input-close callback. A compatible ACC continuation receives the input
   directly; a home/local/discard continuation consumes it directly. Failed
   entry owns/closes the unused input once and does not publish a fictitious
   successful value.

SEND completion retains the existing 16B step-result ABI: its pointer word is
a union of the next instruction and an owned result, selected by an explicit
successful-result status. An unobserved carried completion roots that result
through input retirement before returning it to typed completion. PRE
observation sees the still-owned input window and a rooted result. A reached
POST observer instead receives the ordinary committed state: transfer the
result to A and detach it from the external root before calling the observer.
Continue through explicit materialized WORD counterparts afterward; do not
retake A based on an unchanged PC. The physical post view must describe the
homed value even when the immutable word selects an ACC output. Redirects,
including same-PC generation changes during observation or input finalization,
use the actual landing and cannot deliver an old held result to a stale ACC
consumer. Errors consume the held result once or leave its materialized owner
to the reached cleanup recipe. Input finalizers are observer boundaries too:
execute the existing non-reentrant close primitive in original retirement
order, and materialize before the first close that may reenter. Test each owner
at its actual turn, so aliases need no speculative all-input finality proof.
Charge these close gates and the cold materialization path. This is
source-contract approval; runtime and native qualification remain required.

Yield commits suspended state, real resume PC, exception/recursion unlink and
external detach before publishing its owner to the parent. The final native
GENERATOR_YIELD/RETURN classification is after the last publication callback.
Suspension has no live accumulator owner: the outgoing owner has left the
generator, and the future resume-input owner does not exist yet. Its reserved
incoming home is NULL and its typed ACC continuation merely names that vacant
future spill location. Clear ACC_LIVE in the suspended commit and publish the
ordinary suspended HomeView. In particular remote asyncio delegated traversal
must decode the continuation's index and validate its kind/source edge; raw
return_slot bounds, IR_A equality and receiver-inequality tests cannot inspect
the packed continuation as an index. A suspended ACC continuation is not itself
an unavailable live-register state.
Generic send providers and throw completion use the same typed successful edge
after their existing once-only transaction, not the other branch's outputs.
This keeps all real frames and every per-member operation; no retained-prefix
shortcut or Python prewalk is proposed.

Native generator send is a cold ingress. Preserve current setter-before-hook
semantics: duplicate the borrowed input once, consume/materialize it into the
actual typed incoming home before PEP523 selection, and root it across early
C-recursion failure. For ACC this is its reserved vacant spill home; the
private default start entry takes it once after valid entry and continues with
ACC. This external boundary pays a home round trip. The nested SEND fast cycle
does not. Created-frame unused None and throw/close are explicit discard/error
entries; arbitrary public nonzero throwflag keeps its existing exception
meaning. No reserved public integer discriminator or hook bypass.

## Effects, roots and cold recovery

Add an explicit audited source effect cut to the existing parsed emitter,
declaring NONREENTRANT, ALLOCATE, PYTHON_CALL, FINALIZING_CLOSE or SUSPEND. Unknown
calls conservatively materialize. Reuse existing Storage owner/partial-output
tracking and phase rows at each reached cut. The cut is not a helper-name
allowlist or a new arbitrary-C proof system. A source audit establishes the
helper preconditions; metadata cannot grant a whole-handler owner/error fact
merely because one result is canonical.

Before a callback, GC, allocation path with possible observer entry, monitor,
pending work or error transition can run, move ACC to its reserved NULL home
and set the C variable NULL. Publish the **reached** ordinary HomeView/PC, then
tail-transfer to a private generated effect entry using the same configured
ABI, WORD payload and a named source continuation. The effect entry starts at
the reached cut, not at opcode entry: no repeated stats, pure arithmetic,
guards, inputs, monitor event or partial take. Its output can later be taken
back from the current home only if fresh actual frame/PC/phase still grants
that continuation. An unchanged pointer alone is insufficient after callbacks.

A split admits **all** native live state at the cut, not only ACC. Every other
owned C local must already be transferred to an authoritative home/root; every
borrow/scalar/pointer must be reconstructible from the admitted current words
and homes without repeating an effect or acquisition. No C local, bridge mark,
conditional ownership state or address may be implicitly carried in a hidden
continuation record. If this fails, materialize ACC and retain the existing
ordinary body at that operation; charge its returning C edges. The first
adapter splits therefore use only audited cuts with this complete property.

Ordinary effect return uses the actual logical continuation. A pending redirect
goes through existing completion after materialization; an already applied
redirect owns its actual landing state and old homes cannot be read/cleared
again, including same-PC f_lineno generation changes. Abnormal provider/poll
results enter the actual error checkpoint and never publish a successful ACC
output. Existing exception/home maps consume the materialized reached layout.

Existing 40B transition result roots remain for held C results during frame
retirement and effect adapters. No duplicate frame edge or new global root
registry. At ordinary checkpoints the frame owns the materialized value;
takeover/copy moves it with other actual homes. Cold error, frame getters,
locals/debugger writes, trace/profile/evaluator changes, throw/close/finalize and
GC entry must all reach this publication boundary before accessing its state.

ACC_LIVE is an explicit phase in the existing phase-state word, with the cold
spill index derivable from the admitted ACC entry/site. It grants no HomeView
authority for the missing owner. Synchronous callbacks cannot occur in this
phase. Async readers may still display the real frame/code/PC if their API
needs no homes, but a reader needing complete owners must explicitly refuse an
unmaterialized ACC phase; stopping threads does not manufacture its owner.
No stale home fallback. First source/runtime activation is GIL-only; FT code
selects no ACC placements and retains ordinary reads/acquisition, and all new
schema/field constructors remain valid there. Remote/debug-reader schema and
explicit refusal require root assignment and closure before enabling ACC code.

## Source boundaries and staged commits

Compiler worker owns ACC edge selection and
common validation in regcfg/assemble/codeobject, executable primary map lifetime
in ir.c, cold specialization/instrumentation writers and compiler/public-image
tests. Backend owns generator parsed placement
and effect lowering, bytecodes, ceval macros/entry/completion, frame/genobject
residence and focused behavior tests. Shared `pycore_ir.h` payload/continuation
types, exec pointer and mode tables get one reviewed sequential interface commit.
Frame header, interpframe, ceval headers and debug exports get a second reviewed
commit. Backend owns remote schema ports; compiler owns magic and public-image
fixtures; root owns generated products and integration.
The closed cold-map unit is not a shared-file dependency.

Raw closure inventory from selected source: return_slot/return_pc constructors
and copy in interpframe; `_PyFrame_SetReturnValue`/PublishReturnSlot; eval-loop
sentinel/start/throw and init-shim/direct/transaction binding in ceval;
bytecodes getitem/attr and call/generator writer intrinsics; generator native
send/create/frame-object/clear; frameobject active-call/context/redirect setters;
ir generator frame-switch return flush, return_to_caller, enter_generator,
suspend_delegated and completion lowering; instrumentation and specialization
mode-preserving handler selection; debug_offsets, remote stack/frame/asyncio and
GDB readers. Existing call-window/child-overlap exclusions remain authoritative.
This includes raw suspended-delegation readers in remote asyncio.c and its ABI
offset validator, not just a renamed debug-offset field.
Use rg actual writer/reader census in implementation receipt, not this list as
permission to leave a raw old consumer.

First source increments: (1) inert shared scalar interface and executable-map
lifetime with no ACC selection; (2) parsed static placement/effect emitter and
focused host emission/mutation tests; (3) typed writer/reader ports,
complete CALL/SEND/YIELD source closure and compiler edge selection; (4) only
then root-generated products and activation. No partial runtime nomination.

## Decisive verification and cost gate

Host tests must show WORD nonparticipants unchanged; ACC input never reads its
NULL home; owner copy/TAKE/alias rejection; producer/sole-use/mode/edge mismatch
refusal; SEND three-edge separation; unknown effect spills before callback;
same-site deopt/stats once; abrupt output absent; typed map size/overflow/OOM and
ordinary static trampoline constants. Runtime tests for root cover mortal
results and displaced bindings, finalizers/GC/cycles, pending callbacks,
same-PC and applied redirects, hooks/monitoring/profile changes, throw/close,
recursion failures, copied frames and public replace/marshal invalid ACC edges.

Root's complete native slices must price arithmetic through actual consumer,
Python CALL including input binding/body/retirement/consumer, and a whole
SEND/yield plus completion cycle. Record actual handlers and owners, code/ELF
identity, instruction/stack/text changes and all returning C edges. Charge:
2B/word+8B/exec+padding; ordinary binding map loads and index arithmetic; typed
kind masks/branches in completion; ACC word reloads/live register pressure;
ACC_LIVE phase stores and clears, including transfer/suspension/effect commits;
effect spills/reloads and private tail transitions; extra physical handlers and
unknown-provider specialization refusal. No extra opcode/native hop may be
silently omitted from a path total. Nonparticipant dispatch keeps low16 loads.
No expected speedup, frame-size saving, natural benchmark coverage or parity
claim precedes these complete receipts and root's broad protection screen.

A standalone native C check of the current typed-header checkpoint confirms
the 64-bit GIL release frame header remains 112B, with the recipient at offset
104 (2B) and phase at 110. The executable header grows 40→48B; words remain 8B.
`E59/execution72-geometry-checkpoint.json` pins the actual headers, compiler,
fixture and binaries. This is layout evidence, not an interpreter build or
qualification of a final candidate; changed headers require a fresh check.
