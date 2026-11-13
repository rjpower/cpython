# Call model55: selected51 source cost audit

Source-only work ledger for the architecture proposal. Reference tree is
`BASE/worktrees/round51-golden` at
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`; BASE is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
The later cold construction correction does not change these ordinary paths.
This audit runs no runtime, compiler, test or performance experiment and makes
no cycle/frequency estimate. References name files in that source tree.

The target is Python data-model behavior, not exact CPython destruction timing
or order. Mandatory gates are explicit with/finally/exception control protocols,
unique ownership, lifetime safety, coherent published state and bounded
retention. A replacement may document a different RC retirement policy; exact
reverse stack-pop order is a current IR choice, not an architectural constraint.

## The existing fast call already avoids a generic copy

`Python/bytecodes.c:4331–4395` defines the exact Python path: PEP523 check,
function-version/type admission, exact arity, space and recursion checks,
_INIT_CALL_PY_EXACT_ARGS, recipient/continuation store, then _PUSH_FRAME.
A thread caller uses the final argument cells as callee homes. Optional self
adjusts the base; no per-argument count duplication or generic binder copy is
required on this hit. The callable owner moves into f_funcobj and its old
window cell is cleared. _PyFrame_ReserveForCall reserves once and
_PyFrame_Initialize initializes once; _PUSH_FRAME publishes current_frame,
adjusts recursion and reloads dispatch state.

`Include/internal/pycore_interpframe.h:577–635` preserves saved_top because
callee physical extent can end below an older caller frontier or beyond it.
Header storage grows down and value storage up within an arena chunk.
EnsureFrameStorage at 487 maintains an initialized frontier and only extends
an uncertain tail when necessary. It does **not** clear the whole arena or
all callee homes on each warm call. The overlap tail is already NULL by the
current validity invariant. Exact calls from a heap generator instead move
arguments into thread-stack homes, then clear source window cells
(`Python/ceval.c:1812`); their storage cannot overlap the thread arena.
Generic argument binding really does populate homes and handle defaults,
keywords, varargs and varkw (`initialize_locals:2685`), but that work must not
be charged to the specialized exact path.

## Work ledger

“Current-model requirement” means needed for this implementation's invariant,
not an immutable language rule. Backend optimization can remove redundant
loads/stores; this is a source contract ledger, not a dynamic instruction tally.

| Boundary / current work | Why the current model needs it | Requirement versus design choice |
| --- | --- | --- |
| Exact-call admission, function version, arity, space and recursion checks (`bytecodes.c:4308–4395`) | Selects a callable/code and valid frame geometry before commit; honors hooks and recursion failure | Correct callable/binding/errors are required. These particular guards and their distribution are specialization/ABI policy; a different call IR may establish proofs earlier. Do not remove mutable-function-code validation without an invalidation contract. |
| Activation header initialization (`interpframe.h:302–330`) | Installs independent code/function owners, bases, links, environments, PCs, mapping/frame pointer and sentinel/flags | An activation needs identity, roots and continuation. Its 112-byte fixed layout, cached globals/builtins/feedback and universal cold flags are representation choices. Removing cache words can add dependent loads; no isolated result proves an entire alternative layout optimal. |
| Independent f_executable pin plus f_funcobj owner (`interpframe.h:24,133,295–316`) | Function __code__ can change while an old activation executes; function/environment lifetime also survives | Executing-code stability and object lifetime are required. Independent ownership is real: deriving executing code from the mutable function is unsound. Choice of tagged owner versus typed activation/code descriptor is redesignable. |
| Object decode at known function/code boundaries (`pycore_value.h:691`) | AsKnownObjectBorrow checks pointer space in every build, masks owner tag and aborts malformed nonobjects; code/function type asserts are separately debug-only unless an actual guard is used | This is value-API enforcement policy, not a Python requirement for repeatedly decoding a proven typed activation pin. Do not count debug PyCode_Check/PyFunction_Check as universal release type guards. Typed internal handles require trusted ingress and independent owner retention. |
| TEMP consume (`ceval_macros.h:505–546`; IRStack.consume:2288) | Classifies local versus scratch; scratch owner moves to C and physical cell becomes NULL, local stays borrowed | A single counted owner and a safe lifetime are required under current RC. NULL-in-cell as the validity/ownership record is a design choice. Borrowed locals must not outlive reentrant rebinding. |
| Result home commit (`ceval_macros.h:550–610`) | Temporary output expects NULL; local output commits new owner before displaced old owner can run a finalizer; discard retires owner | Python operations and lifetime-coherent published state are required. Exact timing/order of old-owner destruction is replaceable under a documented RC retirement policy with bounded retention. A physical NULL store before a proven overwrite, and repeated local/temp/discard classification, follow current universal formats. Canonical-result proofs already bypass some publication work. |
| Canonical home conversion (`pycore_value.h:141`; IR_STORE_* macros) | Boxed scalar owners may canonicalize into inline home values; specialized proven producers avoid this | Canonicality is current PyValue policy. Boxing and canonicalization are not free language-mandated operations on every register write; the new IR must state which forms are legal at each boundary. |
| PC store on most handlers (`ir_generator.py:4841`) | frame->instr_ptr exposes current operation to error attribution, callbacks, traceback/observer/debugger reconstruction | Correct exposed/error state is required. Universal per-op materialized PC is a representation/observation contract choice. no_save_ip already exempts handlers. Deferring it needs a defined publication phase and asynchronous-observer contract, not only an assertion that no explicit callback occurs. |
| Escape spill/refresh (`ir_generator.py:3555–3610,2476`; ceval.c:1337–1388) | Flushes live cached owners/views before escape; callbacks/finalizers may rebind locals, redirect PC or change instrumentation. Refresh checks frame/executing words and restores current PC | Reentrant coherent roots/owners are required. Spilling every value or revalidating immutable exec pointers through the same general helper is policy. Views become stale after code_ran; a new model must keep this invalidation or replace it with explicit ownership. |
| Return (`bytecodes.c:1479`; interpframe.h:170–204`) | Keeps result C-owned through child unlink/teardown, clears live locals, function/code owners and arena frontier, then publishes to caller descriptor, replacing any old home | Correct return/exception control and lifetime-coherent exposed state are required. Exact old-owner retirement timing/order is current policy. A generic old-destination load/close even when proven empty, universal recipient representation and per-frame field placement are not. Recipient proof must belong to the actual call and survive reentry. |
| Error/unwind (`bytecodes.c:5563–5625`) | Builds traceback/raise event, finds handler, walks temporary suffix top-down, clears/retire owners, installs exception/lasti then observes handler | Exception selection/state and explicit with/finally/exception-control protocols are required. Reverse-index retirement emulating upstream stack pops is current IR policy, not a hard gate. A bitmap/map may enumerate owners in a different documented RC policy if ownership, coherent observation and bounded retention remain valid. Full-capacity suffix scanning follows NULL validity plus current ordered allocation. |
| Frame roots (`frame.c:26`; `gc.c:483`; `gc_trace.c:1062`) | Visits f object/mapping/function/code plus every physical home; skips NULL/index/inline bits; extra environment edges are explicit in trace roots | Accurate reachability and RC subtraction are required. Full-home scan and NULL testing are validity representation choices. Reference-subtraction/ownership accounting must not double-count transferred aliases; idempotent reachability marking can revisit an alias. |
| Retiring frame / finalizer GC (`gc_trace.c:1025–1046`; frame.c:175–237`) | Frame is unlinked before destructors run, yet later homes remain owners. Conservative arena/header scans retain those owners while clear_slots proceeds | Surviving owners must remain roots during reentrant teardown. An explicit retiring-root/owner record could replace reliance on raw scanned cells; deleting clears or adding a live bitmap alone does not establish this protocol. Conservative stale raw pointers can also retain dead objects even when precise maps omit them. |
| Creation/escape relocation (`bytecodes.c:5256`; interpframe.h:240`; frame.c:94`) | RETURN_GENERATOR allocates heap state then copies full physical header/homes and clears all source homes. Escaped frame takeover repeats a full physical move | Suspension and escaped frame lifetime are required. Heap representation, initial thread staging, full-capacity copy and zeroing every source cell are choices. Do not confuse this copy with an exact ordinary call. |
| Suspend/resume (`bytecodes.c:1629–1748`; ceval_macros.h:793`) | Transfers input/output owners, state and exception linkage, installs next-input recipient, unlinks suspended frame and releases CREATED/SUSPENDED state | Resumability, send/throw/close and generator exclusion are required. Entry representation and relocation are choices; current resume skipping already avoids executing idle RESUME but still checks instruction/eval-breaker/instrumentation state. |

