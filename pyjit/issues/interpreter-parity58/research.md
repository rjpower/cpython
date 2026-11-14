# Interpreter parity58: research and critical audit

This is source/native inspection and architectural analysis, dated 2026-11-14.
No runtime change, build, generation, test or performance execution was performed
for this review. The proposed integrated design is
[interpreter-parity58](../../design/proposals/interpreter-parity58.md).

## Identities and evidence discipline

Canonical documentation lives in
`C=/home/power/code/cpython/.worktrees/python-jit`. Its runtime C is stale.
Actual selected source inspected here is
`I=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated`
at `0dcba75bb3df40c32137db453b001f7e8f101de0` (verified by `git rev-parse`).
Upstream source is its sibling `upstream-3a440b6`, pinned to
`3a440b65695615e9fd9912342b79cdd54fced0d2`.

`E=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1`.
Selected native is `E/call-model55/comp57-dictionary-native/python`, SHA256 in
its manifest `db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2`.
Read-only `objdump -d --no-show-raw-insn --disassemble=SYMBOL` was used on that
retained ELF. Addresses below identify this ELF, not other experimental builds.
Existing upstream native excerpts are under
`E/interpreter-performance56/comprehensions57-source-audit`.

Reviewed applicable AGENTS and the [CPython AI policy](https://devguide.python.org/getting-started/ai-tools/),
then README, GOAL, STATUS, HANDOFF and PROCESS, model56's combined proposal and
critical audit, selected56/comprehensions57 analyses, reports, plans and tables,
and current register/value/interpreter/activation/observer contracts. Policy
requires human accountability and review; this analysis is not permission to
bypass tests or remove supported behavior.

Evidence labels used below:

- **Measured:** retained exact-head campaign or artifact, without fresh execution.
- **Source/native fact:** inspected selected C, generated C or retained native.
- **Inference:** an explanation supported by those facts but not an exclusive
  measured attribution of time.
- **Proposed:** a new contract, experiment or implementation change.

## Measurements constrain the architecture

Selected57's matched upstream counters are from
`E/comprehensions57/dictionary-measurement/perf-{numbers.json,table.md}` and
`dictionary-perf-stat`; all 18 invocations are reported valid, unmultiplexed and
identity-stable. Instructions/cycles are 1.436416/1.333179 for comprehensions,
1.361424/1.118423 for DeltaBlue and 1.307831/1.267788 for coroutines. Approximate
comprehension instructions/tray are 278,517 versus 193,897. The ratios are not
statements that the same fraction of candidate time is removable.

`dictionary-measurement/{numbers.json,table.md}` owns the all29 and confirmation
campaigns. All29 .995586/golden56 and 1.127137/upstream are observed two-pair
aggregates. The opposing upstream XML iterparse outliers prevent a precise
global parity-gap claim. Confirmation leaves comprehension/DeltaBlue/logging
near flat and accepts roughly 2% coroutine/raytrace debt plus uncertain generator
debt. Dictionary qualification is 203 methods per role with overlapping focused
coverage, not a new full-system run of this two-file successor.

The rejected `9ec291998f7395fedf21f456e5eb58c2187d2e23` four-lane composition
removed real native work but lost logging/coroutines. Its upstream comprehension
instruction ratio was 1.394208; it did not close the gap even before its broad
regressions disqualified it. Native handler shrinkage and lower instruction
count were insufficient selection evidence. The alignment and local-append
revisions also lost. A failed frontend PMU capture cannot establish an
instruction-cache explanation. Branch counts/misses that remain unchanged do
not prove absence of other frontend or dependency effects.

## Trace A: nested dict.get, list iteration and append

**Source workload.** The retained unchanged workload expression is
`[id_to_widget.get(dwid) for dwid in w.derived_widget_ids]`. A tray executes
108 dict.get, 162 appends, 54 dict subscriptions, 42 predicate calls and
18 generator creations; counts come from the retained separate operation probe
`E/interpreter-performance56/comprehensions57-inspection/operation-counts.json`.
The warmed candidate/upstream disassemblies in that directory show six versus
eight words in the inner loop. Those artifacts predate dictionary-only57, whose
selected changes do not alter the IR/compiler/handlers; this is inherited IR
evidence, not a newly executed adaptive dump of selected57.

**Register/generated path.** I `Python/bytecodes.c:5010` and generated
`Python/generated_ir_cases.c.h:8479` implement the descriptor call. It reads an
owning window; checks pointer space, descriptor type/flags, self existence and
self type; invokes `_PyValue_VectorcallTstate`; publishes result over callable;
closes args/self; polls; moves or discards the result. The guard is useful;
reentering general dispatch loses its narrow admission knowledge.

I `Include/internal/pycore_value_call.h:109` performs value-call dispatch;
`Objects/descrobject.c:621` checks descriptor args and keywords before
`method_value_invoke` enters recursion-protected native code. Upstream
`Python/bytecodes.c:5290` projects the already-checked descriptor and directly
uses `_PyCallMethodDescriptorFast_StackRef`. This comparison concerns
dispatch/check transport, not boxing an integer key: the selected value-native
dict method does not need such a box.

Selected native `_TAIL_CALL_CALL_METHOD_DESCRIPTOR_FAST` starts `0x390a50`.
At `+0x408` onward it checks general value-call support/offset/target and calls
the selected value-vectorcall function at `+0x451`; the dispatcher is inlined,
not a literal `_PyValue_VectorcallTstate` CALL instruction. From `+0x239` it
loads/NULLs/closes argument homes; a second loop from `+0x2a0` clears window
homes before self retirement. These are real native loops inside **one**
dispatched handler, not extra bytecodes for each DSL cleanup phase. A future
single retirement plan could avoid redundant clears only with actual
callback/owner-phase proof; deleting a source loop without that proof is unsafe.

**Selected dictionary path.** I `Objects/dictobject.c:4614` (`dict_get_impl`)
calls `dict_get_value` at 2068, below public validation. Its callback-capable
route owns dictionary/query, hashes, calls `dict_lookup_value` at 1366, acquires
the found result, then retires pins. `can_use_cached_str_lookup` additionally
requires a Unicode-only table under the GIL. Mixed-table equality can call
Python even for a cached exact-string query. This source no longer makes the
old public-reader → known-hash-reader nested-pin journey. Selected `dict_get`
native at `0x1d6a80` contains `_PyValue_Hash` at `+0x99` and `_Py_dict_lookup`
at `+0xb4`, agreeing with the shared provider path. Refcount/tag handling and
the provider's actual hashing/probe work remain; the entire symbol is not tax.

**Iteration.** I bytecodes 3879 combines `_ITER_CHECK_LIST`, `_ITER_JUMP_LIST`
and `_ITER_NEXT_LIST`; generated cases 14766 project list state, use
`PyList_GET_VALUE`, DUP, increment cursor, then generic `IR_PUBLISH_TEMP` for
cursor/item and `IR_MOVE_TO_DST_OR_DISCARD`. Native `_TAIL_CALL_FOR_ITER_LIST`
starts `0x3957e0`. Ordinary item path:

| Native anchor | Work actually emitted |
| --- | --- |
| `+0x15` onward | Publish current PC, decode B, load receiver, pointer/type guard |
| `+0x52`–`+0x69` | Load/decode cursor and compare current list size |
| `+0xd0`–`+0xe9` | Three stores, call `PyList_GET_VALUE`, three reloads |
| `+0xed` onward | Result ownership classification and cursor increment |
| `+0x11a` onward and `+0x26b` onward | General cursor/result publication classification, with PyLong/PyFloat tests and canonicalization cold branches |
| `+0x19d` onward | Window result store, dynamic destination classification, local replacement/window NULL, old-owner classification/retirement |
| `+0x1f2` onward | Reload next word, handler-table lookup, restore native frame and tail jump |

The native branch layout has several value-dependent paths; these offsets do
not assert all branches execute per item. They establish that known protocol
and representation facts did not eliminate the general machinery. The 32-byte
stack reservation and getter spill/reload sequence execute on the ordinary
item path. The getter is a trivial array read; moving its C body is not enough
unless the enclosing typed interval also removes repeated projections and
publication. Round57 tried only part of that closure and still lost broadly.

**Upstream iteration/append.** U generated cases 6463 directly reads list item
storage, acquires a StackRef, advances cursor and writes stack positions. Its
later STORE_FAST still pays destination retirement; do not compare our fused
local store against only upstream FOR_ITER. U `Include/internal/pycore_list.h:36`
inlines spare-capacity append and calls resize only when needed. Retained
`upstream-append-calls.txt` shows evaluator resize-call sites `0x2a5e58` and
`0x2a7986`. I `Python/bytecodes.c:1239` uses `_PyList_AppendTakeValue`; selected57
did not retain the rejected inline append. Both consume the element owner;
removing an invented extra append DUP/CLOSE is not an available saving.

**Inference.** This loop is the clearest example of a smaller instruction stream
lowered into larger native intervals. A source-stack-shaped window and generic
tagged APIs repeatedly reconstruct information already established by guards.
The C compiler removes some repeated expressions but cannot infer ownership,
container canonicality or cross-handler facts from our external semantic rules.
The observed ~84.6k instruction excess is much larger than these named primitives
alone have been shown to remove.

## Trace B: exact Python call and return

I bytecodes 4353/4409 and generated cases 11958 show exact admission, reservation,
initialization, explicit return slot/PC and direct child dispatch. Thread-owned
caller arguments already overlap callee homes. Generator-owned callers copy to
the thread arena. `_PyFrame_ReserveForCall` at
`Include/internal/pycore_interpframe.h:780` uses initialized storage/frontier;
the ordinary path does not fill all callee slots every time.

`call_transaction_begin` at I ceval1417 checks homes in a source loop, but
`_PyValue_RecheckHome` is `((void)0)` in release (`pycore_value.h:648`). Selected
native `_TAIL_CALL_CALL_PY_EXACT_ARGS` at `0x38ae80` contains no argument-check
loop on entry. Charging that debug loop to release performance is an error.

The native body has a 64-byte native stack reservation and separate pointer-space
test of window storage (`+0x37`) and checked value projection (`+0x50`), followed
by type/version, argument-count, stack/recursion admission. The latter conditions
are not all removable: some are Python correctness or fallback admission.
The candidate then computes overlap/frontier (`+0x124` onward), initializes the
112-byte frame header (`+0x190` onward), optionally fills constant homes, saves
recipient/continuation and dispatches. `_PyStackChunk_ExtendInitialized` is on
the frontier-extension branch (`+0x2a4`), and `call_transfer_exact_args` is on
the non-thread caller branch (`+0x2e0`), not unconditional warm costs.

`_PyFrame_Initialize` (interpframe478) writes code/function pins, environment,
base/previous, execution pointers and lifecycle flags. Both pins are deliberate:
the executing code must survive mutation of the function's current code. Header
struct assertions place `phase_state` at110 and owned slots at112. Thus a claim
that this is a huge legacy frame or another heap allocation is unsupported.

Generated `RETURN_VALUE_EMPTY` at22942 takes a canonical value, unlinks the child,
calls existing result-preserving retirement, then directly applies
`_PyFrame_SetReturnValue` for ordinary return. It uses generator completion only
for generator-owned frames. The empty form is already a proof-based distinction;
general return handles surviving scratch. `_PyFrame_SetReturnValue` at
interpframe262 checks redirect/discard and publishes the actual recipient. A
local recipient's old owner must reflect callbacks during child retirement.

Selected native `_TAIL_CALL_RETURN_VALUE_EMPTY` at `0x3986b0` confirms the split:
source local-versus-temporary classification at `+0x29`, thread retirement call
to `clear_thread_frame` at `+0x22e`, and ordinary redirect/recipient handling from
`+0xc4`. `_PyEval_CompleteGeneratorReturn` at `+0x1c9` belongs to the generator
branch, not every Python return. The body directly stores the canonical result
and tail-dispatches; no generic ordinary-return continuation decoder is present.

U generated cases4684 copies args into a new frame; cases11474 return via the
saved stack pointer and direct push after cleanup. Upstream has a simpler
recipient, but ours can eliminate separate store/discard instructions. Count
both sides' complete call/return sequences. The target is better physical
lowering of our chosen convention, not another proposal for overlap or inline
returns that already exist.

**Proposed distinction.** Prove ordinary recipient shape at the waiting site
and consume it at completion, while retaining redirect handling when callbacks
can change the parent. A cold-state/header experiment must first identify
executed writes and dependent loads it removes. Removing pins, all refreshes,
or useful cached environment pointers is not justified by header size.

## Trace C: attribute reads as a negative control

`w.derived_widget_ids`, DeltaBlue's object fields and other ordinary attributes
use adaptive LOAD_ATTR forms. I generated cases19571 implement
`LOAD_ATTR_SHAPE_LOCAL_TEMP`: borrow the local, validate pointer/type version
and shape, `shape_read_dup` the field and directly store its canonical owner.
The apparent receiver-retirement code has a NULL receiver in this variant.

Native `_TAIL_CALL_LOAD_ATTR_SHAPE_LOCAL_TEMP` at `0x378a80` confirms no ordinary
helper call, no receiver acquisition/release and no result normalization. It
does exact guarded loads, a conditional result incref, destination store,
optional method NULL and dispatch. There is a phase-zero write at `+0x9b`.
Proving that write redundant is different from removing all semantic publication.

U generated cases8963 `LOAD_ATTR_INSTANCE_VALUE` guard type version and inline
values validity, load/acquire the field and close the receiver. The two runtimes
encode storage differently; type version and shape/validity checks implement
real lookup correctness. Our implementation is already strong here. A new
universal wrapper, capability object or per-op descriptor lookup would add cost.

**Inference/proposal.** If a checked field result feeds a simple callback-free
test, a short physical region may avoid an intermediate owner/home and dispatch.
That requires the owner anchor and miss/observer proof, not another generic
canonical-output boolean. Descriptor fallback, polymorphism and mutation keep
their guards. Existing SHAPE PIC is already selected and not a new opportunity.

## Trace D: resume/yield and completion

I generated cases14698 (`FOR_ITER_GEN`) and23209 (`SEND_GEN`) guard the provider
and generator state, record actual recipient/continuation, link exception state,
switch current frame, enter recursion, deliver the resume input, load the saved
instruction and conditionally skip ordinary RESUME. Those are direct child
entries. `IR_SKIP_RESUME` (ceval_macros864) checks instrumentation/breaker state;
it does not blindly suppress events.

Generated YIELD_VALUE25537 takes the outgoing owner, NULLs its home, restores
exception linkage, records incoming resume recipient, updates profiling frame
state, unlinks child, publishes suspension and stores a canonical result via
`IR_STORE_CANONICAL_RETURN`. Direct FOR/SEND/throw completion entry points and
literal generator recipients already landed56. They must not be proposed again
as though missing.

Native `_TAIL_CALL_YIELD_VALUE` starts `0x34e3d0`. It has a 16-byte native stack
reservation; outgoing NULL/suspension/linkage and frame switch occupy the initial
body. From `+0x92` it examines pending redirect, destination/discard, frame owner
and local extent. Local replacement can decref/deallocate; cold redirect calls
`_PyFrame_CompleteRedirect` at `+0x127`. These branches implement the general
recipient rather than a generic generator dispatcher. Canonicalization is
already absent. U generated YIELD_VALUE13563 performs analogous exception/state
switches then writes a known stack top directly.

The corresponding selected native `_TAIL_CALL_SEND_GEN` at `0x396af0` checks
PEP523, representation, exact gen/coro type and created/running state before
commit. From `+0xa0` it transfers the input, saves continuation, links exception
state and switches frame; `+0x10e` onward applies the general incoming-recipient
logic. It directly dispatches the child from `+0x190` onward after the RESUME
check. Thus this two-sided recipient cost is visible in native resume **and**
yield, while the child switch itself is already direct.

**Inference.** General recipient geometry, physical home traffic and state
publication provide a common optimization target with ordinary calls. They do
not make exception linkage, running-state protection or child cleanup optional.
Coroutine/generator deficits require an entry/exit-pair analysis; optimizing
terminal exhaustion alone cannot remove work on every yield.

## Minimum compiler/generator extension

Upstream `Include/internal/pycore_stackref.h:546` implements Borrow by setting
`Py_TAG_REFCNT`; DUP638 and CLOSE699 touch the object count only when the
StackRef owns an object count. MakeHeapSafe672 acquires a strong owner for an
unsafe mortal borrow. U bytecodes290 uses Borrow for LOAD_FAST_BORROW. The
compiler's lifetime rules at `flowgraph.c:2866` allow borrowed argument locals,
require the supporting local to survive its stack uses, retain local values
displaced through `f_locals`, and strengthen references on heap escape and
return/yield. This is a deliberately supported runtime protocol, not simply
a C borrowed pointer accidentally stored in a frame.

I's owner-only homes choose a different contract. An unfolded LOAD_FAST_BORROW
owns its temporary; direct folded local views avoid that temporary entirely.
Thus the comparison must count surviving snapshots and actual acquisitions,
not source LOAD_FAST names. Result acquisition from an independently mutable
container remains necessary when its value must survive; both runtimes pay it.
Upstream borrowed argument admission can avoid count updates at some Python
call boundaries even though it copies slots, while ours avoids argument copying
but transfers owning snapshots. That tradeoff is material to the holistic
comparison and needs counts before an owner-only parity claim.

Existing `regcfg_passes.c:155` onward documents local folding. CopyProp243 uses
pending loads, clears at effects/writes, requires same source line, one popped
use and matching exception region (273–298). Thus additional aliasing is a
bounded extension with real remaining restrictions; a claim that every local
currently loads and INCREFs is false.

Model56's reproducible 21-file/426-CFG census already found zero surviving
callback-free COPY webs in its proposed first morphology (report69–72). This
closed negative result is an admission constraint. Packet3 must identify a
different positive population, such as surviving CFG-demanded snapshots or
producer/consumer home traffic, before implementation. The general presence
of a multi-use restriction does not prove a hot missed opportunity.

Proposed new compiler records attach to existing value IDs: equivalence,
anchor/version and materialization demand. Proposed generator facts extend the
existing action graph: exact projection, canonical output, internal protocol,
ownership, read role and invalidating effects. Generated C consumes those facts
through typed locals and direct publication/recipient forms. Neither adds a
runtime proof object. Initial virtual values keep owners in existing homes;
physical vacated cells remain NULL. No root exists only in an unenumerated
register across a callback/collection point.

Within-word typed lowering is distinct from cross-word fusion. The first joins
existing DSL micro-ops into one coherent native fact interval. The second keeps
values between two-to-four original words and removes intermediate home traffic
and dispatch. It needs explicit interior-site/exit metadata and observer
defusion. No static source annotation can turn an unknown Python operation into
a pure one. A specialized miss must materialize and continue before effects;
a post-effect path continues from committed state and never replays work.

## Primary-source comparisons and their limits

The actual handler ABI is another negative control. I `ceval_macros.h:64–108`
uses musttail plus GCC `no_callee_saved_registers` or supported `preserve_none`;
`ceval.c:2404` explains the GCC15.2 non-tail entry bug and normal-ABI
`enter_handlers` shim. The shim runs at evaluator entry, not per dispatch.
Inspected FOR_ITER_LIST has RBP setup and getter spills but no r12–r15
callee-save prologue. The relevant costs are helper crossings, operand/state
movement and live ranges, not assumed ordinary SysV saves around every opcode.
Likewise [frame-state54](../frame-state54/report.md) already achieved an
80-byte header and rejected its performance tradeoff. A new header experiment
must include all changed field readers and dependent loads.

The pinned upstream files above are the relevant semantic and performance
control. Two external implementations supply narrow design examples:

- V8's [BytecodeRegisterOptimizer](https://raw.githubusercontent.com/v8/v8/12b2ffaed1dafdc79df251fb693a4f1dfd278c96/src/interpreter/bytecode-register-optimizer.cc)
  explicitly separates register equivalence and materialization and tracks type
  hints. This is a concrete precedent for avoiding a physical write for every
  logical register move. The reference was resolved through GitHub's commits
  API and reread at `12b2ffaed1dafdc79df251fb693a4f1dfd278c96`; this is source
  precedent, not a matched performance experiment. Python needs owner lifetime,
  reentrant finalizer and frame-locals reconstruction rules; those must be added
  to the idea, not assumed from JavaScript GC.
- LuaJIT's [x64 interpreter source](https://raw.githubusercontent.com/LuaJIT/LuaJIT/63c8f1a9b479ba5518cf6a170ff7729178bc3cfd/src/vm_x64.dasc)
  defines VM register assignments, decode/dispatch macros and call windows
  together. Lines18–79 and210–253 show native register/ABI planning as part of
  the interpreter design. Its replicated-versus-common dispatch commentary is
  historical and architecture-specific. This is motivation to inspect our
  actual helper spills and dispatch, not evidence that assembly, Lua's frame
  convention or a particular cache layout would make Python faster. The v2.1
  reference was resolved through GitHub's commits API and reread at
  `63c8f1a9b479ba5518cf6a170ff7729178bc3cfd`; it is not a matched benchmark control.

The [Ignition description](https://v8.dev/blog/ignition-interpreter) also describes
register-bytecode execution and generated handlers. No V8/LuaJIT speed number,
GC choice or compatibility rule is imported into this proposal. The comparison
does not require a new optimizer framework or abandoning the selected RC model.

## Unresolved questions and concrete decision

We can explain a repeated mechanism and show native examples; we cannot assign
all 84.6k excess instructions or the cycle deficit to exclusive categories from
flat samples. Missing evidence is a post-pass frequency/materialization census,
hot native path counts for full call/resume pairs, and valid event groups for
cycle regressions. Count those before committing to large cross-word work.

Proceed with the four connected techniques in the proposal, starting with one
typed single-word interval and a census that can admit a distinct alias/transport
transform. Carry their facts
into common call/resume completion and a single measured short region. Preserve
SHAPE_LOCAL_TEMP as a negative control; preserve all rejected experiments and
immutable controls. Do not make bitmap, IRSC2 or fullM4 automatic prerequisites.

The parity claim remains conditional. The proposal states native-work milestones
and abort criteria so that insufficient coverage is exposed rather than hidden
behind additional opcode tweaks. Only a coherent integrated campaign can show
whether the design achieves parity across comprehensions, DeltaBlue, generators
and coroutines while retaining existing numeric/logging strengths.
