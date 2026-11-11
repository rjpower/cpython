# Source attribution, semantic recovery and ordinary execution

Read-only audit of C `3e96bd9311e2e91fc45faad2d058c4ea749ed1a2` and retained
`native50c` disassembly. Source below means the selected runtime checkout
`BASE/worktrees/execution-boundaries-20261108`, with BASE in HANDOFF. No runtime
edits, builds, benchmark runs or commits. Read current guidance, the CPython AI
policy and C50-post-review. The coordinator reports both final focused roles
pass 1,939 tests; this audit does not independently execute them.

## Finding

**Ordinary interpreter dispatch, method/CALL specialization, C admission and
frame activation do not decode line/column/source maps.** C has no source-span
or same-line gate. Removing debug/source lookups from that hot route would
remove zero demonstrated work. Execution PCs, instruction operands, feedback
units, control-flow targets and exception regions are semantic execution data;
they remain necessary even if every source coordinate is unavailable.

There are two real remaining couplings:

1. Register lowering uses line equality as an optimization restriction to
   preserve today's observed line-entry/stack contracts. Thus source layout
   affects generated core execution even though the interpreter does not read
   locations on each hit.
2. Source-only observer records pack semantic exception recovery with source
   positions inside co_linetable. Unobserved exception routing is independent,
   but an observer callback error can make the unwinder decode this container.

These should be addressed explicitly, not by pretending they are present-day
DeltaBlue hot map lookups or deleting observer information.

## Exact path inventory

| Path | Actual dependency and meaning |
| --- | --- |
| `Python/ceval_macros.h:178` DISPATCH | Loads next instruction/handler, verifies frame state and dispatches. No location decode. Debug LLTrace hooks are separate compiled/debug observer paths. |
| `Python/specialize.c:631` pair nomination | Reads PCs, immutable operand/window/arity geometry, current raw handlers, exception-table absence and `_PyCode_InstrTarget`; current receiver/type/layout proves exact function kind. No line or column access. |
| `Python/ceval.c:986` C preflight | Thread ownership, redirect, trace/profile/PEP523, current instrumentation epoch and raw next CALL handler. Epoch is observation configuration, not source coordinates. |
| `Tools/cases_generator/ir_generator.py:2793` terminal consumer | Captured canonical owners, current function version/code arity/stack/recursion, CALL feedback/destination and CALL+1. Sets caller PC to original CALL. No source decode. |
| `Include/internal/pycore_interpframe.h:584,626` overlap/exact push | Storage geometry, owners, code/IR pointers, globals/builtins and continuation. Executing code anchor is required independently of source locations. |
| `Python/ceval.h:483` ordinary exception lookup | co_exceptiontable encodes PC ranges, handler PC, cleanup slot and lasti. These select Python control/cleanup behavior, not display. |
| `Python/ceval.c:1291,1314` DTrace entry/return | Explicitly enabled observer path resolves filename/function/line using frame APIs. Not an ordinary disabled observer cost. C nomination declines DTrace builds initially. |
| `Python/instrumentation.c:1500,1583` source-line/edge events | Monitoring-only path reads source records, sets observed f_lineno, executes callbacks and manages redirects. Comment at 1580 explicitly states ordinary dispatch does not read source maps or update cursors. |
| `Objects/frameobject.c:1622,3442` resume analysis/f_lineno | Observed debugger paths decode source and structural maps to find valid execution targets/owner state. They are not direct opcode admission. |
| `Python/frame.c:256` public get-line | Decodes attribution on request. No reason to remove requested Python behavior. |

A whole-file search of generated_ir_cases.c.h finds no line/column/source-map
API usage. The call route contains only execution PC updates. Keeping those
updates is how traceback/source APIs resolve attribution later without doing
location work during every operation.

## Compile-time source dependence

Examples in `Python/regcfg_passes.c`:

- `swap_rename_region:102`: SWAP renaming stops across line changes. Comment at
  71 explains f_lineno assignment expects the original stack order at line starts.
- `CopyProp:273`: local-load borrowing stops across lines, separately from local
  writes and callback/effect barriers. Comment at 170 names line events and
  debugger stack-state compatibility.
- `store_producer:361`, `StoreElim:393`: store folding requires same line in
  addition to value-use, local effects, target and exception/effect restrictions.
- `unpack_locals_site:449`: consecutive local stores can fold only on the
  unpack's line; this preserves observable intermediate stores today.
- `remove_with_line:539–558`: eliminating a producer/pop preserves a source-only
  NOP origin if no neighbor carries its line. The assembler later gives this
  NOP no executable word; it is not necessarily a runtime NOP tax.
- `fall_through_pop:705`, `fold_local_truth:728`, `truth_fold_region:773`: truth
  composition requires line agreement to retain intermediate observer behavior.
- Constant-slot candidate scan `:938`: line changes end eligible load/reader
  matching even though constant/value/control facts may remain valid.