## Hidden constants and observer state

Constant homes are inside full localsplus, between cells and free variables,
not an extra writable temporary tail (`Objects/codeobject.c:1918–1970`).
_PyFrame_FillConstSlots (`interpframe.h:282`) fills them on activation entry;
inline/immortal entries need no RC increment, replacement mortal constants do.
They increase physical capacity, scanning and copy work. A constant operand
class/pool could avoid repeating those homes, but changes register formats,
borrowing, code-constant lifetime, public constructors and folded constant
access. A bitmap marking them permanently live does not itself remove their
entry stores or copy capacity.

Some cold observer state is already lazy: frame_obj normally remains NULL;
materialization allocates a frame object and pending virtual kwargs mapping
only when an observer needs it (`Python/frame.c:36–90`). Traceback creation,
escaped-frame f_back and debugger reconstruction need coherent semantic phase,
not merely a fully initialized C header. The mapping-bearing eager-frame
experiment exposed exactly that distinction. Do not charge all observer
allocation to ordinary calls, and do not expose a private staging record to
GC discovery before its observer invariants hold.

Header cold candidates include mapping/frame-object linkage, redirect and
virtual-kwargs state, owner/GC flags and observer continuation details. Base,
saved frontier, active code/dispatch descriptors and return linkage have
ordinary hot consumers. A split layout must include side allocation and
pointer chasing for exec/class, callbacks, generators and frame APIs; “cold”
means frequency demonstrated per path, not a field with an observer-sounding
name. globals/builtins have hot LOAD_GLOBAL/LOAD_NAME consumers.