- `FuseJump:1175`: compare and jump must share a line and exception region because
  one emitted word currently carries the compare position. The exception-region
  requirement is semantic; the coordinate requirement is an attribution/observer
  representation limitation.
- `elide_null_region:1219–1230`: source-line continuity chooses whether eliminated
  NULL initialization retains a source-only origin. Again not proof of an
  executed placeholder.

`Python/assemble.c:1298` emits zero words for NOP/elided/completed operations.
`:379–397` serializes source-only edge records, and `:414–415` rejects a leading
source-only span with no executable predecessor. These are compile-time choices
for observer representation. They can affect code admission/layout, but are not
map decoding in the normal interpreter.

Deleting these line tests without a replacement contract could expose borrowed
locals after debugger writes, lose line callbacks or offer f_lineno a state with
unreconstructible owners. Conversely, retaining them permanently makes source
formatting an input to optimization. The clean goal is to make that dependence
explicit and then remove it with observer-state reconstruction, not treat line
equality as an ownership or control-flow proof.

## Semantic recovery currently bundled with source positions

`Include/internal/pycore_ir_source.h:10–30` packs from/to/edge, four source
coordinates, and handler/exc_slot/lasti into ten fields in co_linetable. Only
observation reads edge records during successful execution. But callback-error
recovery is stronger than presentation:

1. `Python/instrumentation.c:1572` calls `_PyEval_RecordSourceError` with record
   ordinal when a source-only line callback fails without a redirect.
2. `Python/ceval.c:2172` retains frame plus that ordinal in the C evaluation
   activation. Nested C entries get separate pending-error state.
3. `Python/ceval.h:543–555` detects that pending source error, decodes co_linetable
   and reads handler/exc_slot/lasti to choose semantic exception recovery.
   Ordinary errors instead use co_exceptiontable.

Therefore stripping co_linetable edge records can alter exception handling when
an observer raises. This is a real cold coupling of semantic recovery to an
attribution container, not evidence that unobserved DeltaBlue reads coordinates.

### Small clean boundary, if source separation is selected

Have the observation provider resolve and copy an explicit semantic recovery
recipe into the pending error record when it knows which observed event failed.
The recipe is `(handled, exception_slot, handler_pc, preserve_lasti)`; retain a
stable event/origin ID separately for attribution. Then the unwinder consumes
that recipe and never reads a source map. Preserve activation-local ownership,
frame identity, nested C entry isolation, redirect invalidation and once-only
consumption. The existing four-int `_PyExceptionHandler` at ceval.h:330 is the
natural execution shape; expose an appropriate internal type rather than add
an independent tuple convention or runtime flags.

This removes the unwinder's source-container dependency and repeated cold
record decode. It does not remove the need for semantic recovery metadata:
source-only events can belong to an exception region different from the next
physical word. It also does not by itself remove compiler line restrictions or
make the serialized map removable from public/observer APIs. A later metadata
format change can store structural event/recovery identities separately from
coordinate arrays, with explicit marshal/code.replace/debugger reader updates.
Do not grow this into a port of every code object consumer in this performance
experiment.

Tests for this boundary must cover line callbacks raising inside/outside try
regions, eliminated source-only spans, same-physical-PC redirects, failed frame
materialization, nested C evaluation, cleanup reentry and observer disable/enable.
Assert original catch/cleanup behavior and attribution separately. A native code
inspection should show ordinary dispatch and exceptiontable routing unchanged.
No DeltaBlue speedup is predicted for this cold change.

### Longer-term source-independent lowering contract

Core passes should consume value def-use, ownership/liveness, effects, explicit
control/exception edges and execution identities. Carry logical origin IDs as
passive annotations through transformations. A separate observer projection maps
those identities/edges to positions and reconstructible frame states. A fused
operation may carry several origins; it should not require equal coordinates.
Late observer attachment and f_lineno writes must have explicit reconstruction
or an observed fallback before callbacks. Source coordinates select requested
observation positions; they must not establish core ownership/semantic safety.

This is broader than replacing a condition. In particular, present CopyProp and
SWAP line restrictions protect debugger-visible state, so side-map coordinates
alone cannot replace them. Start with one independently reviewed transform,
prove its observer boundary and origin composition, then remove that transform's
line gate. Compare equivalent source variants with different whitespace/line
wrapping: unobserved IR/control/owners should agree while attribution differs.
Keep observer callback sequences, local writes, redirects, traceback ranges and
exception routes correct. Defer the broad rewrite from this measured call round.

## C versus ordinary A: surviving native work

Retained native50c exact CALL is 1,410 bytes and 339 instruction lines, essentially
unchanged from A after address normalization. Refined C is 1,396 bytes versus
A's 249-byte ordinary canonical method handler plus exact CALL. These are code
sizes, not dynamic costs. C saves a dispatch and callable home publication/
reload, and preserves nomination's exact-kind proof. It still owns function,
self and independent code, initializes a real frame, and uses ordinary return.

Native C at `0x367128` checks thread owner; `0x36714b` checks PEP523. Later,
`0x36727c` repeats PEP523, and `0x367386` tests owner again before a retained
cold generator alternative. At `0x367391–0x3673a4` it still derives has_self
and argument base despite C's owned self and zero-explicit-argument contract.
The first observer epoch/raw-neighbor checks are actual eligibility work absent
from A's simple method capture, so removed dispatch alone is not a net saving.
Code size/branch observations cannot causally explain a campaign timing gap.

### Strongest bounded general simplification to review

Consume the already-checked thread/self facts at the existing shared overlap
boundary. After self is installed at call_window[1], C can call:

    _PyFrame_PushOverlapping(tstate, function_owner, call_window + 1, 1, frame)

Ordinary `_PyFrame_PushExact` already uses this same implementation. Header,
allocation, code ownership, continuation and frame entry therefore remain shared;
no second activation ABI, runtime mode flags, copied body or custom return is
needed. C's callable home is already NULL, so its generic wrapper's repeated
NULL store is unnecessary. No optional gate is added to ordinary calls.

The proof is the GIL-held, callback/decref/allocation-free interval between
preflight, guarded capture, CALL admission and commit. Producer/consumer
validation must keep that interval explicit; debug assertions retain thread
owner, nonnull self and physical window geometry. Keep current function version,
arity, stack and recursion checks, and fallback to captured CALL before effects.
The same proof can justify dropping the repeated post-capture PEP523 test, but
only after its shared checked-entry contract is reviewed; do not start deleting
observer checks based on source names or nominal handler identity.

First obtain direct matched C/A timings and actual handler326 entries/commits,
then compare dynamic ordinary/refined instruction paths with equivalent scopes.
Root is building frozen A and C stats controls. If a simplification is warranted,
require release assembly to remove the owner/null-self/generator work and retain
ordinary A-path native code, focused semantics and matched target results. Reject
or keep A if the added eligibility work outweighs the saved boundary. Do not
expand arities/body grammar to rescue C.

## Bounded follow-up decision after direct evidence

Coordinator direct frozen C/A six pairs found DeltaBlue1.043648 (all six slower)
and nbody1.038921, with logging0.968599. C is not admitted for the target. Actual
Py_STATS census separately records42,710 refined entries and40,629 committed
activations per unchanged DeltaBlue iteration after warm30; entry-without-commit
is2,081. Coverage is real but does not establish lower cost. Root authorizes one
bounded simplification, then selection of A if it does not improve DeltaBlue.
These results come from coordinator evidence, not audit execution.

The change is precisely the existing shared PushOverlapping call described
above, in C's generated terminal continuation only. Ordinary PushExact, frame
entry, continuation, ownership and return remain unchanged. Keep current mutable
admission checks, including the repeated PEP523 test for this one experiment;
no other gate or optimization expands. Peer already agrees the shared boundary
and proof; obtain explicit follow-up approval before editing.

Add one runtime test to TestAdjacentMethodCall: a factory creates ordinary caller
`return owner.selected()` and a Python selected method returning42. Recursively
replace factory/nested code objects with `code.replace(co_linetable=b'')`
before executing the factory's MAKE_FUNCTION operations. Assert
unchanged co_code/co_fbinit/co_exceptiontable, all co_positions tuples are None,
then warm and assert refined handler installation and42 results. This is a
legitimate constructor path: Objects/codeobject.c:633 pads empty raw positions
with no-location runs; :1651 normalizes the container without changing executable
IR. Existing test_ir_boundaries:42 already verifies this normalization. Do not
strip source-only observer records from code with observer/error semantics for
this fixture; the simple caller/callee have no exception regions or eliminated
source spans requiring such recovery.

This proves C installation/execution does not need line/column attribution,
without implying source-independent lowering or complete observer metadata
separation. It adds no runtime source lookup or helper API. A generator test
checks C enters shared PushOverlapping with known self span while ordinary exact
CALL still uses unchanged PushExact. Native gate: generator/null-self branch
must disappear from C, ordinary CALL must remain stable, semantics/source test
must execute and direct C/A must improve the target; otherwise retain rejected
C evidence and select A. No further C microiterations are proposed.

The initial fixture assigned stripped code to already-created functions and
failed exact CALL eligibility. Diagnosis found that __code__ assignment
permanently clears the function specialization version (funcobject.c:297/371),
independently of source contents. A minimal native probe confirmed original
functions refine, mutated functions decline, and targets created by MAKE_FUNCTION
from pre-stripped code refine with every position absent. The peer-approved
test-only correction retains all assertions and passes its native release unit;
no core admission change or extra warmup was needed.