## Contracts a holistic replacement must make explicit

A live/owned bitmap is a serious alternative to NULL-as-validity, not refuted
by the rejected per-field experiments. It trades home clears/tests/scans for
bit updates, map lookup and new publication rules. Distinguish static liveness,
dynamic ownership (including unbound locals), pointer/inline type and semantic
commit phase: one bitmap does not automatically encode all four. Reuse static
IR maps where they suffice; dynamic state needs an identified authority.

Current frame-object traversal already avoids traversing attached active
frame homes as a second container edge (`Objects/frameobject.c:3927`). The
precise root marker can visit overlapping physical cells repeatedly because
marking is idempotent; that is not evidence of duplicate reference ownership.

Caller/callee overlapping windows need an ownership partition: the physical
cells may alias, but only the callee owns the transferred argument region.
Paused caller maps must exclude transferred aliases; return/error/escape must
restore or retire that authority. Heap callers, binding failures and native
reentry need equally explicit transitions. No callback may observe both maps
claiming the same owner or neither map claiming a still-owned value.

Map/bit updates and homes must commit together before a safepoint, fallible
allocation, escaped release, tracing event, suspension publication or observer
read. PRE failure must preserve its required owners or supply an equivalent valid
error-state recipe; successful stores must expose coherent POST state before any callback that can observe that POST state. A different documented
retirement policy may defer callbacks or retire owners in another order, with
unique ownership and bounded retention; it need not reproduce current
destructor timing or reverse-stack order. The generator already distinguishes
these cases and rejects unsequenced borrowed views. A replacement should
express call preparation/commit/return, scratch ownership and observation
phases in IR so cleanup and maps are generated from one contract rather than
retrofit per-op guards. It must include handlers, native bridges, C API ingress,
public code construction/marshal, instrumentation/deopt, GC subtraction/root
scans, debugger jumps, traceback and free-threaded publication.

Measure the complete model against exact overlapping calls, heap-caller calls,
creation/suspension, error-heavy/finalizer reentry, mapping entry and cold frame
observation. Inspect generated/native work and reached paths before timings.
Treat the previous isolated losses as evidence about those exact cache/guard/
allocation tradeoffs, not proof that a coherent bitmap or different call ABI
cannot win. No architecture is selected by this source audit.
